/* ========================================================================== */
/*                                                                            */
/*   main.c                                                                   */
/*   (c) 2011 Matteo Luca Scazzoli                                            */
/*                                                                            */
/*   Description                                                              */
/*   RiMMS 2014			                                                      */
/* ========================================================================== */

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

#include "sm.h"
#include "utility.h"
#include "main.h"
#include "rimms.h"
#include "display.h"
#include "protocolrimms.h"
#include "ftpclient.h"
#include "modem.h"
#include "keyboard.h"
#include "rimmsnet.h"

#define NSEC_PER_SEC    (1000000000) /* The number of nsecs per sec. */

//char *progname; //pointer to the name of the program
//char *path;
char pathstring[100];
unsigned char flagrebootsystem;
unsigned char flagpoweroffsystem;


typedef struct str_thdata
{
    int flagexit;
	int flagexit_comunication;
	int flagexit_io;
	int flagexit_RiMMS;
	int flagexit_modem;
	unsigned char InCharge;
	unsigned char LowBattery;
	unsigned char PowerIn;
	float adc_vin;
	float adc_vbatt;
	
}thdata;

static thdata data;

extern unsigned char debug,logftp;
unsigned char demon;

void thread_comunication(void *arg);
void thread_io(void *arg);
void thread_RiMMS(void *arg);
void thread_modem(void *arg);


static void killFromPidFile(void)
{
	FILE *fp;
	int pid;
    //char tmp[100];
    //char *ptr = &tmp[0];
        
    //strcpy((char *)tmp,(char *)pathstring);
    //strcat((char *)tmp,LOCK_FILE);

	fp = fopen(LOCK_FILE, "r");
	if (!fp){
		printf("unable to open %s : %s\n",LOCK_FILE, strerror(errno));
		exit(0);
	}
	if (fscanf(fp, "%i", &pid) != 1) {
		printf("unable to read the pid from file \"%s\"\n",LOCK_FILE);
        fclose(fp);
        exit(0);
	}
	fclose(fp);

	if (kill(pid, SIGTERM)) {
		printf("unable to kill proccess %i: %s\n", pid, strerror(errno));
	} else {
        printf("proccess %i STOPPED!!\n",pid);
    }
	exit(0);
}

int sig;
int signal_handler(sig)
{	
	switch(sig) {
		case SIGINT:
		case SIGHUP:
		case SIGSTOP:
		case SIGKILL:
		case SIGTERM:
			data.flagexit=0;
			return 0;
			break;
		case SIGALRM:
			return 0;
        break;
	}
	return -1;
}

typedef void (*sighandler_t)(int);
void daemonize()
{
	int i,lfp;
	char str[10];
	char tmp[100];
    char *ptr = &tmp[0];

    //strcpy(ptr,path);
    //strcat(ptr,LOCK_FILE);


	if(getppid()==1) return; /* already a daemon */
	i=fork();
	if (i<0) {
		printf("fork error\n\r");
		exit(1); /* fork error */
	}
	if (i>0) exit(0); /* parent exits */
	/* child (daemon) continues */
	setsid(); /* obtain a new process group */
	for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
	if(chdir(pathstring)!=0){
		exit(1); 
	}
	i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standart I/O */
	lfp=open(LOCK_FILE,O_RDWR|O_CREAT,0666);
	if (lfp<0) {
		printf("can not open LOCK_FILE\n\r");
		exit(1); /* can not open */
	}
	if (lockf(lfp,F_TLOCK,0)<0) {
		//printf("can not lock \n\r");
		exit(1); /* can not lock */
	}
	/* first instance continues */
	sprintf(str,"%d\n",getpid());
	write(lfp,str,strlen(str)); /* record pid to lockfile */
	close(lfp);
	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGHUP,(sighandler_t)signal_handler); /* catch hangup signal */
	signal(SIGTERM,(sighandler_t)signal_handler); /* catch kill signal */
    signal(SIGKILL,(sighandler_t)signal_handler);
    signal(SIGSTOP,(sighandler_t)signal_handler);
    signal(SIGINT,(sighandler_t)signal_handler);
}

int GetMacAddress(char *macstring)
{	char ptr[256];

	sprintf(ptr,"ifconfig wlan0 >> ramdisk/mac.txt");
	system(ptr);
	sprintf(ptr,"ramdisk/mac.txt");
	strcpy(macstring,ReadParamFile("HWaddr ",ptr));
    WriteLog("MAC: (%s)",macstring);	
	return 0;
}

