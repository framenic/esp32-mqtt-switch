#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
//#include "driver/gpio.h"

#include "comm_uart.h"
#include "user_config.h"
#include "config.h"
#include "comm_queue.h"

#define RX_BUF_SIZE  128

static const char *TAG = "comm_uart.c";

static void rx_task(void *arg)
{
    uint8_t* rx_buf = (uint8_t*) malloc(RX_BUF_SIZE+1);
	int i_rx_buf=0;
	char *saveptr,*token;
	char stopic[128], svalue[128];
	
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, &rx_buf[i_rx_buf], RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        
		if (rxBytes > 0) {
			rx_buf[i_rx_buf+rxBytes] = 0;
			printf("%s",&rx_buf[i_rx_buf]);
			fflush(stdout);
			//ESP_LOGI(TAG, "Read %d bytes: '%s'", rxBytes, &rx_buf[i_rx_buf]);
			i_rx_buf+=rxBytes;
            for (int i=0; i<i_rx_buf; i++) {
				uint8_t c = rx_buf[i];
				if ((c=='\r') || (c=='\n')) {
					printf("\n");
					rx_buf[i]=0; i_rx_buf=0;
					ESP_LOGI(TAG, "UART command: %s", rx_buf);
					token = strtok_r((char *)rx_buf, " ", &saveptr);
					sprintf(stopic, "%s/%s/%s", SUB_PREFIX, sysCfg.mqtt_topic, token);
					token = strtok_r(NULL, "", &saveptr);
					sprintf(svalue, "%s", (token == NULL) ? "" : token);
					comm_queue_rxsend(stopic, strlen(stopic), svalue, strlen(svalue));
				}
			}	
        } 
		/*
		rx_buf[i]=getchar();
		ESP_LOGI(TAG, "Read char: %c %x", rx_buf[i],rx_buf[i]);
		putchar(rx_buf[i]);
		if (rx_buf[i]=='\n') {
			i=0;
		}
		else i++;
		if (i>RX_BUF_SIZE) i=0;
		*/
    }
    free(rx_buf);
}

void comm_uart_init()
{
	const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	
	uart_driver_install(UART_NUM_0, RX_BUF_SIZE*2, 0, 0, NULL, 0);
	
	xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, 5, NULL);
}