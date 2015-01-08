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

static char Buffclient[1024];
static int sockfd;
static struct sockaddr_in serv_addr; 

static NET RiMMSNet;

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
	memset(RiMMSNet.rxbuff, '0',sizeof(RiMMSNet.rxbuff));
	memset(RiMMSNet.txbuff, '0',sizeof(RiMMSNet.txbuff));
    if((RiMMSNet.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		WriteLog("OpenSocket: Could not create socket");
        return -1;
    }
	memset(&RiMMSNet.serv_addr, '0', sizeof(RiMMSNet.serv_addr));
	RiMMSNet.serv_addr.sin_family = AF_INET;
    RiMMSNet.serv_addr.sin_port = htons(RiMMSNet.port); 
	
	
	if(inet_pton(AF_INET, RiMMSNet.address, &RiMMSNet.serv_addr.sin_addr)<=0){
        WriteLog("OpenSocket: inet_pton error occured");
        return -1;
    }
	
	if(connect(RiMMSNet.sockfd, (struct sockaddr *)&RiMMSNet.serv_addr, sizeof(RiMMSNet.serv_addr)) < 0){
       WriteLog("OpenSocket: Connect Failed");
       return -1;
    }
	
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
}


int GestRiMMSNet (void)
{char *retstr	
	switch(RiMMSNet.status){
		case 0://devo fare la connessione
			if(GetModemStatus()==1){ //sono online?
				if(OpenRiMMSNet()>=0){
					RiMMSNet.status++;
				}
			}
			break;
			
		case 1://socket aperto..
			n = read(RiMMSNet.sockfd,RiMMSNet.rxbuff,sizeof(RiMMSNet.rxbuff));
			if (n < 0) {
				CloseRiMMSNet();
				RiMMSNet.status=0;
			}
			else if (n > 0) { //ho ricevuto dati
				retstr=(char *)ParserCommand((char *)RiMMSNet.rxbuff);
				write(RiMMSNet.sockfd, retstr, strlen(retstr)); 
			}
			break;
		default:
			CloseRiMMSNet();
			RiMMSNet.status=0;
			break;
	}
}