int ParseCommandLine (int argc, char* argv[], unsigned char *debug, unsigned char *logftp, unsigned char *demon)
{
	int	i;

	*debug = 0;
	*logftp = 0;
	*demon = 1;
	for (i = 1; i < argc; i++)
	{
		if (!strcmp (argv [i], "--debug")){
			*debug = 1;
			printf("debug abilitato\n\r");
		} else if (!strcmp (argv [i], "--logftp")) {
			*logftp = 1;
			printf("log ftp abilitato\n\r");
		} else if (!strcmp (argv [i], "--nd")) {
			*demon = 0;
			printf("log ftp abilitato\n\r");	
		} else if (!strcmp (argv [i], "--kill")) {
			killFromPidFile();
		} else if (!strcmp (argv [i], "--ver")) {
			printf("%s\n\r",VER);
		} else {
			printf("error command\n\r");
		}
	}
	return 0;
}

int LoadConfig(void)
{
	char tmp[50];
	
	if(LoadParamDb("transferperiod",tmp)==0){
		transferperiod = atoi(tmp);
		if(transferperiod<=0){
			transferperiod=3600; //se non carico metto 1h
		}
		WriteLog("transferperiod %d",transferperiod);
	} else {
		WriteLog("Error loading transferperiod");
		return -1;
	}
	
	 
	if(LoadParamDb ("unitid",UnitName)!=0){
		WriteLog("Error loading unitid");
		return -1;
	} else {
		WriteLog("unitid %s",UnitName);
	}
	
	
	if(LoadParamDb("ftphost",ftphost)!=0){
		WriteLog("Error loading ftphost");
		return -1;
	} else {
		WriteLog("ftphost %s",ftphost);
	}
	
	if(LoadParamDb("ftplogin",ftplogin)!=0){
		WriteLog("Error loading ftplogin");
		return -1;
	} else {
		WriteLog("ftplogin %s",ftplogin);
	}
	
	if(LoadParamDb("ftppwd",ftppwd)!=0){
		WriteLog("Error loading ftppwd");
		return -1;
	} else {
		WriteLog("ftppwd %s",ftppwd);
	}
	
	if(LoadParamDb("ftpdir",ftpdir)!=0){
		WriteLog("Error loading ftpdir");
		return -1;
	} else {
		WriteLog("ftpdir %s",ftpdir);
	}
	
	if(LoadParamDb("ftpport",ftpport)!=0){
		WriteLog("Error loading ftpport");
		return -1;
	} else {
		WriteLog("ftpport %s",ftpport);
	}
	
	if(LoadParamDb("gwradioaddress",tmp)==0){
		Gw.Val = atoi(tmp);
		if(Gw.Val<=0){
			WriteLog("Error loading Gw.Val");
			return -1;
		} else {
			WriteLog("Gw.Val %d",Gw.Val);
		}
	} else {
		WriteLog("Error loading Gw.Val");
	}
	
	if(LoadParamDb("modemport",Modem.port)!=0){
		WriteLog("Error loading modemport");
		return -1;
	} else {
		WriteLog("modemport %s",Modem.port);
	}
	
	if(LoadParamDb("modemtype",tmp)==0){
		if(strcmp(tmp,"usb")==0){
			Modem.type=0;
			WriteLog("modemtype USB");
		} else if(strcmp(tmp,"tty")==0){
			Modem.type=1;
			WriteLog("modemtype TTY");
		} else {
			Modem.type=0;
			WriteLog("modemtype error: load USB type");
		}
	} else {
		WriteLog("Error loading modemtype");
		return -1;
	}
	
	/*
	if(LoadParamDb("wakeupnumber1",tmp)==0){
		strcpy(Modem.wakeupnumber[0],tmp);
	} else {
		strcpy(Modem.wakeupnumber[0],"----");
	}
	WriteLog("wakeupnumber1 %s",Modem.wakeupnumber[0]);
	
	if(LoadParamDb("wakeupnumber2",tmp)==0){
		strcpy(Modem.wakeupnumber[1],tmp);
	} else {
		strcpy(Modem.wakeupnumber[1],"----");
	}
	WriteLog("wakeupnumber2 %s",Modem.wakeupnumber[1]);
	
	if(LoadParamDb("wakeupnumber3",tmp)==0){
		strcpy(Modem.wakeupnumber[2],tmp);
	} else {
		strcpy(Modem.wakeupnumber[2],"----");
	}
	WriteLog("wakeupnumber3 %s",Modem.wakeupnumber[2]);
	*/
	
	if(LoadParamDb("keyboard",tmp)==0){
		if(strcmp(tmp,"enable")==0){
			Keyb.enable=1;
			WriteLog("keyboard enabled");
		} else if(strcmp(tmp,"disable")==0){
			Keyb.enable=0;
			WriteLog("keyboard disabled");
		} else {
			Keyb.enable=0;
			WriteLog("keyboard error: disabled");
		}
	} else {
		WriteLog("Error loading keyboard");
		return -1;
	}
	
	if(LoadParamDb("rimmsnetport",tmp)==0){
		RiMMSNet.port = atoi(tmp);
		if(RiMMSNet.port<=0){
			WriteLog("Error loading rimmsnetport");
			return -1;
		} else {
			WriteLog("rimmsnetport %d",RiMMSNet.port);
		}
	} else {
		WriteLog("Error loading rimmsnetport");
		return -1;
	}
	
	if(LoadParamDb("rimmsnetadd",tmp)==0){
		strcpy(RiMMSNet.address,tmp);
		WriteLog("rimmsnetadd %s",RiMMSNet.address);
	} else {
		WriteLog("Error loading rimmsnetport");
		return -1;
	}
	
	
	return 0;
}

