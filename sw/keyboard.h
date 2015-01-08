#ifndef KEYBOARD_H 
#define KEYBOARD_H

struct _keyboard {
	unsigned char enable;
	unsigned char menulevel;
	unsigned char menuindex;
	unsigned char sw1,sw2,sw3,sw4;
	unsigned short filtersw1,filtersw2,filtersw3,filtersw4;
	int timeron;
};

extern struct _keyboard Keyb;

int InitKeyboard(void);
int GetKeybStatus(void);
int GestKeyboard(void);

#endif
