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
#include "modem.h"

int fdisplay,fdexpander;

char *displaybuffer;
unsigned short displaybuffercount,displaybufferin,displaybufferout;

char displayline1[2][17];    //y,x
char displaymemline1[2][17]; //y,x
unsigned char displaynewmsg,displaybusy,displaygest;
unsigned int  timerdisplay,timernewmsg;

unsigned char flagWriteDisplayBuf;

int lcdsend(int fd,int mode,unsigned char value) {
	unsigned char buffer[2];

	if (mode==0) {	
		buffer[0]=0x00;		
	} else {
		buffer[0]=0x40;		
	}
	buffer[1]=value;		
	if (write(fd,buffer,2)!=2) {
		WriteLog("Error writing file: %s\n", strerror(errno));
		return -1;
	}
	usleep(1000);
	return 0;
}

int lcdclear(int fd) 
{   int ret;

	ret = -1;
	ret=lcdsend(fd,0,0x01);
	return ret;
}

int lcdsetcurpos(int fd, int x, int y) 
{	int ret;
	
	ret = -1;
	if (y<0 || y>1) return -1;
	if (x<0 || x>15) return -1;

	if (y==0){
		ret=lcdsend(fd,0,0x80+0x00+x);
	}else{
		ret=lcdsend(fd,0,0x80+0x40+x);
	}
	return ret;
}

int lcdputchar(int fd, char value) 
{
	int ret;
	ret = -1;
	
	ret=lcdsend(fd,1,value);
	return ret;
}

int lcdputstring(int fd,char *string) 
{
	int i,ret;
	
	ret = 1;
	if (strlen(string)==0) return -1;
	if (strlen(string)>16) {
		sprintf(string,"%16s",string);
	}

	for (i=0;i<strlen(string);i++) {
		ret=lcdputchar(fd,string[i]);
	}
	return ret;
}