int main(int argc, char *argv[])
{
    int i;
    int x,y;
	void *res;
	int ret;
	FILE *fp;
    
	pthread_t function_RiMMS,function_comunication,function_io,function_modem;
	pthread_attr_t my_attr;
	struct sched_param param_RiMMS, param_comunication, param_io,param_modem;

	
	
	fp = fopen(PATH_FILE, "r");
	if(fp==NULL){
		printf("unable to open the path file \"%s\"\n",PATH_FILE);
		exit(1);
	}
	if (fscanf(fp, "%s", pathstring) != 1) {
		printf("unable to read the path from file \"%s\"\n",PATH_FILE);
        fclose(fp);
        exit(1);
	}
	fclose(fp);
	
	//path = pathstring;
    
    debug = 0;
	logftp = 0;
	demon = 1;

    ParseCommandLine (argc, argv, &debug, &logftp,&demon);
    	
    if(demon){
		printf("Start daemon..PID:%d\n\r",getpid()+1);
		daemonize();
	} else {
		printf("NON AVVIATO COME DEMONE\n");
	}
    WriteLog("Avvio programma...");
	WriteLog("%s",VER);
	if(debug){
		WriteLog("Debug abilitato");
		WriteDebug("Avvio programma");
	}
	if(logftp){
		WriteLog("LOG FTP abilitato");		
	}

    if(OpenShadowMemory()==-1){
		return 1;
    }
	
	data.flagexit = 1;
	data.InCharge = 0;
	data.LowBattery=0;
	data.PowerIn=0;
	flagrebootsystem=0;
	flagpoweroffsystem=0;
	data.flagexit_comunication=0;
	data.flagexit_io=0;
	data.flagexit_RiMMS=0;
	data.flagexit_modem=0;
	data.adc_vin=0;
	data.adc_vbatt=0;
	
	if (init_memoryToIO()) {
        WriteLog("Error GPIO");
		return 1;
    } 
	
	if(lcdinit()!=0){
		WriteLog("Error init DISPLAY");
		pthread_exit(NULL);
	}
	
	if(LoadConfig()){
		WriteLog("Error Loading CONFIG");
		return 1;
	}
	
	if(InitDevices()!=0){
		WriteLog("Error init DEVICES");
		return 1;
	}
	
	if(InitModem()==-1){
		WriteLog("Error MODEM");
		return 1;
	}
	
	if(InitSerial()!=0){
		WriteLog("Error init SERIAL PORT");
		return 1;
	}
	
	if(InitRiMMS()!=0){
		WriteLog("Error init RiMMS");
		return 1;
	}
	
	if(InitKeyboard()!=0){
		WriteLog("Error init Keyboard");
		return 1;
	}
	
	if(InitRiMMSNet()!=0){
		WriteLog("Error init RiMMSNet");
		return 1;
	}
	
	pthread_attr_init(&my_attr);
	pthread_attr_setschedpolicy(&my_attr, SCHED_FIFO);
	
	param_RiMMS.sched_priority=1;
	param_comunication.sched_priority=2;
	param_modem.sched_priority=3;
	param_io.sched_priority=4;
	
	
	if(pthread_attr_setschedparam(&my_attr, &param_io)){
		WriteLog("error scheduling thread_io");
		return (1);
	}
	if ( pthread_create( &function_io, &my_attr, (void *) &thread_io, (void *) &data) ) {
		WriteLog("error creating thread_io");
		return (1);
	}
	
	if(pthread_attr_setschedparam(&my_attr, &param_modem)){
		WriteLog("error scheduling thread_modem");
		return (1);
	}
	if ( pthread_create( &function_modem, &my_attr, (void *) &thread_modem, (void *) &data) ) {
		WriteLog("error creating thread_modem");
		return (1);
	}
	
	if(pthread_attr_setschedparam(&my_attr, &param_RiMMS)){
		WriteLog("error scheduling thread_RiMMS");
		return (1);
	}
	if ( pthread_create( &function_RiMMS, &my_attr, (void *) &thread_RiMMS, (void *) &data) ) {
		WriteLog("error creating thread_RiMMS");
		return (1);
	}
	
	if(pthread_attr_setschedparam(&my_attr, &param_comunication)){
		WriteLog("error scheduling thread_comunication");
		return (1);
	}
	if ( pthread_create( &function_comunication, &my_attr, (void *) &thread_comunication, (void *) &data) ) {
		WriteLog("error creating thread_comunication");
		return (1);
	}
	
	///RIMANGO QUI FINCHE' I THREAD NON TERMINANO!!!
	while(data.flagexit){
		sleep(1);
	}
	
	x=0;
	while(data.flagexit_RiMMS){
		sleep(1);
		if(x++>=10){
			pthread_cancel(function_RiMMS);
			data.flagexit_RiMMS=0;
			WriteLog("function_RiMMS cancellato");
		}
	}
	ret = pthread_join ( function_RiMMS, &res );
	if(ret==0){
		WriteLog("thread_RiMMS terminato correttamente");
	} else {
		if (res == PTHREAD_CANCELED){
			WriteLog("thread_RiMMS CANCELLATO!!");
		} else {
			WriteLog("Errore nella terminazione del thread_RiMMS!!");
		}
	}
	
	x=0;
	while(data.flagexit_comunication){
		sleep(1);
		if(x++>=10){
			pthread_cancel(function_comunication);
			data.flagexit_comunication=0;
			WriteLog("function_comunication cancellato");
		}
	}
	ret = pthread_join ( function_comunication, &res );
	if(ret==0){
		WriteLog("thread_comunication terminato correttamente");
	} else {
		if (res == PTHREAD_CANCELED){
			WriteLog("thread_comunication CANCELLATO!!");
		} else {
			WriteLog("Errore nella terminazione del thread_comunication!!");
		}
	}
	
	x=0;
	while(data.flagexit_modem){
		sleep(1);
		if(x++>=10){
			pthread_cancel(function_modem);
			data.flagexit_modem=0;
			WriteLog("function_modem cancellato");
		}
	}
	ret = pthread_join ( function_modem, &res );
	if(ret==0){
		WriteLog("thread_modem terminato correttamente");
	} else {
		if (res == PTHREAD_CANCELED){
			WriteLog("thread_modem CANCELLATO!!");
		} else {
			WriteLog("Errore nella terminazione del thread_modem!!");
		}
	}
	
	x=0;
	while(data.flagexit_io){
		sleep(1);
		if(x++>=10){
			pthread_cancel(function_io);
			data.flagexit_io=0;
			WriteLog("function_io cancellato");
		}
	}
	ret = pthread_join ( function_io, &res );
	if(ret==0){
		WriteLog("thread_io terminato correttamente");
	} else {
		if (res == PTHREAD_CANCELED){
			WriteLog("thread_io CANCELLATO!!");
		} else {
			WriteLog("Errore nella terminazione del thread_io!!");
		}
	}
	
	pthread_attr_destroy(&my_attr);

			
	CloseDevices();
	CloseSerial();
	
	if(CloseShadowMemory()==0){
		WriteLog("Chiusura SHM corretta!");
	}
	if(data.flagexit==0){
		WriteLog("Chiusura programma CORRETTA!!");
	}else{
		WriteLog("Chiusura programma IMPREVISTA!!!");
	}
	
	system("poff"); //se ero connesso su gsm mi scollego
	
	WriteDisplay(0,0,"Centralina ferma");
	WriteDisplay(0,1,"                ");
	
	if(flagrebootsystem){
		WriteLog("..rebooting system");
		WriteDisplay(0,1,"Riavvio sistema ");
		system("reboot");
	} else
	if(flagpoweroffsystem){
		WriteLog("..poweroff system");
		WriteDisplay(0,1,"Arresto sistema ");
		system("shutdown -h now");
	}
	
	lcdclose();
	
	return 0;
}

