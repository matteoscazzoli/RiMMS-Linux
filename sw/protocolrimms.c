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
#include "db.h"

int ftty;
unsigned char protocolstate,protocolchecksum,protocolready;
unsigned char *buffer_serial;
unsigned int buffer_count;
unsigned int buffer_in;
unsigned int buffer_out;
WORD_D Device,Rp1,Rp2,Rp3,Gw;

unsigned char *bufferdata;
unsigned char bufferdatain;
unsigned char bufferdataout;
unsigned char bufferdatacount;

//DEVICE *Devices;
DEVICE Devices[MAXDEVICE];
unsigned int DeviceNumber,DeviceIndex;

long temperature,umidita,batteria;

int open_port(char *dev_name,char *speedstring)
{  struct termios options;
   
   ftty = open(dev_name, O_RDWR | O_NONBLOCK | O_NOCTTY | O_NDELAY);
   if (ftty != -1){
   	 /*
     * Get the current options for the port...
     */
     /*
	 struct sigaction saio;         // set the serial interrupt handler
     saio.sa_handler = LoadDataRf;  // to this function
     sigemptyset(&saio.sa_mask);    // clear existing settings
     saio.sa_flags = 0;             // make sure sa_flags is cleared
     saio.sa_restorer = NULL;       // no restorer
     sigaction(SIGIO, &saio, NULL); // apply new settings

     fcntl(ftty, F_SETOWN, getpid());      // enable our PID to receive serial interrupts
     fcntl(ftty, F_SETFL, FASYNC);
	 */

     tcgetattr(ftty, &options);
     bzero(&options, sizeof(options));

     /*
     * Set the baud rates to 19200...
     */
     //speedport= atoi(speedstring);
     if(strcmp(speedstring,"9600")==0){
      if(cfsetispeed(&options, B9600)==-1){
       return -1;
      }
      if(cfsetospeed(&options, B9600)==-1){
       return -1;
      }
      //printf("Serialport parameter  %s 9600",dev_name);
     } else
     if(strcmp(speedstring,"19200")==0){
      if(cfsetispeed(&options, B19200)==-1){
       return -1;
      }
      if(cfsetospeed(&options, B19200)==-1){
       return -1;
      }
      //printf("Serialport parameter  %s 19200\n\r",dev_name);
     } else {
      if(cfsetispeed(&options, B19200)==-1){
       return -1;
      }
      if(cfsetospeed(&options, B19200)==-1){
       return -1;
      }
      //printf("Serialport def parameter  %s 19200\n\r",dev_name);
     }
     options.c_cc[VMIN] = 0;      // 0 means use-vtime
     options.c_cc[VTIME] = 1;     // time to wait until exiting read (tenths of a second)

    /*
     * Enable the receiver and set local mode...
     */
     options.c_cflag |= (CLOCAL | CREAD);
     options.c_cflag &= ~PARENB;
     options.c_cflag &= ~CSTOPB;
     options.c_cflag &= ~CSIZE;
     options.c_cflag |= CS8;
     //options.c_cflag &= ~CNEW_RTSCTS;

     options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

     options.c_iflag &= ~(IXON | IXOFF | IXANY);

     options.c_oflag &= ~OPOST;

    /*
     * Set the new options for the port...
     */
     tcflush(ftty, TCIFLUSH);      // flush old data
     tcsetattr(ftty, TCSANOW, &options);

	 } else {
	 	return -1;
	 }
   //printf("Porta seriale %s\n\r",dev_name);
   return (ftty);
}

int InitSerial(void)
{
	int ret;
	
	buffer_count=0;
	buffer_in=0;
	buffer_out=0;
	buffer_serial = (unsigned char*) calloc(MAXBUFFER,sizeof(unsigned char));
	if(buffer_serial==NULL){
		WriteLog("Errore allocazione memoria buffer_serial");
		return -1;
	}
	
	bufferdatain = 0;
	bufferdataout = 0;
	bufferdatacount = 0;
	bufferdata = (unsigned char*) calloc(MAX_BUFFERDATA,sizeof(unsigned char));
	if(bufferdata==NULL){
		WriteLog("Errore allocazione memoria bufferdata");
		return -1;
	}

	
	protocolstate = 0;
	protocolchecksum = 0;
	protocolready = 0;
		
	ret=open_port(TTY_PORT,TTY_BAUD);
	if(ret == -1){
		WriteLog("Errore apertura porta seriale: %s", strerror(errno));
		return -1;
	}
	return 0;
}

