#include <stdio.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

#include "config.h"
#include "http_serv.h"


#define D_HTML_LANGUAGE en

static const char *TAG = "http_serv.c";

#define OUTBUFFER_SIZE 1024

// static headers for HTTP responses
//const static char http_html_hdr[] = "HTTP/1.1 200 OK\n\n";
const static char http_404_hdr[] = "HTTP/1.1 404 NOT FOUND\n\n";
const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
//const static char http_html_page[] = "CIAO\n";

const char HTTP_HEADER[] =
  "<!DOCTYPE html>"
  "<html>"
  "<body>"
  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
  "<div align='center'>";
  
const char HTTP_FORM[] =
  "<h2>Network configuration</h2>"
  //"<form action='/action_page.php' method='post'>"
  "<form method='post'>"
  "<fieldset style='width:0px'>"
	  "<legend>WiFi Parameters:</legend>"
	  "AP SSID:<br>"
	  "<input type='text' name='wifissid' value='%s'>"
	  "<br>"
	  "Password:<br>"
	  "<input type='text' autocomplete='off' name='wifipassword' value='%s'>"
	  "<br>"
  "</fieldset>"
  "<fieldset style='width:0px'>"
	  "<legend>MQTT Parameters:</legend>"
	  "MQTT Host:<br>"
	  "<input type='text' name='mqtthost' value='%s'>"
	  "<br>"
	  "MQTT Port:<br>"
	  "<input type='text' name='mqttport' value='%d'>"
	  "<br>"
	  "MQTT User:<br>"
	  "<input type='text' name='mqttuser' value='%s'>"
	  "<br>"
	  "MQTT Password:<br>"
	  "<input type='text' autocomplete='off' name='mqttpassword' value='%s'>"
	  "<br>"  
  "</fieldset>"
  "<br>"
  "<input type='submit' name='save' value='Save'>"
  "<br><br>"
  "<input type='submit' name='savereboot' value='Save and Reboot'>"
  "</form>"
//  "<p>If you click the Submit button, the form-data will be sent to a page called /action_page.php.</p>"
	;

const char HTTP_STATUSUPDATED[] =
  "<br><br>" 
  "<p style='color:Tomato;'>Configuration updated, please reboot</p>";
  
const char HTTP_STATUSUPDATEDREBOOT[] =
  "<br><br>" 
  "<p style='color:Tomato;'>Configuration updated, REBOOTING...</p>";


const char HTTP_END[] =
  "</div>"  
  "</body>"
  "</html>";

#define POSTPARSE_NUM (sizeof(postparse)/sizeof(t_postparse))

typedef struct t_postparse {
	char *postkey;
	char *formatkey;
	size_t SYSCFGoffset;
} t_postparse;

const t_postparse postparse[] = { {"wifissid=","%s",offsetof(SYSCFG,sta_ssid)},{"wifipassword=","%s",offsetof(SYSCFG,sta_pwd)},
	{"mqtthost=","%s",offsetof(SYSCFG,mqtt_host)},{"mqttport=","%u",offsetof(SYSCFG,mqtt_port)},{"mqttuser=","%s",offsetof(SYSCFG,mqtt_user)},{"mqttpassword=","%s",offsetof(SYSCFG,mqtt_pass)}};
  
static char *outbuffer;

int urldecode(const char *source, char *dec)
{
	char *start=dec;
	
	while (*source!=0) {
		*dec=*source++;
		if (*dec=='+') *dec=' ';
		else if (*dec=='%') {
				if ((*source>='0') & (*source<='9')) *dec=*source-'0';
				else if ((*source>='A') & (*source<='F')) *dec=*source-'A'+10;
				else if ((*source>='a') & (*source<='f')) *dec=*source-'a'+10;
				else return -1;
				source++; *dec<<=4;
				if ((*source>='0') & (*source<='9')) *dec+=*source-'0';
				else if ((*source>='A') & (*source<='F')) *dec+=*source-'A'+10;
				else if ((*source>='a') & (*source<='f')) *dec+=*source-'a'+10;
				else return -1;
				source++;
		}
		++dec;
	}
	*dec=0;
	return (dec-start);
}
  
void http_server_contentsend(struct netconn *conn, const char* format, ...)  
{
  // This uses char strings. Be aware of sending %% if % is needed
  va_list args;
  va_start(args, format);
  
  int len = vsnprintf(outbuffer, OUTBUFFER_SIZE, format, args);
  va_end(args);	
  
  //ESP_LOGI(TAG, "HTTP server writing %d characters",len);
  //ESP_LOGI(TAG, "HTTP server writing: %s",outbuffer);
  
  if (len>0) netconn_write(conn, outbuffer, len, NETCONN_NOCOPY);

  
}  

