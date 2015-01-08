#ifndef MODEM_H 
#define MODEM_H

int ModemRx (char *parametro);
int GetModemSignal(void);
int GestModem(void);

#define MODEM_NOTPRES	0
#define MODEM_NOTREG	1
#define MODEM_OFFLINE	2
#define MODEM_GOONLINE	3
#define MODEM_ONLINE	4
#define MODEM_GOOFFLINE 5

#define MODEMTYPE_USB 0
#define MODEMTYPE_TTY 1
#define MODEMTYPE_LAN 2

struct _modem {
	char present;
	unsigned char type;
	char port[50];
	unsigned char registered;
	unsigned char status;
	unsigned char desiredstatus;
	int timeronline;
	char operatorname[4];
	char callingnumber[25];
	char wakeupnumber[3][25];
	unsigned char signal;
	int timergest;
	int timercheck;
};

extern struct _modem Modem;

extern char ModemPort[50];
extern char redirhost[50];

char *GetModemOperator(void);

#endif