int CloseSerial (void)
{
	close(ftty);
	free(buffer_serial);
	free(bufferdata);
	return 0;
}


int InitDevices(void)
{	int i,ret;
	
	DeviceNumber = 0;
	DeviceIndex = 0;
	DeviceNumber = LoadDeviceDb(Devices);
	WriteDebug("Caricati %u devices",DeviceNumber);
	for(i=0; i<DeviceNumber;i++){
		if(strlen(Devices[i].name)==0){
			sprintf(Devices[i].name,"Uknown %i",i);
		}
		if(Devices[i].sampleperiod<10){
			Devices[i].sampleperiod=10;
		}
		if(Devices[i].radioperiod<10){
			Devices[i].radioperiod=10;
		}
		if(Devices[i].aliveperiod<=Devices[i].radioperiod){
			Devices[i].aliveperiod=(Devices[i].radioperiod*2);
		}		
		Devices[i].sampleperiod/=10;
		Devices[i].radioperiod/=10;
		//Devices[i].lastidradio = 0;
		Devices[i].lastcontact = time(NULL);
		Devices[i].lastalarm = time(NULL);
		Devices[i].flagalalarmlive=0;
		Devices[i].flagalalarmtemp=0;
		Devices[i].flagalalarmhumi=0;
		Devices[i].flagalalarmbatt=0;
		WriteDebug("InitDevices: %u %u %s %u %u",Devices[i].id.Val,Devices[i].add.Val,Devices[i].name,Devices[i].sampleperiod,Devices[i].radioperiod);
	}
	return 0;
}

int CloseDevices(void)
{
	return 0;
}

int FindDevicesFromAdd(unsigned short add)
{int i;
	
	for(i=0; i<DeviceNumber;i++){
		if(Devices[i].add.Val==add){
			return i;
		}
	}
	return -1;
}

int FindDevicesFromId(unsigned short id)
{int i;
	
	for(i=0; i<DeviceNumber;i++){
		if(Devices[i].id.Val==id){
			return i;
		}
	}
	return -1;
}

int GetSensorId(char *string)
{char tmp[10];
 unsigned int i;

	for(i=0; i<DeviceNumber;i++){
		sprintf(tmp,",%i",Devices[i].id.Val);
		strcat(string,tmp);
	}
	return DeviceNumber;
}

int GetSensorName(char *string,unsigned short id)
{unsigned int i;

	for(i=0; i<DeviceNumber;i++){
		if(Devices[i].id.Val==id){
			sprintf(string,"%s",Devices[i].name);
			return 1;
		}
	}
	return 0;
}

int GetValueDevicesFromId(unsigned short id, long *temperatura, long *umidita, long *batteria, int *lastcontact,unsigned char *flagalive,unsigned char *flagtemp,unsigned char *flaghumi,unsigned char *flagbatt)
{int i;

	for(i=0; i<DeviceNumber;i++){
		if(Devices[i].id.Val==id){
			*temperatura = Devices[i].temperature;
			*umidita  = Devices[i].umidita;
			*batteria = Devices[i].batteria;
			*lastcontact = Devices[i].lastcontact;
			*flagalive = Devices[i].flagalalarmlive;
			*flagtemp = Devices[i].flagalalarmtemp;
			*flaghumi = Devices[i].flagalalarmhumi;
			*flagbatt = Devices[i].flagalalarmbatt;
			return i;
		}
	}
	return -1;
}

void LoadDataRf (int status)
{	int readchar,i;
	unsigned char temp[1024];

    readchar = read(ftty,temp,1024);
	for(i=0;i<readchar;i++){
		if(buffer_count<MAXBUFFER){
			buffer_serial[buffer_in]=temp[i];
			buffer_in++;
			if(buffer_in>=MAXBUFFER){
				buffer_in=0;
			}
			buffer_count++;
		}
	}
	return;
}

int GetChar(unsigned char *c)
{
	if(buffer_count){
		*c = buffer_serial [buffer_out];
		buffer_out++;
		if(buffer_out>=MAXBUFFER){
			buffer_out=0;
		}
		buffer_count--;
	} else {
		return -1;
	}
	return 0;
}



int GetBufferData(unsigned char *data)
{
	if(bufferdatacount){
		*data=bufferdata[bufferdataout];
		bufferdataout++;
		if(bufferdataout>MAX_BUFFERDATA){
			bufferdataout = 0;
		}
		bufferdatacount--;
		return 0;
	} else {
		bufferdataout=0;
		return -1;
	}
}

