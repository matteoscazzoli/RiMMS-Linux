#ifndef UDPCOMM_H 
#define UDPCOMM_H

#define RIMMS_SERVER_PORT 

typedef struct {
	int port;
	char address[100];
	char rxbuff[1024];
	char txbuff[1024];
	int sockfd;
	struct sockaddr_in serv_addr; 
	unsigned char status;
}NET;

#endif