#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
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
#include "modem.h"

int facm;
struct _modem Modem;

int open_modem(char *dev_name)
{  struct termios options;
    
	/* open the port */
	WriteDebug("Trying open modem on (%s)",dev_name);
	facm = open(dev_name, O_RDWR | O_NOCTTY | O_NDELAY);
	if(facm==-1) return -2;
	//fcntl(fd, F_SETFL, 0);

	/* get the current options */
	tcgetattr(facm, &options);

	/* set raw input, 0.1 second timeout */
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_cflag     |= (CLOCAL | CREAD);
	options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag     &= ~OPOST;
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 1;

	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);
	
	/* set the options */
	tcsetattr(facm, TCSANOW, &options);
	tcflush(facm, TCIFLUSH);      // flush old data
		
   	return (facm);
}

int ModemRx (char *parametro)
{	int readchar,i;
	char temp[1024];
	
	if(parametro==NULL){
		return 0;
	}
	i=0;
	do{
		
		ioctl(facm, FIONREAD, &readchar);
		if(readchar>=strlen(parametro)){
			readchar = read(facm,temp,1024);
			temp[readchar]=0;
			if(strstr(temp,parametro)!=NULL){
				return 1;
			}
		}
		usleep(10000);
	}while(i++<100);
	return 0;
}

int SendModemCmd(char *cmd, char *response)
{
	if(write(facm,cmd,strlen(cmd))>0){
		return ModemRx(response);
	} 
	WriteDebug("Error writing to modem");
	return 0;
}

char *SendModemCmdR(char *cmd, char *response)
{	int readchar,i;
	char temp[1024],*param;
	
	if(cmd!=NULL){
		if(write(facm,cmd,strlen(cmd))<=0){
			WriteDebug("Error writing to modem");
			return 0;
		}
	}
	if(response==NULL) {
		return 0;
	}
	i=0;
	do{
		ioctl(facm, FIONREAD, &readchar);
		if(readchar>=strlen(response)){
			readchar = read(facm,temp,1024);
			temp[readchar]=0;
			param = strstr(temp,response);
			return param;
		}
		usleep(10000);
	}while(i++<100);
	return 0;
}

int InitModem(void)
{char *tmp;

	WriteDebug("Modem port (%s)",Modem.port);
	facm = open_modem(Modem.port);
	if(facm==-2){
		WriteLog("Modem non presente");
		Modem.present=0;
	} else if(facm==-1){
		WriteLog("Error opening modem");
		return -1;
	} else {
		Modem.present=1;
		if(SendModemCmd("ATZ\r","OK")==0){
			WriteLog("Modem ATZ Error");
			return -1;
		}
		if(SendModemCmd("AT+CPIN?\r","READY")==0){
			WriteLog("Modem AT+CPIN? Error");
			return -1;
		}
		if(SendModemCmd("AT+CMGF=1\r","OK")==0){
			WriteLog("Modem AT+CMGF=1 Error");
			return -1;
		}
		if(SendModemCmd("AT+CLIP=1\r","OK")==0){
			WriteLog("Modem AT+CLIP=1 Error");
			return -1;
		}
		if(SendModemCmd("AT+CSQ\r","+CSQ")==0){
			WriteLog("Modem AT+CSQ Error");
			return -1;
		}
		strcpy(Modem.operatorname,"---");
		Modem.signal = 0;
		Modem.timergest = GetClockSecond();
		WriteLog("Modem ok");
	}
	return 0;
}

int GetModemSignal(void)
{
	return Modem.signal;
}

int ModemSignal(void)
{char *string;
 int len,i,x,y;
 char param[100][1024];
	
	string=SendModemCmdR("AT+CSQ\r","+CSQ:");
	if(string){
		len=strlen(string);
		//WriteDebug("Signal %s (%d)",string,len);
		x=0;
		y=0;
		for(i=0;i<len;i++){
			if((string[i]!=' ')&&(string[i]!=',')&&(string[i]!='\n')&&(string[i]!='\r')&&(string[i]!='\0')){
				param[x][y++]=string[i];
			} else {
				if(y>0){
					param[x][y]='\0';
					x++;
					y=0;
				}
			}
		}
		if(x>2){
			Modem.signal=atoi(param[1]);
		}
	}
	return 0;
}