int ParserData(unsigned short posmem)
{unsigned char data,i;
 int Tmp;
 DWORD_D timestamp,sample;
 WORD_D	 sampleperiod;

	
	if(GetBufferData(&data)==0){
		if(data==0x01){
			//carico timestamp misure
			GetBufferData(&timestamp.b[0]);
			GetBufferData(&timestamp.b[1]);
			GetBufferData(&timestamp.b[2]);
			GetBufferData(&timestamp.b[3]);
			//carico sampleperiod misure
			GetBufferData(&sampleperiod.b[0]);
			GetBufferData(&sampleperiod.b[1]);
			sampleperiod.Val *= 10;
			//verifico maschera misure
			GetBufferData(&data);
			WriteDebug("M:%d",data);
			for(i=0; i<6; i++){
				GetBufferData(&sample.b[0]);
				GetBufferData(&sample.b[1]);
				GetBufferData(&sample.b[2]);
				GetBufferData(&sample.b[3]);
				if(data=0x07){
					temperature = sample.b[0]+(sample.b[1]*256);
					umidita = sample.b[2];
					batteria = sample.b[3];
					if(batteria>0){
						batteria*=100;
						batteria/=255;
					}
					if(umidita!=0 && temperature!=0){
						calc_sth11();
					}
				} else if(data=0x03){
					temperature = sample.b[0]+(sample.b[1]*256); //recupero valore temperatura
					umidita = 0;
					batteria = sample.b[3];
					if(batteria>0){
						batteria*=100;
						batteria/=255;
					}
				} else {
					WriteDebug("Maschera sensore sconosciuta %u",data);
					return -1;
				}
				if(sample.Val){
					WriteDebug("TIME:%lu T:%ld H:%ld B:%ld",timestamp.Val+(i*sampleperiod.Val),temperature,umidita,batteria);
					if(timestamp.Val<=1000){
						WriteDebug("Misura non valida");
					} else {
						Devices[posmem].lastcontact = time(NULL);
						if(Devices[posmem].lastidradio!=timestamp.Val){
							Tag_SOR(0);
							Tag_CURRENTSENSOR(Devices[posmem].id.Val);
							Tag_SOR(timestamp.Val+(i*sampleperiod.Val));
							Tag_Batteria(batteria);
							Tag_SOR(timestamp.Val+(i*sampleperiod.Val));
							Tag_Temperatura(temperature);
							Tag_SOR(timestamp.Val+(i*sampleperiod.Val));
							Tag_Umidita(umidita);
							Devices[posmem].batteria=batteria;
							Devices[posmem].temperature=temperature;
							Devices[posmem].umidita=umidita;
							WriteDebug("Sample saved!");
						} else {
							WriteDebug("Dumping sample");
						}
					}
				}
			}
			Devices[posmem].lastidradio = timestamp.Val;		
			
		}
	}
	return 0;
}

int SendAckSensor(unsigned short add,unsigned short sampleperiod,unsigned short radioperiod)
{unsigned char Chksum,i,x,buffer[44];
 DWORD_D timestamp;
 WORD_D sample,radio,devadd;
 int ret;
 //char temp[512];
	
	i=0;
	timestamp.Val=time(NULL);
	Chksum=0;
	devadd.Val=add;
	sample.Val = sampleperiod;
	radio.Val = radioperiod;
	
	buffer[i++]=':';
	buffer[i++]=devadd.b[0];
	buffer[i++]=devadd.b[1];
	buffer[i++]=Rp1.b[0];
	buffer[i++]=Rp1.b[1];
	buffer[i++]=Rp2.b[0];
	buffer[i++]=Rp2.b[1];
	buffer[i++]=Rp3.b[0];
	buffer[i++]=Rp3.b[1];
	buffer[i++]=Gw.b[0];
	buffer[i++]=Gw.b[1];
	buffer[i++]=0x02; //tipo pacchetto
	buffer[i++]=timestamp.b[0];
	buffer[i++]=timestamp.b[1];
	buffer[i++]=timestamp.b[2];
	buffer[i++]=timestamp.b[3];
	buffer[i++]=sample.b[0];
	buffer[i++]=sample.b[1];
	buffer[i++]=radio.b[0];
	buffer[i++]=radio.b[1];
	for(x=0;x<23;x++){
		buffer[i++]=0x00;
	}
	for(x=0;x<i;x++){
		Chksum+=buffer[x];
	}
	buffer[i++]=Chksum;
	/*temp[0]=0;
	for(x=0;x<i;x++){
		sprintf(&temp[strlen(temp)],"%X ",buffer[x]);
	}
	WriteDebug("SendAckSensor %u -> %s",i,temp);*/
	ret = write(ftty,buffer,i);
	WriteDebug("SendAckSensor %u %d",i,ret);
	return 0;
}