int PowerOff (void)
{
	WriteLog("Comando poweroff system..");
	WriteDebug("Comando poweroff system..");
	flagpoweroffsystem=1;
	data.flagexit=0;		
	return 0;
}

int RebootSystem (void)
{
	WriteLog("Comando reboot system..");
	WriteDebug("Comando reboot system..");
	flagrebootsystem=1;
	data.flagexit=0;	
	return 0;
}

void thread_RiMMS(void *arg)
{
	thdata *data;            
    data = (thdata *) arg;  /* type cast to a pointer to thdata */

	data->flagexit_RiMMS=1;	
	
	WriteLog("start thread_RiMMS");
	
	while(data->flagexit){
		Parser();
		GestSMComunication();
		usleep(200000);
	}
	
	WriteLog("thread_RiMMS terminated");
	
	data->flagexit_RiMMS=0;	
	pthread_exit(NULL);
}

void thread_comunication(void *arg)
{
	int codice,i;
		
	thdata *data;            
    data = (thdata *) arg;  /* type cast to a pointer to thdata */
	
	data->flagexit_comunication=1;	
	WriteLog("start thread_comunication");
	
	Tag_SOR(0);
	Tag_POWER_OFF();
	
	while(data->flagexit){
		GestFtp();
		GestRiMMSNet();
		usleep(500000);
	}
	
	WriteLog("thread_comunication terminated");
	data->flagexit_comunication=0;	
	pthread_exit(NULL);
}

