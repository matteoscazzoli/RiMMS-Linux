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
#include "main.h"
#include "keyboard.h"


struct _keyboard Keyb;

int InitKeyboard(void)
{
	Keyb.menulevel=0;
	Keyb.menuindex=0;
	Keyb.filtersw1=0;
	Keyb.filtersw2=0;
	Keyb.filtersw3=0;
	Keyb.filtersw4=0;
	return 0;
}

int GetKeybStatus(void)
{
	if(Keyb.menulevel>0){
		return 1;
	}
	return 0;
}

int GestKeyboard(void)
{
	if(Keyb.enable==0) return 0;
	
	return 1;
}