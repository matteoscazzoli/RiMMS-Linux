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
#include "zlib/include/zconf.h"
#include "zlib/include/zlib.h"
#include "protocolrimms.h"
#include "ftpclient.h"
#include "modem.h"
#include "main.h"
#include "command.h"


char  commandresponse[COMMAND_LEN_PARAM*COMMAND_LEN_PARAM];
char  commandparser[COMMAND_NUM_PARAM][COMMAND_LEN_PARAM];

char *ParserCommand (char *cmd)
{int lencmd;
 unsigned int commandparser_index,commandparser_punt;
	
	memset(commandparser, 0, sizeof(commandparser));
	memset(commandresponse, 0, sizeof(commandresponse));
	commandparser_index=0;
	commandparser_punt = 0;
	lencmd=strlen(cmd)+1;
	//WriteDebug("ParserCommand: %s",cmd);
	while(lencmd>0){
		if((*cmd==';')||(*cmd==',')){ //separatori parametro
			commandparser[commandparser_index][commandparser_punt]=0;
			commandparser_index++;
			commandparser_punt=0;
		}else if((*cmd=='\r')||(*cmd=='\n')||(*cmd==0)){
			commandparser[commandparser_index][commandparser_punt]=0;
			commandparser_index++;
			//WriteDebug("ParserCommand: param %i",commandparser_index);
			ExecCommand(commandparser_index);
			return commandresponse;
		}else{
			commandparser[commandparser_index][commandparser_punt++]=*cmd;
		}
		cmd++;
		lencmd--;
	}
	WriteDebug("ParserCommand: FAIL %X",*cmd);
	return 0;
}

