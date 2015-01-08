#include <stdarg.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <time.h> 
#include "utility.h"


unsigned char debug; 

int GetClockSecond(void)
{int ret,time;
 struct timespec res;

	ret=clock_gettime(CLOCK_MONOTONIC,&res);
	if(ret==0){
		time=res.tv_sec+(res.tv_nsec/1000000000);
	} else {
		return -1;
	}
	return time;	
}

int GetClockMsec(void)
{int ret,time;
 struct timespec res;

	ret=clock_gettime(CLOCK_MONOTONIC,&res);
	if(ret==0){
		time=(res.tv_sec*1000)+(res.tv_nsec/1000000);
	} else {
		return -1;
	}
	return time;	
}

char *ReadParamFile (char *parametro, char *file) {
	FILE *fpconf;
	char *line = NULL;
	char *position = NULL;
	char *param;
	int i;
       
    fpconf = fopen(file,"r");
	if(fpconf == NULL){
		WriteLog("Unable to open param file %s",file);
		return 0;
	}
    i=100;
	while(getline (&line, &i, fpconf)!= -1){// read config file line per line
      	position = strstr(line,parametro);
		if(position!=NULL){
			position+=strlen(parametro);
			param=strchr(position,' ')+1;
			if(strchr(param,'\n')){
				position = strchr(param,'\n');
				*position = 0;
			}
			if(strchr(param,'\r')){
				position = strchr(param,'\r');
				*position = 0;
			}
			//WriteDebug("ReadParamFile 3 (%s)",param);
			fclose(fpconf);
 			return param;
   		}
    }
  	fclose(fpconf);
	//WriteLog("ReadParamFile error searching parameter <%s> ",parametro);
  	return 0;
}

 WriteDebug(char *str,...) 
 {
    FILE *fp;
    char tmp[30];
    time_t mytime;
    struct tm *mytm;
    if(debug!=1){
     return 0;
    }

    fp = fopen(FILE_DEBUG,"a+");
    if (fp==NULL) return -1;
    mytime=time(NULL);
    mytm=localtime(&mytime);
    strftime(tmp,sizeof (tmp),"(%d%b%Y)%H:%M:%S",mytm);
    fprintf(fp,"%s: ",tmp);
    va_list arglist;
    va_start(arglist,str);
    vfprintf(fp,str,arglist);
    va_end(arglist);
    fprintf(fp,"\n");
    fclose(fp);
    return 1;
}

int WriteLog(char *str,...) {
    FILE *fp;
    char tmp[30];
    time_t mytime;
    struct tm *mytm;
    
    fp = fopen(FILE_LOG,"a+");
    if (fp==NULL) return -1;
    mytime=time(NULL);
    mytm=localtime(&mytime);
    strftime(tmp,sizeof (tmp),"(%d%b%Y)%H:%M:%S",mytm);
    fprintf(fp,"%s: ",tmp);
    va_list arglist;
    va_start(arglist,str);
    vfprintf(fp,str,arglist);
    va_end(arglist);
    fprintf(fp,"\n");
    fclose(fp);
    return 1;
}
