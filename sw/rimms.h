#ifndef RIMMS_H 
#define RIMMS_H

typedef union
{
    long Val;
	unsigned char b[4];
} DWORD_D;

typedef union
{
    short Val;
	unsigned char b[2];
} WORD_D;

int Tag_SOR (int timestamp);

#define DATA_FILE "ramdisk/RiMMS.dat" 
#define SAMPLE_FILE "sample.dat" 
#define SAMPLE_RAM "ramdisk/sample.dat" 
#define HEADER_FILE "header.dat"

#define TYPE_SENSORE		0x00
#define TYPE_ATTUATORE		0x01

#define MEASURE_BATTERY		0x01
#define	MEASURE_TEMPERATURE	0x02
#define	MEASURE_HUMIDITY	0x04

#define TAG_MEASURE_BATT	50
#define TAG_MEASURE_TEMP	51
#define TAG_MEASURE_HUMI	52

#define OVERFLOW_UP 0 
#define OVERFLOW_DWN 1 

extern int transferperiod,lasttransferperiod;
extern char UnitName[30];
extern char ftphost[50];
extern char ftplogin[50];
extern char ftppwd[50];
extern char ftpdir[50];
extern char ftpport[50];

int RemoveFile(char *file);
int RenameFile(const char *old, const char *new);
int RemoveFileToUpload(void);
int RemoveFileSample(void);
int WriteToFile(char *data, int Len);
int WriteToFile2(char *file, char *data, int Len);

int Tag_SOR (int timestamp);
int Tag_DATE_TIME_CHANGE(void);
int Tag_CURRENTSENSOR (long id);
int Tag_Batteria(long value);
int Tag_Temperatura(long value);
int Tag_Umidita(long value);
int Tag_SAMPLING_STARTED(void);
int Tag_SAMPLING_STOPPED(void);
int Tag_SAMPLE_TIME_CHANGED(unsigned short second);
int Tag_DOWNLOAD_STARTED(void);
int Tag_DOWNLOAD_COMPLETED(void);
int Tag_DOWNLOAD_FAILED(void);
int Tag_POWER_OFF(void);
int Tag_DATA_CLEARED(void);
int Tag_TAG_SENSORFAILURE (long id);
int Tag_NEWELEMENT (long id, unsigned int RadioAddress, char *Name, unsigned char type, unsigned char mask);
int Tag_THRESHOLD_OVERFLOW_ALERT(long id, unsigned char TagMeasure, unsigned char ud);
int Tag_ELEMENTREMOVED (long id);
int	Tag_MEMORY_FULL(void);
int Tag_STORAGE_CLEARED(void);

unsigned long file_size (char *file);
char *LoadFile(char *file, char *buffer, long *size);
long Flip (long data, long Bits);
int CRC32FromStream (char *p, long dwSize);
int CreateFileToUpload(void);
char *Get_UnitName(void);
int InitRiMMS (void);
int GetNameUnit(char *filetmp);
int ForceUpload(int sec);

int GestFtp (void);

#endif