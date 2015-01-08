#ifndef COMMAND_H 
#define COMMAND_H

#define COMMAND_BUFFER_LEN 2048
#define COMMAND_NUM_PARAM 16
#define COMMAND_LEN_PARAM 128

char *ParserCommand (char *cmd);
int ExecCommand (int numparam);
char *GetDiskStatus (char *file);
char *GetMemStatus (char *file);
char *GetAppStatus (char *file);

#endif
