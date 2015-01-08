#ifndef PROTOCOLRIMMS_H 
#define PROTOCOLRIMMS_H

#define TTY_PORT "/dev/ttyS1"
#define TTY_BAUD "19200"
#define MAXBUFFER 1024
#define MAX_BUFFERDATA 80

typedef struct {
	WORD_D id;
	WORD_D add;
	char name[30];
	unsigned int sampleperiod;
	unsigned int radioperiod;
	unsigned int aliveperiod;
	int min_temperature;
	int max_temperature;
	int min_umidita;
	int max_umidita;
	int min_batteria;
	int max_batteria;
	long lastidradio;
	int  lastcontact;
	int  lastalarm;
	unsigned char flagalalarmlive;
	unsigned char flagalalarmtemp;
	unsigned char flagalalarmhumi;
	unsigned char flagalalarmbatt;
	int temperature;
	int umidita;
	int batteria;
}DEVICE;

//extern DEVICE *Devices;

#define DEVICE_FILE "RiMMS.dev"
#define MAXDEVICE 500

extern WORD_D Gw;

int InitSerial(void);
int CloseSerial (void);
void LoadDataRf (int status);
void calc_sth11(void);
int GetValueDevicesFromId(unsigned short id, long *temperatura, long *umidita, long *batteria, int *lastcontact,unsigned char *flagalive,unsigned char *flagtemp,unsigned char *flaghumi,unsigned char *flagbatt);
int GetSensorId(char *string);
int GetSensorName(char *string,unsigned short id);

#define MYADDRESS 0x0001

#endif