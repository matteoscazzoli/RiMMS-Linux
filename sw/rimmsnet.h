#ifndef UDPCOMM_H 
#define UDPCOMM_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


#define RIMMS_SERVER_PORT 

struct _NET{
	int port;
	char address[100];
	char rxbuff[1024];
	int sockfd;
	int timerconnect;
	struct sockaddr_in server; 
	struct sockaddr_in from;
	unsigned int length;
	unsigned char status;
};

extern struct _NET RiMMSNet;

#endif