/*
int ModemClip(void)
{char *string,*num;
 int len,i,x,y;
 char param[100][1024];
	
	string=SendModemCmdR(NULL,"+CLIP:");
	if(string){
		len=strlen(string);
		x=0;
		y=0;
		for(i=0;i<len;i++){
			if((string[i]!=' ')&&(string[i]!=',')&&(string[i]!='\n')&&(string[i]!='\r')&&(string[i]!='\0')){
				param[x][y++]=string[i];
			} else {
				if(y>0){
					param[x][y]='\0';
					x++;
					y=0;
				}
			}
		}
		if(x>=2){
			param[1][25]=0;
			strcpy(Modem.callingnumber,param[1]);
			if(SendModemCmd("ATH\r","OK")){
				num=NULL;
				num=strstr(Modem.callingnumber,Modem.wakeupnumber[0]);
				if(num!=NULL){
					return 1;
				}
				num=strstr(Modem.callingnumber,Modem.wakeupnumber[1]);
				if(num!=NULL){
					return 1;
				}
				num=strstr(Modem.callingnumber,Modem.wakeupnumber[2]);
				if(num!=NULL){
					return 1;
				}
				WriteLog("Chiamata sconosciuta da %s",Modem.callingnumber);
			}
		}
	} 
	//strcpy(Modem.callingnumber,'----');
	return 0;
}
*/

char *GetModemOperator(void)
{
	return Modem.operatorname;
}

int ModemOperator(void)
{char *string;
 int len,i,x,y;
 char param[100][1024];
	
	string=SendModemCmdR("AT+COPS?\r","+COPS:");
	if(string){
		len=strlen(string);
		x=0;
		y=0;
		for(i=0;i<len;i++){
			if((string[i]!='"')&&(string[i]!=' ')&&(string[i]!=',')&&(string[i]!='\n')&&(string[i]!='\r')&&(string[i]!='\0')){
				param[x][y++]=string[i];
			} else {
				if(y>0){
					param[x][y]='\0';
					x++;
					y=0;
				}
			}
		}
		if(x>4){
			param[4][3]=0; //casto il nome operatore a 3 lettere
			strcpy(Modem.operatorname,param[4]);
			Modem.registered=1;
		} else {
			strcpy(Modem.operatorname,"---");
			Modem.registered=0;
		}

	}
	return 0;
}

int CloseModem(void)
{
	close(facm);
}

int GetRemoteConnection(void)
{
	system("rm ramdisk/ppponline.txt");
	system("ifconfig ppp | grep inet >> ramdisk/ppponline.txt");
	usleep(500000);
	if(ReadParamFile("inet addr","ramdisk/ppponline.txt")!=0){
		return 1;
	}
	return 0;
}

int RemoteConnection(unsigned char onoff)
{int timer,retry,readchar,i;
 char c;
 char ptr[256];
 

	if(onoff){
		if(GetRemoteConnection()){ //siamo già connessi?
			return 1;
		}
		retry = 0;
		do{
			system("rm ramdisk/ppponline.txt");
			system("pon");
			timer = 0;
			do{
				if(GetRemoteConnection()){ //è connessi?
					return 1;
				}
			}while(timer++<15);
			system("poff");
			usleep(500000);
		}while(retry++<5);
	} else {
		if(GetRemoteConnection()){
			system("poff");
		}
		return 1;
	}
	return 0;
}

int GetModemStatus (void)
{
	if(Modem.status==MODEM_ONLINE){
		return 1;
	}
	return 0;
}

int SetModemStatus(unsigned char status)
{
	if(status==1){
		Modem.desiredstatus=1;
	}
	return 1;
}

