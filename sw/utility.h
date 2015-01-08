#ifndef UTILITY_H 
#define UTILITY_H

char *ReadConfFile (char *parametro); 
char *ReadParamFile (char *parametro, char *file);
int WriteLog(char *str,...); 
int WriteDebug(char *str,...); 
int WriteData(time_t mytime, char *str,...); 
int WritePresence(time_t mytime, char *str,...); 
int SaveImageIpCamera (void); 
int GetClockMsec(void);

extern unsigned char debug;

#define FILE_LOG "RiMMS.log" 
#define FILE_DEBUG "ramdisk/RiMMS.dbg"

#endif