void thread_modem(void *arg)
{
	thdata *data;            
    data = (thdata *) arg;  /* type cast to a pointer to thdata */
	
	data->flagexit_modem=1;	
	WriteLog("start thread_modem");
	
	while(data->flagexit){
		GestModem();
		usleep(500000);
	}
	
	WriteLog("thread_modem terminated");
	data->flagexit_modem=0;	
	pthread_exit(NULL);
}

int ADC_Read(int channel){
	int fd,rtc;
	char filename[41];
	char buffer[5];
	
	if(channel==0){
		if ((fd=open(ADC0,O_RDONLY))<0) return -1;
	}
	if(channel==1){
		if ((fd=open(ADC1,O_RDONLY))<0) return -1;
	}
	
	read(fd,buffer,5);
	rtc = atoi(buffer);
	close(fd);

	return rtc;
}

int GetAlim(void)
{
	return data.PowerIn;
}

int GetChg(void)
{
	return data.InCharge;
}

float GetVin(void)
{
	return data.adc_vin;
}

float GetVbatt(void)
{
	return data.adc_vbatt;
}

void thread_io(void *arg)
{
	thdata *data;            
    data = (thdata *) arg;  /* type cast to a pointer to thdata */
	int filtroswitch,filtrocharge;
	static float adcvin,adcvbatt;
	float adc;
	
	//struct timespec t,mem;
    //int interval = 10000000; /* 10ms*/
	//int diff;
	
	data->flagexit_io=1;	
	
	WriteLog("start thread_io");
	
	setGpioinInput(SWITCH_ARIETTA);
	setGpioinInput(SWITCH_ARIETTA_EXT);
	
	WriteDisplayBuf("RiMMS 2014 VER %s",VER);
		
	system("echo 50 > /sys/class/leds/arietta_status/delay_on");
	system("echo 1950 > /sys/class/leds/arietta_status/delay_off");
	
	filtroswitch = 0;
	filtrocharge = 0;
	adcvin=400;
	adcvbatt=370;
	
	//clock_gettime(CLOCK_MONOTONIC,&mem);
	
	while(data->flagexit){
	
		usleep(50000);
		/* //calculate next shot 
        clock_gettime(CLOCK_MONOTONIC,&t);
		diff = t.tv_nsec - mem.tv_nsec;
		t.tv_nsec += (interval-diff);
        while(t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
            t.tv_sec++;
        }
		mem.tv_nsec=t.tv_nsec;
		mem.tv_sec=t.tv_sec;
		// wait until next shot 
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		*/
		
		//GESTIONE SWITCH
		if(!fastReadGpio(SWITCH_ARIETTA)||!fastReadGpio(SWITCH_ARIETTA_EXT)){
			filtroswitch++;
		} else {
			if(filtroswitch>=50){
				WriteLog("Richiesta arresto sistema da switch");
				PowerOff();
			} else if(filtroswitch>=30){
				WriteLog("Richiesta riavvio sistema da switch");
				RebootSystem();
			} else if(filtroswitch>0){
				WriteDebug("Premuto tasto %d",filtroswitch);
			}
			filtroswitch=0;
		}
		
		//LETTURA INGRESSO RICARICA BATTERIA
		if(!fastReadGpio(SWITCH_ARIETTA_CHG)){
			if(filtrocharge<300) filtrocharge++;
		} else {
			if(filtrocharge>0) filtrocharge--;
		}
		
		//LETTURA VIN
		adc = (float) ADC_Read(AD_VIN);
		adc *= 600;
		adc /=1024;
		adcvin +=adc;
		adcvin /=2;
		data->adc_vin=adcvin;
		if(adcvin>0){
			//WriteDebug("AD_VIN %05.2f",data->adc_vin);
			if(adcvin>420){
				if(data->PowerIn==0){
					data->PowerIn=1;
					WriteDebug("Alimentazione presente");
					WriteDisplayBuf("Rete presente");
					system("echo 50 > /sys/class/leds/arietta_status/delay_on");
					system("echo 950 > /sys/class/leds/arietta_status/delay_off");
				}
				if(filtrocharge>=300){
					if(data->InCharge==1){
						data->InCharge=0;
						WriteDebug("Batteria non in carica");
						WriteDisplayBuf("Carica terminata");
					}
				} else if(filtrocharge<=0){
					if(data->InCharge==0){
						data->InCharge=1;
						WriteDebug("Batteria in carica");
						WriteDisplayBuf("In carica");
					}
				}
			} else if(adcvin<400){
				if(data->PowerIn==1){
					data->PowerIn=0;
					Backlight(0);
					WriteDebug("Alimentazione non presente");
					WriteDisplayBuf("Rete assente");
					system("echo 50 > /sys/class/leds/arietta_status/delay_on");
					system("echo 2950 > /sys/class/leds/arietta_status/delay_off");
				}
				if(data->InCharge==1){
					data->InCharge=0;
					WriteDebug("Batteria non in carica");
					WriteDisplayBuf("Carica arrestata");
				}
			}
		} else {
			WriteLog("Error AD_VIN");
		}
		
		//LETTURA VBATT
		adc = (float) ADC_Read(AD_VBATT);
		adc *= 550;
		adc /=1024;
		adcvbatt+=adc;
		adcvbatt/=2;
		data->adc_vbatt=adcvbatt;
		if(adcvbatt>0){
			//WriteDebug("AD_VBATT %05.2f %05.2f",data->adc_vbatt,(float)GetVbatt());
			if(adcvbatt>380){
				if(data->LowBattery==1){
					data->LowBattery=0;
					WriteDebug("Batteria carica");
					WriteDisplayBuf("BATTERIA CARICA %05.2fV",(data->adc_vbatt/100));
				}
			} else if(adcvbatt<=325){
				if(data->LowBattery==0){
					data->LowBattery=1;
					WriteDebug("Batteria scarica");
					WriteDisplayBuf("BATTERIA SCARICA %05.2f",(data->adc_vbatt/100));
					system("echo 50 > /sys/class/leds/arietta_status/delay_on");
					system("echo 4950 > /sys/class/leds/arietta_status/delay_off");
				}
			}
		} else {
			WriteLog("Error AD_VBATT");
		}
		
		GestDisplay();
		GestKeyboard();
	}
	//SetDisplayGestOff();
	
	WriteDisplay(0,0,"arresto in corso");
	WriteDisplay(0,1,"attendere...    ");
	
	system("echo 500 > /sys/class/leds/arietta_status/delay_on");
	system("echo 500 > /sys/class/leds/arietta_status/delay_off");
	
	WriteLog("thread_io terminated");
	data->flagexit_io=0;	
	pthread_exit(NULL);
}