int CheckSensorThresholds (long id)
{
	//controllo le soglie temperatura
	if(Devices[id].min_temperature!=0 && Devices[id].max_temperature!=0){ //ho valori validi ed eseguo controllo
		if(Devices[id].temperature<Devices[id].min_temperature){
			if(Devices[id].flagalalarmtemp!=1){
				Devices[id].flagalalarmtemp=1;
				Tag_SOR(0);
				Tag_THRESHOLD_OVERFLOW_ALERT(Devices[id].id.Val,TAG_MEASURE_TEMP,OVERFLOW_DWN);
				ForceUpload(5);
			}
		} else 
		if(Devices[id].temperature>Devices[id].max_temperature){
			if(Devices[id].flagalalarmtemp!=2){
				Devices[id].flagalalarmtemp=2;
				Tag_SOR(0);
				Tag_THRESHOLD_OVERFLOW_ALERT(Devices[id].id.Val,TAG_MEASURE_TEMP,OVERFLOW_UP);
				ForceUpload(5);
			}
		} else {
			Devices[id].flagalalarmtemp=0;
		}
	}
	//controllo le soglie umidita
	if(Devices[id].min_umidita!=0 && Devices[id].max_umidita!=0){ //ho valori validi ed eseguo controllo
		if(Devices[id].umidita<Devices[id].min_umidita){
			if(Devices[id].flagalalarmhumi!=1){
				Devices[id].flagalalarmhumi=1;
				Tag_SOR(0);
				Tag_THRESHOLD_OVERFLOW_ALERT(Devices[id].id.Val,TAG_MEASURE_HUMI,OVERFLOW_DWN);
				ForceUpload(5);
			}
		} else 
		if(Devices[id].umidita>Devices[id].max_umidita){
			if(Devices[id].flagalalarmhumi!=2){
				Devices[id].flagalalarmhumi=2;
				Tag_SOR(0);
				Tag_THRESHOLD_OVERFLOW_ALERT(Devices[id].id.Val,TAG_MEASURE_HUMI,OVERFLOW_UP);
				ForceUpload(5);
			}
		} else {
			Devices[id].flagalalarmhumi=0;
		}
	}
	//controllo le soglie batteria
	if(Devices[id].min_batteria!=0 && Devices[id].max_batteria!=0){ //ho valori validi ed eseguo controllo
		if(Devices[id].batteria<Devices[id].min_batteria){
			if(Devices[id].flagalalarmbatt!=1){
				Devices[id].flagalalarmbatt=1;
				Tag_SOR(0);
				Tag_THRESHOLD_OVERFLOW_ALERT(Devices[id].id.Val,TAG_MEASURE_BATT,OVERFLOW_DWN);
				ForceUpload(5);
			}
		} else 
		if(Devices[id].batteria>Devices[id].max_batteria){
			if(Devices[id].flagalalarmbatt!=2){
				Devices[id].flagalalarmbatt=2;
				Tag_SOR(0);
				Tag_THRESHOLD_OVERFLOW_ALERT(Devices[id].id.Val,TAG_MEASURE_BATT,OVERFLOW_UP);
				ForceUpload(5);
			}
		} else {
			Devices[id].flagalalarmbatt=0;
		}
	}
	return 0;
}

