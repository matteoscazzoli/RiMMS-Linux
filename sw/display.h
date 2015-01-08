#ifndef DISPLAY_H 
#define DISPLAY_H

// Display I2C address
#define I2C_ADDR_DISPLAY 0x3E

// Expander I2C address
#define I2C_ADDR_EXP 0x3F	// PCF8574AT
#define KEY1 1
#define KEY2 2
#define KEY3 3
#define KEY4 4

#define MAXDISPLAYBUFFER 1024

int lcdinit(void);
int WriteDisplayFromKeyb(unsigned char x,unsigned char y,char *str,...);

#endif