#ifndef DB_H 
#define DB_H

int LoadParamDb (char *parametro, char *str);
int SaveParamDb (char *parametro, char *value);
int LoadDeviceDb (DEVICE *Dev);

char *GetSesorDbToFile(void);

#endif