int Parser (void)
{unsigned char data;
 int ret;
 //static int deviceindexlocal;

	
	LoadDataRf(0);
	while(GetChar(&data)==0){
		//WriteDebug("Parser %X",data);
		//sprintf(&temp[strlen(temp)],"%X ",data);
		switch(protocolstate){
			case 0: if(data==':'){
						protocolstate++;
						protocolchecksum = 0;
						protocolready = 0;
						bufferdatain = 0;
						bufferdataout = 0;
						bufferdatacount = 0;
					}
					break;
			case 1: if(data==Gw.b[0]){
						protocolstate++;
					} else {
						protocolstate=0;
						WriteDebug("Data gw bad!");
					}
					break;
			case 2: if(data==Gw.b[1]){
						protocolstate++;
					} else {
						protocolstate=0;
						WriteDebug("Data gw bad!");
					}
					break;
			case 3: Rp3.b[0]= data;
					protocolstate++;
					break;
			case 4: Rp3.b[1]= data;
					protocolstate++;
					break;
			case 5: Rp2.b[0]= data;
					protocolstate++;
					break;
			case 6: Rp2.b[1]= data;
					protocolstate++;
					break;
			case 7: Rp1.b[0]= data;
					protocolstate++;
					break;
			case 8: Rp1.b[1]= data;
					protocolstate++;
					break;
			case 9: Device.b[0] = data;
					protocolstate++;
					break;
			case 10: Device.b[1] = data;
					protocolstate++;
					break;
			case 43:if(data== protocolchecksum){
						protocolready = 1;
						//WriteDebug("Parser %u-> %s",protocolstate,temp);
						ret=FindDevicesFromAdd(Device.Val);
						if(ret>=0){
							SendAckSensor(Devices[ret].add.Val,Devices[ret].sampleperiod,Devices[ret].radioperiod);
							ParserData(ret);
							CheckSensorThresholds(ret);
							WriteDebug("Ricevuto %u %u %u %u %s",ret,Device.Val,Devices[ret].add.Val,Devices[ret].id.Val,Devices[ret].name);
							WriteDisplayBuf("Ricevuto sens %s %03.1fC %03.1f%% %03.1fV",Devices[ret].name,(float)Devices[ret].temperature/10,(float)Devices[ret].umidita/10,(float)Devices[ret].batteria/10);
							Devices[ret].lastalarm = time(NULL);
							Devices[ret].flagalalarmlive=0;
						} else {
							WriteDisplayBuf("Ricevuto sens sconosciuto %u",Device.Val);
							SendAckSensor(Device.Val,10,20);
						}
						
					} else {
						WriteDebug("Data checksum error");
					}
					protocolstate = 0;
					break;
			default:if(bufferdatacount<MAX_BUFFERDATA){
						bufferdata[bufferdatain]=data;
						bufferdatain++;
						if(bufferdatain>=MAX_BUFFERDATA){
							bufferdatain=0;
						}
						bufferdatacount++;
					}
					protocolstate++;
					break;
		}
		protocolchecksum+=data;
		//WriteDebug("0X%X stato:%u",data,protocolstate);
	}
	
	//controllo alivesensor
	if((time(NULL)-Devices[DeviceIndex].lastalarm)>Devices[DeviceIndex].aliveperiod){
		Devices[DeviceIndex].lastalarm = time(NULL);
		Devices[DeviceIndex].flagalalarmlive=1;
		Tag_SOR(0);
		Tag_TAG_SENSORFAILURE(Devices[DeviceIndex].id.Val);
		ForceUpload(5);
	} 
	DeviceIndex++;
	if(DeviceIndex>=DeviceNumber){
		DeviceIndex=0;
	}
	return;
}

void calc_sth11(void)
{
  const long  d1 = -4000; // *100  //°C  @ 5V
  const long  d2 =  4;    // *100  //°C  @ 12 bit
  const long c1 =  -40;   //*10          @ 8 bit
  const long c2 =   648;  //*1000        @ 8 bit
  const long c3 =  -72;   //*100000      @ 8 bit
  const long t1 =   10;   //*1000        @ 8 bit
  const long t2 =   128;  //*100000      @ 8 bit

  long u1,u2;
  long rh_lin;
  long rh_true;
  
  //conversione temperatura in decimi di °C
  temperature *= d2;
  temperature += d1;
  temperature /= 10;
  //t_C = d1 + (d2 * temperature);
  
  //conversione umidità assoluta in decimi
  u1 = umidita;
  u1 *= u1;
  u1 *= c3;
  u1 /= 10000;
  
  u2 = umidita;
  u2 *= c2;
  u2 /= 100;
  
  rh_lin = c1;
  rh_lin += u1;
  rh_lin += u2;
  //rh_lin = c1 + (c2 * rh) + (c3 * rh * rh);
  
  //conversione umidità relativa
  u1 = umidita;
  u1 *= t2;
  u1 /= 100;
  u1 += t1;
  u2 = temperature;
  u2 -= 250;
  rh_true = u2;
  rh_true *= u1;
  rh_true /=1000;
  rh_true += rh_lin;
  //rh_true = (t_C - 25) * (t1 + (t2 * rh)) + rh_lin;
  
  if(rh_true > 1000) rh_true = 1000;
  if(rh_true < 0)    rh_true = 0;

  umidita     = rh_true;
}