int GestModem(void)
{//int retry,readchar;
 //char c;
 //char filetmp[256];
 
	/*if(Modem.type==MODEMTYPE_TTY){
		switch(Modem.status){
			case MODEM_NOTPRES:	
							if(Modem.present){
								Modem.status=MODEM_NOTREG;
							}
							break;
			case MODEM_NOTREG:
							if(Modem.present){
								if((GetClockSecond()-Modem.timergest)>=5){
									Modem.timergest = GetClockSecond();
									ModemSignal();
									ModemOperator();
								}
								if(Modem.registered){
									Modem.status=MODEM_OFFLINE;
								} else if(Modem.present==0){
									Modem.status=MODEM_NOTPRES;
								}
							} else {
								Modem.status=MODEM_NOTPRES;
							}
							break;
			case MODEM_OFFLINE:	
							if(Modem.present){
								if((GetClockSecond()-Modem.timergest)>=5){
									Modem.timergest = GetClockSecond();
									ModemSignal();
									ModemOperator();
								}
								if(Modem.registered){
									if(ModemClip()){
										WriteLog("Richiesto wakeup da %s",Modem.callingnumber);
										WriteDisplayBuf("Wakeup!!");
										Modem.status=MODEM_GOONLINE;
									}
									if(Modem.desiredstatus){	//richiesta di andare online
										Modem.status=MODEM_GOONLINE;
										Modem.desiredstatus=0;
									}
								} else {
									Modem.status=MODEM_NOTREG;
								}
							} else {
								Modem.status=MODEM_NOTPRES;
							}
							break;
			case MODEM_GOONLINE:
							if(RemoteConnection(1)==1){
								Modem.desiredstatus=0;
								Modem.status=MODEM_ONLINE;
								WriteDisplayBuf("ONLINE");
								system("./redir_ip.sh &");
								Modem.timeronline = GetClockSecond();
							} else {
								Modem.status=MODEM_OFFLINE;
							}
							break;
			case MODEM_ONLINE:	
							if(Modem.desiredstatus){
								Modem.timeronline = GetClockSecond();
								Modem.desiredstatus=0;
							}
							if((GetClockSecond()-Modem.timeronline)>=90){
								Modem.status=MODEM_GOOFFLINE;
							}
							if(GetRemoteConnection()==0){
								WriteDisplayBuf("OFFLINE");
								Modem.status=MODEM_OFFLINE;
								system("poff");
							}
							sleep(2);
							break;
			case MODEM_GOOFFLINE:
							if(RemoteConnection(0)==1){
								WriteDisplayBuf("OFFLINE");
								Modem.status=MODEM_OFFLINE;
							}
							break;
		}
	}else */
	if(Modem.type==MODEMTYPE_USB){
		if((GetClockSecond()-Modem.timergest)>=5){		//continuo a chiedere stato al modem
			Modem.timergest = GetClockSecond();
			if(Modem.status!=MODEM_NOTPRES){
				ModemSignal();
				ModemOperator();
			}
		}
		switch(Modem.status){
			case MODEM_NOTPRES:
							if(Modem.present){
								Modem.status=MODEM_NOTREG;
							}
							break;
			case MODEM_NOTREG:
			case MODEM_OFFLINE:
							if(Modem.present){
								if(Modem.registered){
									Modem.status=MODEM_GOONLINE;
								} else if(Modem.present==0){
									Modem.status=MODEM_NOTPRES;
								}
							} else {
								Modem.status=MODEM_NOTPRES;
							}
							break;
			case MODEM_GOONLINE:
							if(Modem.registered){
								if(RemoteConnection(1)==1){
									Modem.desiredstatus=0;
									Modem.status=MODEM_ONLINE;
									Modem.timeronline = GetClockSecond();
									WriteDisplayBuf("ONLINE");
									system("./redir_ip.sh &");
									Modem.timercheck=GetClockSecond();
								} else {
									Modem.status=MODEM_GOONLINE;
								}
							} else {
								Modem.status=MODEM_NOTREG;
							}
							break;
			case MODEM_ONLINE:	
							if(Modem.registered){
								if((GetClockSecond()-Modem.timercheck)>=3){
									Modem.timercheck=GetClockSecond();
									if(GetRemoteConnection()==0){	//se la connessione è caduta cerco di tornare online
										WriteDisplayBuf("OFFLINE");
										system("poff");
										Modem.status=MODEM_GOONLINE;
									} else {
										if((GetClockSecond()-Modem.timeronline)>=90){
											Modem.timeronline = GetClockSecond();
											system("./redir_ip.sh &");
										}
									}
								}
							} else {
								system("poff");
								Modem.status=MODEM_NOTREG;
							}
							break;
			case MODEM_GOOFFLINE:
							Modem.status=MODEM_GOONLINE;
							break;
		}
	} else 
	if(Modem.type==MODEMTYPE_LAN){
		switch(Modem.status){
			case MODEM_NOTPRES:
			case MODEM_NOTREG:
			case MODEM_OFFLINE:
			case MODEM_GOONLINE:
							Modem.status=MODEM_ONLINE;
							WriteDisplayBuf("ONLINE");
							system("./redir_ip.sh &");
							Modem.timeronline = GetClockSecond();
							strcpy(Modem.operatorname,"LAN");
							Modem.signal = 0;
							break;
			case MODEM_ONLINE:
							if((GetClockSecond()-Modem.timeronline)>=90){
								Modem.timeronline = GetClockSecond();
								system("./redir_ip.sh &");
							}
							break;
			case MODEM_GOOFFLINE:
							Modem.status=MODEM_GOONLINE;
							break;
		}
	}
	return 0;
}