#include "comm_queue.h"
#include <string.h> 

QueueHandle_t rx_queue, txqueue;

void comm_queue_init()
{
	rx_queue = xQueueCreate( 2, sizeof(rx_msg_t) );
	
}

void comm_queue_rxsend(char *topic, uint32_t topic_len, char* data, uint32_t data_len)
{
	struct rx_msg_t rx_msg;
	
	memcpy(rx_msg.topic,topic,topic_len);
	rx_msg.topic_len = topic_len;
	memcpy(rx_msg.data,data,data_len);
	rx_msg.data_len = data_len;
	
	xQueueSendToBack( rx_queue, &rx_msg, portMAX_DELAY);
}

void comm_queue_rxreceive(rx_msg_t *p_rx_msg)
{
	xQueueReceive(rx_queue, p_rx_msg, portMAX_DELAY);
}	