int expsend(int fd,unsigned char value) 
{
	unsigned char buffer;
	
	buffer=value;		
	if (write(fd,&buffer,1)!=1) {
		WriteLog("Error writing exp: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int InitExpander(void)
{	unsigned char buffer;
	
	buffer = 0x0f;
	if (write(fdexpander,&buffer,1)!=1) {
		WriteLog("Error writing init expander: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int Backlight (unsigned char onoff)
{
	unsigned char buffer;
	
	if(read(fdexpander, &buffer, 1)==-1){
		WriteLog("Error reading Expander");
		return -1;
	}
	if(onoff==0){ 
		buffer&=0xEF;
	} else if(onoff==1){
		buffer|=0x10;
	} else {
		WriteLog("Error backlight value: %u",onoff);
		return -1;
	}
	if (write(fdexpander,&buffer,1)!=1) {
		WriteLog("Error writing backlight: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int Key (void)
{	unsigned char buffer;

	if(read(fdexpander, &buffer, 1)!=1){
		WriteLog("Error reading Key");
		return -1;
	}
	return buffer;
}

int GetKey(unsigned char key)
{
	int ret;
	
	ret = Key();
	if(ret==-1){
		WriteLog("Error reading keyboard");
		return -1;
	}
	
	switch(key){
		case KEY1:
			if(ret&0x01) return 0;
			else return 1;
			break;
		case KEY2:
			if(ret&0x02) return 0;
			else return 1;
			break;
		case KEY3:
			if(ret&0x04) return 0;
			else return 1;
			break;
		case KEY4:
			if(ret&0x08) return 0;
			else return 1;
			break;
		default:
			WriteLog("Error keyboard key");
	}
	return -1;
}

int lcdclose(void)
{
	free(displaybuffer);
	close(fdisplay);
	return 0;
}

int lcdinit(void)
{	int ret;
	
	ret=-1;
	
	fdisplay = open("/dev/i2c-0", O_RDWR);

	if (fdisplay < 0) {
		WriteLog("Error opening display: %s", strerror(errno));
		return -1;
	}

	if (ioctl(fdisplay, I2C_SLAVE, I2C_ADDR_DISPLAY) < 0) {
		WriteLog("display ioctl error: %s", strerror(errno));
		return -1;
	}
	
	fdexpander = open("/dev/i2c-0", O_RDWR);

	if (fdexpander < 0) {
		printf("Error opening expander: %s", strerror(errno));
		return -1;
	}
	
	if (ioctl(fdexpander, I2C_SLAVE, I2C_ADDR_EXP) < 0) {
		printf("expander ioctl error: %s", strerror(errno));
		return -1;
	}

	ret=lcdsend(fdisplay,0,0x38);
	ret=lcdsend(fdisplay,0,0x39);
	ret=lcdsend(fdisplay,0,0x14); //Internal OSC freq
	ret=lcdsend(fdisplay,0,0x72); //Set contrast 
	ret=lcdsend(fdisplay,0,0x54); //Power/ICON control/Contrast set
	ret=lcdsend(fdisplay,0,0x6F); //Follower control
	ret=lcdsend(fdisplay,0,0x0C); //Display ON
	ret=lcdclear(fdisplay);	
	
	ret=InitExpander();
	if(ret!=0){
		WriteLog("Error init IO expander");
		return -1;
	}
	ret=Backlight(0);
	if(ret!=0){
		WriteLog("Error init Backlight");
		return -1;
	}
	
	displaybuffer = (char *)calloc(MAXDISPLAYBUFFER, sizeof(displaybuffer[0]));
	if(displaybuffer==0){
		WriteLog("Error init buffer display");
		return -1;
	}
	
	displaynewmsg=0;
	timerdisplay=time(NULL)-9;
	displaybusy=0;
	
	flagWriteDisplayBuf=0;
	displaybuffercount=0;
	displaybufferin=0;
	displaybufferout=0;
	
	return 0;
}


int WriteDisplayBuf(char *str,...)
{
	int timeout;
	char tmp[64];
	unsigned short i;
	
	timeout=0;
	while(flagWriteDisplayBuf) {
		if(++timeout>=100) return -1;
		usleep(10);
	}
	flagWriteDisplayBuf = 1;
	va_list arglist;
	va_start(arglist,str);
	vsprintf(tmp,str,arglist);
	va_end(arglist);
	for(i=0;i<strlen(tmp)+1;i++){
		if(displaybuffercount<MAXDISPLAYBUFFER){
			displaybuffer[displaybufferin++]=tmp[i];
			if(displaybufferin>=MAXDISPLAYBUFFER){
				displaybufferin=0;
			}
			displaybuffercount++;
		} else {
			if(i>0) { //stavo inserendo
				displaybuffer[displaybufferin]=0; //aggiungo il terminatore!!
			}
			flagWriteDisplayBuf = 0;
			return i;
		}
	}
	flagWriteDisplayBuf = 0;
	return 0;
}

int ReadDisplayBuf(char *tmp)
{unsigned short count;
	
	count=0;
	
	while(displaybuffercount>0){
		*tmp=displaybuffer[displaybufferout++];
		if(displaybufferout>=MAXDISPLAYBUFFER){
			displaybufferout=0;
		}
		displaybuffercount--;
		if(*tmp==0) return count;
		if (count>64){
			*tmp=0;
			return count;
		}
		tmp++;
		count++;
	}
	*tmp=0;
	return count;
}

int WriteDisplay(unsigned char x,unsigned char y,char *str,...)
{
    char tmp[35];
	int timeout;
		
    if(GetKeybStatus()) return 1;
	if(strlen(str)>16) return -1;
	timeout=0;
	while(displaybusy) {
		timeout++;
		if(timeout>=100) return -1;
		usleep(10);
	}
	displaybusy=1;
	memset(&displayline1[y],'\0',16);
    va_list arglist;
    va_start(arglist,str);
    vsprintf(tmp,str,arglist);
    va_end(arglist);
	if(y<0 || y>1) return -1;
	if((strlen(tmp)+x)>16) return -1;
	sprintf(&displayline1[y][x],"%s",tmp);
	if(GetAlim()) Backlight(1);
	if(displaygest==0){
		WriteToDisplay();
	}
	displaybusy=0;
	return 0;
}
/*
int WriteDisplayFromKeyb(unsigned char x,unsigned char y,char *str,...)
{
    char tmp[35];
	int timeout;
		
    if(strlen(str)>16) return -1;
	timeout=0;
	while(displaybusy) {
		timeout++;
		if(timeout>=100) return -1;
		usleep(10);
	}
	displaybusy=1;
	memset(&displayline1[y],'\0',16);
    va_list arglist;
    va_start(arglist,str);
    vsprintf(tmp,str,arglist);
    va_end(arglist);
	if(y<0 || y>1) return -1;
	if((strlen(tmp)+x)>16) return -1;
	sprintf(&displayline1[y][x],"%s",tmp);
	WriteToDisplay();
	displaybusy=0;
	return 0;
}
*/
int WriteStatus(void)
{
	memset(&displayline1[0][0],'\0',16);
	sprintf(&displayline1[0][0],"%s-",GetModemOperator());
	sprintf(&displayline1[0][4],"%02d",GetModemSignal());
	if(GetModemStatus()){
		sprintf(&displayline1[0][7],"ONL ");
	} else {
		sprintf(&displayline1[0][7],"OFFL");
	}
	if(GetAlim()){
		sprintf(&displayline1[0][12],"PWR");
	} else {
		sprintf(&displayline1[0][12],"BAT");
	}
	return 0;
}

int WriteTime(void)
{
	time_t mytime;
    struct tm *mytm;
	char tmp[16];
	
	memset(&displayline1[1][0],'\0',16);
	mytime=time(NULL);
    mytm=localtime(&mytime);
	strftime(&displayline1[1][0],16,"%d/%m %H:%M:%S",mytm); //2008 08 29 08 51 42
	return 0;
	
}

int WriteToDisplay(void)
{unsigned char x,y;

	for(y=0;y<2;y++){
		for(x=0;x<17;x++){
			if(displayline1[y][x]!=displaymemline1[y][x]){
				displaymemline1[y][x]=displayline1[y][x];
				lcdsetcurpos(fdisplay,x,y);
				lcdputchar(fdisplay,displayline1[y][x]);
			}
		}
	}
	return 0;
}


int GestDisplay(void)
{static char tmpbuffer[64];
 static unsigned short len,indexvis;
 static unsigned int timershift;

	if(displaynewmsg==0){ //se non sto visualizzando messaggi dal buffer
		if(displaybuffercount==0){ //se non ho nuovi messaggi nel buffer da visualizzare
			if((GetClockSecond() - timerdisplay) >= 1) { //normale visualizzazione di stato e ora
				timerdisplay = GetClockSecond();
				WriteStatus();
				WriteTime();
			}
		} else { //ho nuovi messaggi da visualizzare e carico
			len=ReadDisplayBuf(tmpbuffer);
			WriteDebug("Display mez:(%s)(%u)",tmpbuffer,len);
			displaynewmsg = 1;
			indexvis=0;
			timershift = GetClockMsec()+400;
			timernewmsg = GetClockSecond();
			if(GetAlim()) Backlight(1);
		}
	} else { //visualizzazione nuovi messaggi dal buffer
		if((GetClockSecond() - timerdisplay) >= 1) { //normale visualizzazione dell'ora
			timerdisplay = GetClockSecond();
			WriteTime();
		}
		if((GetClockSecond() - timernewmsg) >= 10){ //scaduto il timer per visualizzazione messaggio
			displaynewmsg = 0;
			Backlight(0);
		} else {
			if(len<=16){
				//sprintf(&displayline1[0][0],"%s",tmpbuffer);
				memset(&displayline1[0][0],'\0',16);
				memcpy((char *)&displayline1[0][0], (char *)tmpbuffer,len);
			} else {	//il messaggio è più lungo dei caratteri del display e lo faccio scorrere
				if((GetClockMsec()-timershift)>=400){
					timershift = GetClockMsec();
					if(indexvis<(len-16+1)){
						memcpy((char *)&displayline1[0][0], (char *)&tmpbuffer[indexvis],16);
					} else {
						memset(&displayline1[0][0],'\0',16);
					}
					indexvis++;
					if(indexvis>(len-16+2)){
						indexvis=0;
					}
				}
			}
		}
	}
	
	displaygest=1;
	WriteToDisplay();
	displaygest=0;
	return 0;
}