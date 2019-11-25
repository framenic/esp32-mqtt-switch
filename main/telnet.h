#ifndef __TELNET_H__
#define __TELNET_H__


#define TELNET_TIMEOUT 60 // seconds

// Send buffer size
#define MAX_TXBUFFER (512)

//extern char telnetInBuf[128];

/*
typedef struct telnetConnData {
	struct espconn *conn;
	uint16_t       txbufferlen;   // length of data in txbuffer
	char           *txbuffer;     // buffer for the data to send
	char           *sentbuffer;   // buffer sent, awaiting callback to get freed
	uint32_t       txoverflow_at; // when the transmitter started to overflow
	bool           readytosend;   // true, if txbuffer can be sent by espconn_sent
} telnetConnData;
*/

void telnetInit(int port);
void telnetSend(char *data, int len);


#endif /* __TELNET_H__ */