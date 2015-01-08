/* ========================================================================== */
/*                                                                            */
/*   sm.h                                                                     */
/*   (c) 2011 Matteo Luca Scazzoli                                            */
/*                                                                            */
/*   Description                                                              */
/*   manage shadow memory                                                     */
/* ========================================================================== */
#ifndef SM_H
#define SM_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#define SHMSZ 2048
#define KEYTX 0x9877
#define KEYRX 0x9878


int shmidtx,shmidrx;

key_t keytx,keyrx;
char *shmtx,*shmrx;
char *shmrx_mem;

int OpenShadowMemory();
int CloseShadowMemory();
int WriteSM(char *str,...);
int ReadSM(char *str);

#endif

