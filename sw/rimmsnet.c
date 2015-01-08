#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <pthread.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <pthread.h>
#include <linux/i2c-dev.h>

#include "sm.h"
#include "utility.h"
#include "main.h"
#include "rimms.h"
#include "display.h"
#include "protocolrimms.h"
#include "db.h"
#include "main.h"
#include "rimmsnet.h"

struct _NET RiMMSNet;

int nonblock(int sockfd)
{
    int opts;
    opts = fcntl(sockfd, F_GETFL);
    if(opts < 0)
    {
        //fprintf(stderr, "fcntl(F_GETFL) failed\n");
		return -1;
    }
    opts = (opts | O_NONBLOCK);
    if(fcntl(sockfd, F_SETFL, opts) < 0) 
    {
        //fprintf(stderr, "fcntl(F_SETFL) failed\n");
		return -1;
    }
	return 0;
}

int OpenRiMMSNet(void)
{
	char ip[22];
	struct timeval  timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
	int status;

	memset(RiMMSNet.rxbuff, '0',sizeof(RiMMSNet.rxbuff));
	//memset(RiMMSNet.txbuff, '0',sizeof(RiMMSNet.txbuff));
    if((RiMMSNet.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		WriteLog("OpenSocket: Could not create socket");
        return -1;
    }
	
	if(hostname_to_ip(RiMMSNet.address,ip)!=0){
		WriteLog("OpenSocket: hostname_to_ip error");
		return -1;
	}
		
	memset(&RiMMSNet.server, '0', sizeof(RiMMSNet.server));
		
	if(inet_pton(AF_INET, ip, &RiMMSNet.server.sin_addr)<=0){
		WriteLog("OpenSocket: inet_pton error occured\n");
		return -1;
	}
	RiMMSNet.server.sin_family = AF_INET;
	RiMMSNet.server.sin_port = htons(RiMMSNet.port); 
	RiMMSNet.length=sizeof(struct sockaddr_in);
	
	if(nonblock(RiMMSNet.sockfd)<0){
		WriteLog("OpenSocket: Not blocking Failed");
		close(RiMMSNet.sockfd);
		return -1;
	}
	
	return 0;
}

int CloseRiMMSNet (void)
{
	close(RiMMSNet.sockfd);
}


int InitRiMMSNet(void)
{
	RiMMSNet.status=0;
	RiMMSNet.timerconnect=GetClockSecond();
	return 0;
}


int GestRiMMSNet (void)
{char *retstr;
 char tmp[10];
 int n;
 
	switch(RiMMSNet.status){
		case 0://devo fare la connessione
			if((GetClockSecond()-RiMMSNet.timerconnect)>5){
				RiMMSNet.timerconnect=GetClockSecond();
				if(GetModemStatus()==1){ //sono online?
					if(OpenRiMMSNet()>=0){
						WriteDebug("GestRiMMSNet: server connected");
						sprintf(tmp,"PING\n");
						n=sendto(RiMMSNet.sockfd, tmp, strlen(tmp),0,(const struct sockaddr *)&RiMMSNet.server,RiMMSNet.length);
						if(n<=0){
							WriteDebug("GestRiMMSNet: error sending %i",n);
							CloseRiMMSNet();
							RiMMSNet.status=0;
						} else {
							WriteDebug("GestRiMMSNet: ping (%i)",n);
							RiMMSNet.timerconnect=GetClockSecond();
							RiMMSNet.status++;
						}
					}
				}
			}
			break;
			
		case 1://socket aperto..
			if(GetModemStatus()==0){
				CloseRiMMSNet();
				RiMMSNet.status=0;
				WriteDebug("GestRiMMSNet: server disconnected");
			}
			n = recvfrom(RiMMSNet.sockfd,RiMMSNet.rxbuff,sizeof(RiMMSNet.rxbuff),0,(struct sockaddr *)&RiMMSNet.from, &RiMMSNet.length);
			if (n > 0) { //ho ricevuto dati
				RiMMSNet.timerconnect=GetClockSecond();	
				retstr=(char *)ParserCommand((char *)RiMMSNet.rxbuff);
				//WriteDebug("GestRiMMSNet: (%s) (%i)",retstr,n);
				n=sendto(RiMMSNet.sockfd, retstr, strlen(retstr),0,(const struct sockaddr *)&RiMMSNet.server,RiMMSNet.length);
				if (n < 0) {
					WriteDebug("GestRiMMSNet: error sending %i",n);
					CloseRiMMSNet();
					RiMMSNet.status=0;
				} 
			}
			if((GetClockSecond()-RiMMSNet.timerconnect)>15){
				RiMMSNet.timerconnect=GetClockSecond();
				sprintf(tmp,"PING\n");
				n=sendto(RiMMSNet.sockfd, tmp, strlen(tmp),0,(const struct sockaddr *)&RiMMSNet.server,RiMMSNet.length);
				if(n<=0){
					WriteDebug("GestRiMMSNet: error sending %i",n);
					CloseRiMMSNet();
					RiMMSNet.status=0;
				} 
			}
			break;
			
		default:
			CloseRiMMSNet();
			RiMMSNet.status=0;
			break;
	}
}

