#ifndef MAIN_H 
#define MAIN_H

#define VER "0.4 RT 17/12/2014"


#define LOCK_FILE "RiMMS.lock"
#define PATH_FILE "RiMMS.path"


#define  SWITCH_ARIETTA 17
#define  SWITCH_ARIETTA_CHG 0
#define  SWITCH_ARIETTA_EXT 1
#define  ALIM_MODEM	2
#define  MDM_POWERKWY 3
#define  MDM_RESET 4
#define  MDM_STATUS 28
#define  MDM_NETLIGHT 31


#define ADC0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define AD_VBATT 0
#define ADC1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define AD_VIN 1

int GetAlim(void);
int GetChg(void);
float GetVin(void);
float GetVbatt(void);

#endif
