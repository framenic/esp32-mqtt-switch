// Copyright 2015 by Thorsten von Eicken, see LICENSE.txt

#include <stdint.h>
#include <string.h>
#include <esp_log.h>

#include "telnet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "comm_queue.h"
#include "user_config.h"
#include "config.h"

static const char *TAG = "telnet.c";

static int telnet_sock=-1; 

//static struct espconn telnetConn;
//static esp_tcp telnetTcp;

//char telnetInBuf[128];

// Connection pool
//telnetConnData connData;


// Telnet protocol characters
#define IAC        255  // escape



// Receive callback
static void telnetRecvCb(char *data, int len)
{
  char telnetInBuf[128];
  char stopic[128], svalue[128];
  char *saveptr; // for strok_r
  char *token;
  
  if ((len >= 1) && (data[0]==IAC)) {
	// telnet escape sequence, ignore
	return;
  }
  
  if (len<127) {
	  int i;
	  static int j=0;
	  for (i=0; i<len; i++) {
		uint8_t c = data[i];
		if ((c>=32) && (c<=127)) telnetInBuf[j++]=c;
		if (((c=='\r') || (c=='\n')) && (j!=0)) {
			telnetInBuf[j++]=0;
			j=0;
			ESP_LOGI(TAG, "Telnet command: %s", telnetInBuf);
			token = strtok_r(telnetInBuf, " ", &saveptr);
            sprintf(stopic, "%s/%s/%s", SUB_PREFIX, sysCfg.mqtt_topic, token);
            token = strtok_r(NULL, "", &saveptr);
            sprintf(svalue, "%s", (token == NULL) ? "" : token);
			//telnetInBuf[0] = 0;
			comm_queue_rxsend(stopic, strlen(stopic), svalue, strlen(svalue));
		}
	  }	
  }
  //else telnetInBuf[0]=0;
  //telnetInBuf[127]=0;
}

/*

//===== UART -> TCP

// Send all data in conn->txbuffer
// returns result from espconn_sent if data in buffer or ESPCONN_OK (0)
// Use only internally from espbuffsend and serbridgeSentCb
static sint8 ICACHE_FLASH_ATTR
sendtxbuffer(telnetConnData *conn)
{
  sint8 result = ESPCONN_OK;
  if (conn->txbufferlen != 0) {
    //os_printf("TX %p %d\n", conn, conn->txbufferlen);
    conn->readytosend = false;
    result = espconn_send(conn->conn, (uint8_t*)conn->txbuffer, conn->txbufferlen);
    conn->txbufferlen = 0;
    if (result != ESPCONN_OK) {
      //os_printf("sendtxbuffer: espconn_sent error %d on conn %p\n", result, conn);
      conn->txbufferlen = 0;
      if (!conn->txoverflow_at) conn->txoverflow_at = system_get_time();
	  conn->readytosend = true;
    } else {
      conn->sentbuffer = conn->txbuffer;
      conn->txbuffer = NULL;
      conn->txbufferlen = 0;
    }
  }
  return result;
}

// espbuffsend adds data to the send buffer. If the previous send was completed it calls
// sendtxbuffer and espconn_sent.
// Returns ESPCONN_OK (0) for success, -128 if buffer is full or error from  espconn_sent
// Use espbuffsend instead of espconn_sent as it solves the problem that espconn_sent must
// only be called *after* receiving an espconn_sent_callback for the previous packet.
static sint8 ICACHE_FLASH_ATTR
espbuffsend(telnetConnData *conn, const char data)
{
  
  // make sure we indeed have a buffer
  if (conn->txbuffer == NULL) { 
				conn->txbuffer = (char *)os_zalloc(MAX_TXBUFFER);
				conn->txbufferlen = 0;
  }
  if (conn->txbuffer == NULL) {
    //os_printf("espbuffsend: cannot alloc tx buffer\n");
    return -128;
  }
      
  // add to send buffer
  if (conn->txbufferlen+1 < MAX_TXBUFFER) {
      //uint16_t avail = conn->txbufferlen+len > MAX_TXBUFFER ? MAX_TXBUFFER-conn->txbufferlen : len;
	  
	  conn->txbuffer[conn->txbufferlen] = data;
	  conn->txbufferlen++;

	  // send if end of line or buffer full
	  sint8 result = ESPCONN_OK;
	  if ((conn->readytosend)&&((data=='\n') || (conn->txbufferlen >= MAX_TXBUFFER))) 
		  result = sendtxbuffer(conn);
	  return result;
   }
   else { // buffer overflow
      conn->txbufferlen = 0;
	  //os_printf("espbuffsend: tx buffer overflow\n");
	  return -128; 
   }  
   
}

//callback after the data are sent
static void ICACHE_FLASH_ATTR
telnetSentCb(void *arg)
{
  telnetConnData *conn = ((struct espconn*)arg)->reverse;
  //os_printf("Sent CB %p\n", conn);
  if (conn == NULL) return;
  //os_printf("%d ST\n", system_get_time());
  if (conn->sentbuffer != NULL) os_free(conn->sentbuffer);
  conn->sentbuffer = NULL;
  conn->readytosend = true;
  conn->txoverflow_at = 0;
  //sendtxbuffer(conn); // send possible new data in txbuffer
}


// callback for redirecting os_printf to telnet
void ICACHE_FLASH_ATTR
telnet_write_char(char c)
{
  uart0_write_char(c);
  if (((c>=32) && (c<=127)) || (c=='\a') || (c=='\n')) espbuffsend(&connData, c);
}

//===== Connect / disconnect

// Disconnection callback
static void ICACHE_FLASH_ATTR
telnetDisconCb(void *arg)
{
  os_install_putc1(uart0_write_char);
  
  telnetConnData *conn = ((struct espconn*)arg)->reverse;
  if (conn == NULL) return;
  // Free buffers
  if (conn->sentbuffer != NULL) os_free(conn->sentbuffer);
  conn->sentbuffer = NULL;
  if (conn->txbuffer != NULL) os_free(conn->txbuffer);
  conn->txbuffer = NULL;
  conn->txbufferlen = 0;
  
  conn->conn = NULL;
  
}

// Connection reset callback (note that there will be no DisconCb)
static void ICACHE_FLASH_ATTR
telnetResetCb(void *arg, sint8 err)
{
  telnetDisconCb(arg);
  //INFO("telnet: connection reset err=%d\n", err);
}

// New connection callback, use one of the connection descriptors, if we have one left.
static void ICACHE_FLASH_ATTR
telnetConnectCb(void *arg)
{
  struct espconn *conn = arg;
    
  // Find if conndata is available
  
  if (connData.conn!=NULL) {
	espconn_disconnect(conn);
    return;  
  }
  
  #ifdef TELNET_DBG
  os_printf("Accept port %d, conn=%p, pool slot %d\n", conn->proto.tcp->local_port, conn, i);
  #endif
  os_memset(&connData, 0, sizeof(struct telnetConnData));
    
  connData.conn = conn;
  conn->reverse = &connData;
  connData.readytosend = true;
    
  espconn_regist_recvcb(conn, telnetRecvCb);
  espconn_regist_disconcb(conn, telnetDisconCb);
  espconn_regist_reconcb(conn, telnetResetCb);
  espconn_regist_sentcb(conn, telnetSentCb);

  espconn_set_opt(conn, ESPCONN_REUSEADDR|ESPCONN_NODELAY);
  
  os_install_putc1(telnet_write_char);
}
*/
static void do_telnetrx(const int sock)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            //ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
			telnetRecvCb(rx_buffer,len);
        }
    } while (len > 0);
}

