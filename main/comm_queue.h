#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t rx_queue, txqueue;

#define TOPIC_SIZE 40
#define DATA_SIZE 40

typedef struct rx_msg_t {
	char topic[TOPIC_SIZE];
	uint32_t topic_len;
	char data[DATA_SIZE];
	uint32_t data_len;
} rx_msg_t;

void comm_queue_init();

void comm_queue_rxsend(char *topic, uint32_t topic_len, char* data, uint32_t data_len);
void comm_queue_rxreceive(rx_msg_t *rx_msg);
void comm_queue_rxenable();
void comm_queue_rxdisable();