int ExecCommand (int numparam)
{   DEVICE tmp;
	int ret;
	long temperatura;
	long umidita; 
	long batteria; 
	int lastcontact;
	unsigned char flagalive,flagtemp,flaghumi,flagbatt;
	int t1,t2;
	char temp[256];
	char *punt;
	
	if(numparam==0) return -1;
	if(strcmp(commandparser[0],"MODEM")==0){
		if(Modem.present){
			sprintf(commandresponse,"MODEM,%s,%d,%u\n",Modem.operatorname,Modem.signal,GetModemStatus());
		} else {
			sprintf(commandresponse,"MODEM,NOTPRESENT\n");
		}
	} else
	if(strcmp(commandparser[0],"PARAM")==0){
		if(numparam>=3){
			if(SaveParamDb(commandparser[1],commandparser[2])==0){
				if(LoadConfig()==0){
					sprintf(commandresponse,"PARAM,OK\n");
				} else {
					sprintf(commandresponse,"PARAM,WRONG\n");
				}
			} else {
				sprintf(commandresponse,"PARAM,DBERR\n");
			}
		} else {
			sprintf(commandresponse,"PARAM,ERR\n");
		}
	} else
	if(strcmp(commandparser[0],"SENSOR")==0){
		if(strcmp(commandparser[1],"LIST")==0){
			GetSensorId(temp);
			sprintf(commandresponse,"SENSOR,LIST%s\n",temp);
		} else
		if(strcmp(commandparser[1],"NAME")==0){
			if(numparam>=3){
				tmp.id.Val=atoi(commandparser[2]);
				if(GetSensorName(temp,tmp.id.Val)==1){
					sprintf(commandresponse,"SENSOR,NAME,%s\n",temp);
				} else {
					sprintf(commandresponse,"SENSOR,NAME,ERRID\n");
				}
			} else {
				sprintf(commandresponse,"SENSOR,NAME,PARAMERR\n");
			}
		} else
		if(strcmp(commandparser[1],"ADD")==0){
			if(numparam>=14){
				tmp.id.Val=atoi(commandparser[2]);
				tmp.add.Val=atoi(commandparser[3]);
				strcpy(tmp.name,commandparser[4]);
				tmp.sampleperiod=atoi(commandparser[5]);
				tmp.radioperiod=atoi(commandparser[6]);
				tmp.min_temperature=atoi(commandparser[7]);
				tmp.max_temperature=atoi(commandparser[8]);
				tmp.min_umidita=atoi(commandparser[9]);
				tmp.max_umidita=atoi(commandparser[10]);
				tmp.min_batteria=atoi(commandparser[11]);
				tmp.max_batteria=atoi(commandparser[12]);
				tmp.aliveperiod=atoi(commandparser[13]);
				if(AddDeviceDb(tmp)==0){
					if(InitDevices()==0){
						Tag_SOR(0);
						Tag_NEWELEMENT(tmp.id.Val,tmp.add.Val,(char *)tmp.name,TYPE_SENSORE,7);
						sprintf(commandresponse,"SENSOR,ADD,OK\n");
					} else {
						sprintf(commandresponse,"SENSOR,ADD,ERRINIT\n");
					}
				} else {
					sprintf(commandresponse,"SENSOR,ADD,DBERR\n");
				}
			} else {
				sprintf(commandresponse,"SENSOR,ADD,PARAMERR\n");
			}
		}else
		if(strcmp(commandparser[1],"UPD")==0){
			if(numparam>=14){
				tmp.id.Val=atoi(commandparser[2]);
				tmp.add.Val=atoi(commandparser[3]);
				strcpy(tmp.name,commandparser[4]);
				tmp.sampleperiod=atoi(commandparser[5]);
				tmp.radioperiod=atoi(commandparser[6]);
				tmp.min_temperature=atoi(commandparser[7]);
				tmp.max_temperature=atoi(commandparser[8]);
				tmp.min_umidita=atoi(commandparser[9]);
				tmp.max_umidita=atoi(commandparser[10]);
				tmp.min_batteria=atoi(commandparser[11]);
				tmp.max_batteria=atoi(commandparser[12]);
				tmp.aliveperiod=atoi(commandparser[13]);
				if(UpdateDeviceDb(tmp)==0){
					Tag_SOR(0);
					Tag_CURRENTSENSOR (tmp.id.Val);
				    Tag_SAMPLE_TIME_CHANGED(tmp.sampleperiod/10);
					if(InitDevices()==0){
						sprintf(commandresponse,"SENSOR,UPD,OK\n");
					} else {
						sprintf(commandresponse,"SENSOR,UPD,ERR\n");
					}
				} else {
					sprintf(commandresponse,"SENSOR,UPD,DBERR\n");
				}
			} else {
				sprintf(commandresponse,"SENSOR,UPD,PARAMERR\n");
			}
		}else
		if(strcmp(commandparser[1],"RMV")==0){
			if(numparam>=3){
				tmp.id.Val=atoi(commandparser[2]);
				if(RemoveDeviceDb(tmp)==0){
					if(InitDevices()==0){
						Tag_SOR(0);
						Tag_ELEMENTREMOVED(tmp.id.Val);
						sprintf(commandresponse,"SENSOR,RMV,OK\n");
					} else {
						sprintf(commandresponse,"SENSOR,RMV,ERR\n");
					}
				} else {
					sprintf(commandresponse,"SENSOR,RMV,DBERR\n");
				}
			} else {
				sprintf(commandresponse,"SENSOR,RMV,PARAMERR\n");
			}
		}else
		if(strcmp(commandparser[1],"GET")==0){
			if(numparam>=3){
				tmp.id.Val=atoi(commandparser[2]);
				ret=GetValueDevicesFromId(tmp.id.Val, &temperatura, &umidita, &batteria, &lastcontact ,&flagalive,&flagtemp,&flaghumi,&flagbatt);
				if(ret>=0){
					sprintf(commandresponse,"SENSOR,GET,%ld,%ld,%ld,%d,%u,%u,%u,%u\n",temperatura,umidita,batteria,lastcontact,flagalive,flagtemp,flaghumi,flagbatt);
				} else {
					sprintf(commandresponse,"SENSOR,GET,ERR\n");
				}
			} else {
				sprintf(commandresponse,"SENSOR,GET,PARAMERR\n");
			}
		}
		else {
			sprintf(commandresponse,"SENSOR,ERR\n");
		}
	} else
	if(strcmp(commandparser[0],"SYSTEM")==0){
		if(strcmp(commandparser[1],"FORCEUPDATE")==0){
			ForceUpload(5); 
			sprintf(commandresponse,"SYSTEM,FORCEUPDATE,OK\n");
		}else
		if(strcmp(commandparser[1],"GETUPDATE")==0){
			t1 = time(NULL)-(GetClockSecond()-lasttransferperiod);
			t2 = time(NULL)-(GetClockSecond()-(lasttransferperiod+transferperiod));
			sprintf(commandresponse,"SYSTEM,GETUPDATE,%d,%d,%d\n",t1,t2,(int)time(NULL));
		}else
		if(strcmp(commandparser[1],"PWR")==0){
			sprintf(commandresponse,"SYSTEM,PWR,%d,%d,%05.2f,%05.2f\n",GetAlim(),GetChg(),(float)(GetVin()/100),(float)(GetVbatt()/100));
		}else
		if(strcmp(commandparser[1],"REBOOT")==0){
			sprintf(commandresponse,"SYSTEM,REBOOT,OK\n");
			RebootSystem();
		}else
		if(strcmp(commandparser[1],"POWEROFF")==0){
			sprintf(commandresponse,"SYSTEM,POWEROFF,OK\n");
			PowerOff();
		}else
		if(strcmp(commandparser[1],"PROGRAMREBOOT")==0){
			sprintf(commandresponse,"SYSTEM,PROGRAMREBOOT,OK\n");
			system("/etc/init.d/RiMMS restart &");
		}else
		if(strcmp(commandparser[1],"STATUS")==0){
			if(strcmp(commandparser[2],"APP")==0){
				punt=GetAppStatus(punt);
				sprintf(commandresponse,"SYSTEM,STATUS,APP,%s\n",punt);
				free(punt);
			} else
			if(strcmp(commandparser[2],"MEM")==0){
				punt=GetMemStatus(punt);
				sprintf(commandresponse,"SYSTEM,STATUS,MEM,%s\n",punt);
				free(punt);
			} else
			if(strcmp(commandparser[2],"DISK")==0){
				punt=GetDiskStatus(punt);
				sprintf(commandresponse,"SYSTEM,STATUS,DISK,%s\n",punt);
				free(punt);
			} else {
				sprintf(commandresponse,"SYSTEM,STATUS,ERROR\n");
			}
		} else
		if(strcmp(commandparser[1],"NAME")==0){
			sprintf(commandresponse,"SYSTEM,NAME,%s\n",(char *)Get_UnitName());
		}
		else {
			sprintf(commandresponse,"SYSTEM,ERROR\n");
		}
	} else
	if(strcmp(commandparser[0],"PING")==0){
		sprintf(commandresponse,"PING,OK\n");
	} else
	if(strcmp(commandparser[0],"VER")==0){
		sprintf(commandresponse,"VER,%s,OK\n",VER);
	} else {
		WriteDebug("Cmd: ERROR");
		sprintf(commandresponse,"ERROR\n");
		return -2;
	}
	WriteDebug("Cmd: %s",commandresponse);
	return 0;
}