static void do_telnettx(const int sock,char *txdata,int len)
{
            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation. 
            int to_write = len;
			
            while (to_write > 0) {
                int written = send(sock, txdata + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
					return;
                }
                to_write -= written;
            }
}

static int telnet_printf(const char *fmt, va_list args)
{
	char buff[80];
	
	vsnprintf(buff,80,fmt,args);
	int len=strlen(buff);
	telnetSend(buff,len);
    return vprintf(fmt, args);
}

static void telnet_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
	int port = *(int *)pvParameters;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", port);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
		if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
		telnet_sock = sock;
        // Convert ip address to string
        if (source_addr.sin6_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        } else if (source_addr.sin6_family == PF_INET6) {
            inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

		//vprintf_like_t saved_printf=esp_log_set_vprintf(telnet_printf);
        do_telnetrx(sock);
		telnet_sock=-1;
		//esp_log_set_vprintf(saved_printf);
		
        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}



// Start telnet TCP server on specified port (typ. 23)
void telnetInit(int port)
{
  static int port_n;
  
  port_n=port;
  /*
  memset(&connData, 0, sizeof(connData));
  //memset(&telnetTcp, 0, sizeof(telnetConnData));
  
  // set-up port
  telnetConn.type = ESPCONN_TCP;
  telnetConn.state = ESPCONN_NONE;
  telnetTcp.local_port = port;
  telnetConn.proto.tcp = &telnetTcp;

  espconn_regist_connectcb(&telnetConn, telnetConnectCb);
  espconn_accept(&telnetConn);
  espconn_tcp_set_max_con_allow(&telnetConn, 1);
  espconn_regist_time(&telnetConn, TELNET_TIMEOUT, 0);
  */
  xTaskCreate(telnet_server_task, "telnet_server", 4096, &port_n, 5, NULL);
  
}

void telnetSend(char *txdata, int len)
{
	if (telnet_sock>=0) {
		do_telnettx(telnet_sock,txdata,len);
	}
}