static void http_server_netconn_serve(struct netconn *conn) {

	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;
	
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		outbuffer = malloc(OUTBUFFER_SIZE);
		
		// get the request and terminate the string
		netbuf_data(inbuf, (void**)&buf, &buflen);
		buf[buflen] = '\0';
		
		// get the request body and the first line
		char* body = strstr(buf, "\r\n\r\n");
		char *request_line = strtok(buf, "\n");
		
		if(request_line) {
			
			// default page
			if(strstr(request_line, "GET / ")) {
				ESP_LOGI(TAG, "HTTP server request %s",request_line);
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, HTTP_HEADER, sizeof(HTTP_HEADER) - 1, NETCONN_NOCOPY);
				//netconn_write(conn, HTTP_FORM, sizeof(HTTP_FORM) - 1, NETCONN_NOCOPY);
				http_server_contentsend(conn, HTTP_FORM,sysCfg.sta_ssid,"",sysCfg.mqtt_host,sysCfg.mqtt_port,sysCfg.mqtt_user,"");
				netconn_write(conn, HTTP_END, sizeof(HTTP_END) - 1, NETCONN_NOCOPY);
				
			}
			else if(strstr(request_line, "POST / ")) {
				int i;
				char* token;
				int token_len;
				SYSCFG *newcfg;
				
				newcfg = (SYSCFG*)malloc(sizeof(SYSCFG));
				*newcfg = sysCfg;
				
				//urldecode(body,outbuffer);
				
				ESP_LOGI(TAG, "HTTP server body %s",body);
				
				for(i=0;i<POSTPARSE_NUM;++i) {
						if((token=strstr(body,postparse[i].postkey))) {
							token=strchr(token,'=');
							if (token!=NULL) {
								token++;
								token_len=strchr(token,'&')-token;
								memcpy(outbuffer,token,token_len);
								outbuffer[token_len]=0;
								ESP_LOGI(TAG, "HTTP server POST key: %s value: %s",postparse[i].postkey,outbuffer);
								urldecode(outbuffer,outbuffer);
								ESP_LOGI(TAG, "HTTP server POST urldecoded key: %s value: %s len: %d",postparse[i].postkey,outbuffer,token_len);
								if (token_len!=0) sscanf(outbuffer,postparse[i].formatkey, (char *)newcfg+postparse[i].SYSCFGoffset);
								//ESP_LOGI(TAG, postparse[i].formatkey ,newcfg+postparse[i].SYSCFGoffset);
								//printf(postparse[i].formatkey, (char *)newcfg+postparse[i].SYSCFGoffset);
								printf("\n");
							}
							else break;
						}
						else break;
				}
				// update sysCfg only if parsing was complete
				if (i==POSTPARSE_NUM) sysCfg=*newcfg;
				
				
				// send back updated page
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, HTTP_HEADER, sizeof(HTTP_HEADER) - 1, NETCONN_NOCOPY);
				//netconn_write(conn, HTTP_FORM, sizeof(HTTP_FORM) - 1, NETCONN_NOCOPY);
				http_server_contentsend(conn, HTTP_FORM,sysCfg.sta_ssid,"",sysCfg.mqtt_host,sysCfg.mqtt_port,sysCfg.mqtt_user,"");
				if ((i==POSTPARSE_NUM) && (strstr(body,"save="))) netconn_write(conn, HTTP_STATUSUPDATED, sizeof(HTTP_STATUSUPDATED) - 1, NETCONN_NOCOPY);
				if ((i==POSTPARSE_NUM) && (strstr(body,"savereboot="))) netconn_write(conn, HTTP_STATUSUPDATEDREBOOT, sizeof(HTTP_STATUSUPDATEDREBOOT) - 1, NETCONN_NOCOPY);
				netconn_write(conn, HTTP_END, sizeof(HTTP_END) - 1, NETCONN_NOCOPY);
				if ((i==POSTPARSE_NUM) && (strstr(body,"savereboot="))) {
					// delay 4 seconds before restarting
					const TickType_t xDelay = 3000 / portTICK_PERIOD_MS;
					vTaskDelay( xDelay );					
					esp_restart();
				}
			}
			else {
				
				// get the requested resource
				char* method = strtok(request_line, " ");
				char* resource = strtok(NULL, " ");
				ESP_LOGI(TAG, "HTTP server request %s %s",request_line,resource);
				ESP_LOGI(TAG, "HTTP server body %s",body);
				
				netconn_write(conn, http_404_hdr, sizeof(http_404_hdr) - 1, NETCONN_NOCOPY);
				//spiffs_serve(resource, conn);
			}
		}
		free(outbuffer);
	}
}

static void http_server_task(void *pvParameters)
{
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	ESP_LOGI(TAG, "HTTP Server listening");
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
		vTaskDelay(10);
	} while(err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}

void http_serv_start()
{
	// start the HTTP server task
	xTaskCreate(&http_server_task, "http_server_task", 2048, NULL, 5, NULL);
	ESP_LOGI(TAG, "HTTP server started");
}

void http_serv_stop()
{
}