char *GetDiskStatus (char *file)
{
	char ptr[256];
	long size;
	
	sprintf(ptr,"df -h | grep /dev/ >> ramdisk/disk.txt");
	WriteDebug("GetDiskStatus: %s",ptr);
	system(ptr);
	file=(char *) LoadFile("ramdisk/disk.txt",file,&size);
	sprintf(ptr,"rm ramdisk/disk.txt");
	system(ptr);
	if(file){
		return file;
	}
	return 0;
}


char *GetMemStatus (char *file)
{
	char ptr[256];
	long size;
	
	sprintf(ptr,"free >> ramdisk/mem.txt");
	WriteDebug("GetMemStatus: %s",ptr);
	system(ptr);
	file=(char *) LoadFile("ramdisk/mem.txt",file,&size);
	sprintf(ptr,"rm ramdisk/mem.txt");
	system(ptr);
	if(file){
		return file;
	}
	return 0;
}

char *GetAppStatus (char *file)
{
	char ptr[256];
	long size;
	
	sprintf(ptr,"ps aux | grep RiMMS.exe | grep -v 'grep' >> ramdisk/app.txt");
	WriteDebug("GetAppStatus: %s",ptr);
	system(ptr);
	file=(char *) LoadFile("ramdisk/app.txt",file,&size);
	sprintf(ptr,"rm ramdisk/app.txt");
	system(ptr);
	if(file){
		return file;
	}
	return 0;
}
