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
#include <sqlite3.h>

#include "sm.h"
#include "utility.h"
#include "main.h"
#include "rimms.h"
#include "display.h"
#include "protocolrimms.h"
#include "db.h"

typedef char ROWDB[14][128];
struct DBTABLE {
	int numrow;
	ROWDB datarow[100];
}Db;

static int callback(void *data, int argc, char **argv, char **azColName){
	int i;
	for(i=0; i<argc; i++){
		if(argv[i]){
			strcpy(Db.datarow[Db.numrow][i],argv[i]);
		} else {
			strcpy(Db.datarow[Db.numrow][i],"");
		}
	}
	Db.numrow++;
	return 0;
}


int LoadParamDb (char *parametro, char *str)
{  	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[128];
	char *data;
   
	/* Open database */
	rc = sqlite3_open("RiMMS.sql3", &db);
	if( rc ){
		WriteLog("LoadParamDb: Can't open database: %s", sqlite3_errmsg(db));
		return -1;
	}
   
	/* Create SQL statement */
	sprintf(sql,"SELECT * FROM config WHERE parametro=\"%s\"",parametro);
	//WriteDebug("LoadParamDb: SQL %s",sql);
   
   Db.numrow=0;
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		WriteLog("SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return -2;
	}
	WriteDebug("LoadParamDb: (%s)",Db.datarow[0][1]);
	sqlite3_close(db);
	strcpy(str,Db.datarow[0][1]);
	return 0;
}

int SaveParamDb (char *parametro, char *value)
{	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char sql[128];
	char *data;
	
	/* Open database */
	rc = sqlite3_open("RiMMS.sql3", &db);
	if( rc ){
		WriteLog("SaveParamDb: Can't open database: %s", sqlite3_errmsg(db));
		return -1;
	}
	
	/* Create merged SQL statement */
	/* Create SQL statement */
	sprintf(sql,"UPDATE config SET value=\"%s\" WHERE parametro=\"%s\"",value,parametro);
	
	Db.numrow=0;
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		WriteLog("SaveParamDb SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return -2;
	}
	sqlite3_close(db);
	return 0;
}

int LoadDeviceDb (DEVICE *Dev)
{	sqlite3 *db;
	char *zErrMsg = 0;
	int rc,i;
	char sql[128];
	char *data;
	
	/* Open database */
	rc = sqlite3_open("RiMMS.sql3", &db);
	if( rc ){
		WriteLog("LoadDeviceDb: Can't open database: %s", sqlite3_errmsg(db));
		return -1;
	}
	
	/* Create SQL statement */
	sprintf(sql,"SELECT * from sensor");
	Db.numrow=0;
	
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		WriteLog("LoadDeviceDb SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return -2;
	}
	sqlite3_close(db);
	
	for(i=0; i<Db.numrow; i++){
		Dev[i].id.Val=atoi(Db.datarow[i][0]);
		Dev[i].add.Val=atoi(Db.datarow[i][1]);
		strcpy(Dev[i].name,Db.datarow[i][2]);
		Dev[i].sampleperiod=atoi(Db.datarow[i][3]);
		Dev[i].radioperiod=atoi(Db.datarow[i][4]);
		Dev[i].min_temperature=atoi(Db.datarow[i][5]);
		Dev[i].max_temperature=atoi(Db.datarow[i][6]);
		Dev[i].min_umidita=atoi(Db.datarow[i][7]);
		Dev[i].max_umidita=atoi(Db.datarow[i][8]);
		Dev[i].min_batteria=atoi(Db.datarow[i][9]);
		Dev[i].max_batteria=atoi(Db.datarow[i][10]);
		Dev[i].aliveperiod=atoi(Db.datarow[i][11]);
		WriteDebug("LoadDeviceDb: %i %i %s %i %i %i %i %i %i %i %i %i"
					,Dev[i].id.Val,Dev[i].add.Val,Dev[i].name,Dev[i].sampleperiod,Dev[i].radioperiod,Dev[i].max_temperature,Dev[i].min_umidita,Dev[i].max_umidita,Dev[i].min_batteria,Dev[i].max_batteria,Dev[i].aliveperiod);
	}
	WriteDebug("LoadDeviceDb: %i",Db.numrow);
	return Db.numrow;
}

