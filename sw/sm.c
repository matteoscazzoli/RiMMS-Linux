/* ========================================================================== */
/*                                                                            */
/*   sm.c                                                                     */
/*   (c) 2011 Matteo Luca Scazzoli                                            */
/*                                                                            */
/*   Description                                                              */
/*   manage shadow memory                                                     */
/* ========================================================================== */

#include <unistd.h>
#include <linux/reboot.h>

#include "sm.h"
#include "utility.h"

char bufferSM_rx[SHMSZ];
unsigned int  bufferSMcount_rx;

int OpenShadowMemory()
{int i;
  
  keytx = KEYTX;
  if ((shmidtx = shmget(keytx, SHMSZ, IPC_CREAT | 0666)) < 0) {
        WriteLog("Errore apertura Shadow Memory tx web");
        return -1;
  }
  if ((shmtx = shmat(shmidtx, NULL, 0)) == (char *) -1) {
     WriteLog("Error attach data to shadow memory tx web");
     return -1;
  }
  
  keyrx = KEYRX;
  if ((shmidrx = shmget(keyrx, SHMSZ, IPC_CREAT | 0666)) < 0) {
        WriteLog("Errore apertura Shadow Memory rx web");
        return -1;
  }
  if ((shmrx = shmat(shmidrx, NULL, 0)) == (char *) -1) {
     WriteLog("Error attach data to shadow memory rx web");
     return -1;
  }

  for(i=0;i<SHMSZ;i++){
   shmtx[i]=0;
   shmrx[i]=0;
   bufferSM_rx[0];
  }
  
  bufferSMcount_rx=0;
  
  WriteLog("Apertura shadow memory ok");
  
  
  return 0;
}

int CloseShadowMemory()
{int rettx,retrx;

	if(shmdt(shmtx)==0){
		if(shmctl(shmidtx, IPC_RMID, NULL)==0){
			rettx = 0;
		} else {
			WriteLog("Errore chiusura shadow tx memory");
			rettx = -1;
		}
	} else {
		WriteLog("Errore chiusura shadow tx memory");
		rettx=-1;
	}
	if(shmdt(shmrx)==0){
		if(shmctl(shmidrx, IPC_RMID, NULL)==0){
			retrx = 0;
		} else {
			WriteLog("Errore chiusura shadow rx memory");
			retrx = -1;
		}
	} else {
		WriteLog("Errore chiusura shadow rx memory");
		retrx=-1;
	}
	
	if((rettx==0) && (retrx==0)){
		return 0;
	} else {
		return -1;
	}
}

int WriteSM(char *str,...)
{int i;
	
    for(i=0;i<SHMSZ;i++){
     shmtx[i]=0;
    }
    va_list arglist;
    va_start(arglist,str);
    vsprintf(shmtx,str,arglist);
    va_end(arglist);
    
    return 1;
}

int GestSMComunication (void)
{char *retstr;
 
	while(shmrx[bufferSMcount_rx]!=0){ //ho un carattere valido in memoria?
		//WriteDebug("SM:(%X)(%c)(%u)",shmrx[bufferSMcount_rx],shmrx[bufferSMcount_rx],bufferSMcount_rx);
		if(bufferSMcount_rx<SHMSZ){
			if((shmrx[bufferSMcount_rx]=='\n')||(shmrx[bufferSMcount_rx]=='\r')){	//ricevuto fine comando: lo eseguo!
				bufferSM_rx[bufferSMcount_rx]=0;
				retstr=(char *)ParserCommand((char *)bufferSM_rx);
				//WriteDebug("SM:(%s)ret(%s)",bufferSM_rx,retstr);
				shmrx[bufferSMcount_rx]=0;
				bufferSMcount_rx=0;
				if(retstr!=NULL){
					WriteSM("%s",retstr);
					return 1;
				} else {
					return -1;
				}
			} else {
				bufferSM_rx[bufferSMcount_rx]=shmrx[bufferSMcount_rx];
				shmrx[bufferSMcount_rx]=0;
			}
			bufferSMcount_rx++;
		} else {
			bufferSMcount_rx=0;
			return -1;
		}	
	}
	return 0;
 }
 
/*
int GestSMComunication (void)
{char *retstr;
 int i;

	i=0;
	if(shmrx[0]==0)return 0;
	while(shmrx[i]!=0){ //ho un carattere valido in memoria?
		//WriteDebug("SM:(%c)(%i)",shmrx[i],i);
		if(bufferSMcount_rx<SHMSZ){
			bufferSM_rx[bufferSMcount_rx++]=shmrx[i];
			if((shmrx[i]=='\n')||(shmrx[i]=='\r')){	//ricevuto fine comando: lo eseguo!
					bufferSM_rx[bufferSMcount_rx]=0;
					retstr=(char *)ParserCommand((char *)bufferSM_rx);
					//WriteDebug("SM:ret(%s)",retstr);
					if(retstr!=NULL){
						WriteSM("%s",retstr);
						return 1;
					} else {
						return -1;
					}
			}
		} else {
			bufferSMcount_rx=0;
			return -1;
		}
		shmrx[i]=0;
		//i++;
	}
	bufferSMcount_rx=0;
	return 0;
}
*/