int AddDeviceDb(DEVICE Dev)
{	sqlite3 *db;
	char *zErrMsg = 0;
	int rc,i;
	char sql[768];
	char *data;
	
	if(FindDevicesFromId(Dev.id.Val)!=-1) return -1; //ho già il device?
	/* Open database */
	rc = sqlite3_open("RiMMS.sql3", &db);
	if( rc ){
		WriteLog("AddDeviceDb: Can't open database: %s", sqlite3_errmsg(db));
		return -2;
	}
	
	/* Create SQL statement */
	sprintf(sql,"insert into sensor (\"id\",\"address\",\"name\",\"sampleperiod\",\"radioperiod\",\"t_min\",\"t_max\",\"h_min\",\"h_max\",\"b_min\",\"b_max\",,\"aliveperiod\") values (\"%u\",\"%u\",\"%s\",\"%u\",\"%u\",\"%d\",\"%d\",\"%u\",\"%u\",\"%u\",\"%u\",\"%u\")"
				,Dev.id.Val,Dev.add.Val,Dev.name,Dev.sampleperiod,Dev.radioperiod,Dev.min_temperature,Dev.max_temperature,Dev.min_umidita,Dev.max_umidita,Dev.min_batteria,Dev.max_batteria,Dev.aliveperiod);
	Db.numrow=0;
	
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		WriteLog("AddDeviceDb SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return -3;
	}
	sqlite3_close(db);
	
	return 0;
}

int UpdateDeviceDb(DEVICE Dev)
{	sqlite3 *db;
	char *zErrMsg = 0;
	int rc,i;
	char sql[768];
	char *data;
	
	if(FindDevicesFromId(Dev.id.Val)==-1) return -1; //ho il device?
	// Open database 
	rc = sqlite3_open("RiMMS.sql3", &db);
	if( rc ){
		WriteLog("RemoveDeviceDb: Can't open database: %s", sqlite3_errmsg(db));
		return -2;
	}
	
	// Create SQL statement 
	sprintf(sql,"UPDATE sensor SET  address=\"%u\",name=\"%s\",sampleperiod=\"%u\",radioperiod=\"%u\",t_min=\"%d\",t_max=\"%d\",h_min=\"%d\",h_max=\"%d\",b_min=\"%d\",b_max=\"%d\",aliveperiod=\"%d\" WHERE id=\"%u\""
				,Dev.add.Val,Dev.name,Dev.sampleperiod,Dev.radioperiod,Dev.min_temperature,Dev.max_temperature,Dev.min_umidita,Dev.max_umidita,Dev.min_batteria,Dev.max_batteria,Dev.aliveperiod,Dev.id.Val);
	Db.numrow=0;
	
	// Execute SQL statement 
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		WriteLog("RemoveDeviceDb SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return -3;
	}
	sqlite3_close(db);
	
	return 0;
}

int RemoveDeviceDb(DEVICE Dev)
{	sqlite3 *db;
	char *zErrMsg = 0;
	int rc,i;
	char sql[128];
	char *data;
	
	if(FindDevicesFromId(Dev.id.Val)==-1) return -1; //ho il device?
	/* Open database */
	rc = sqlite3_open("RiMMS.sql3", &db);
	if( rc ){
		WriteLog("RemoveDeviceDb: Can't open database: %s", sqlite3_errmsg(db));
		return -2;
	}
	
	/* Create SQL statement */
	sprintf(sql,"DELETE FROM sensor WHERE ID=\"%u\"",Dev.id.Val);
	Db.numrow=0;
	
	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if( rc != SQLITE_OK ){
		WriteLog("RemoveDeviceDb SQL error: %s", zErrMsg);
		sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return -3;
	}
	sqlite3_close(db);
	
	return 0;
}

/*
char *GetSesorDbToFile(char *ret)
{	char cmd[256];
	long size;

	sprintf(cmd,"rm ramdisk/getsensor.txt");
	system(cmd);
	sprintf(cmd,"sqlite3 RiMMS.sql3 < getsensor.sql >> ramdisk/getsensor.txt");
	system(cmd);
	ret=(char *) LoadFile("ramdisk/getsensor.txt",ret,&size);
}
*/