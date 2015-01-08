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
#include "zlib/include/zconf.h"
#include "zlib/include/zlib.h"
#include "protocolrimms.h"
#include "ftpclient.h"
//#include "mail/mailsend.h"
#include "modem.h"

unsigned char FtpUploading;

int transferperiod,lasttransferperiod;
char UnitName[30];
char ftphost[50];
char ftplogin[50];
char ftppwd[50];
char ftpdir[50];
char ftpport[50];

int RemoveFile(char *file)
{
	return remove(file);
}

int RenameFile(const char *old, const char *new)
{
	return rename(old,new);
}


int RemoveFileToUpload(void)
{
	return remove(DATA_FILE);
}

int RemoveFileSample(void)
{
	return remove(SAMPLE_FILE);
}

int WriteToFile(char *data, int Len)
{
	FILE *fp;
	char ptr[100];
	
    	
	if(FtpUploading){
		strcpy(ptr,SAMPLE_RAM);
	} else {
		strcpy(ptr,SAMPLE_FILE);
	}
    	
	if (!(fp=fopen(ptr,"ab"))) {
		WriteDebug("Enable to open %s file",SAMPLE_FILE);
		return -1;
	}
	fwrite(data, Len, 1, fp);
    fclose(fp);
	return 0;
}

int WriteToFile2(char *file,char *data, int Len)
{
	FILE *fp;
    	
	if (!(fp=fopen(file,"ab"))) {
		WriteDebug("Enable to open %s file",SAMPLE_FILE);
		return -1;
	}
	fwrite(data, Len, 1, fp);
    fclose(fp);
	return 0;
}

int Tag_SOR (int timestamp)
{
	/*SOR This tag starts each record and denotes the GMT time at which the record itself was stored with the following format of the parameter bytes :
	Byte 1
	Bits 7-3	Day (1 – 31)
	Bits 2-0	Higher part of Month (1 – 12)

	Byte 2
	Bit 7		Lower part of Month
	Bits 6-1	Year (0 – 63) (Is this an acceptable limitation?)
	Bit 0		Higher part of fuse deviation from GMT (+/-12)

	Byte 3
	Bits 7-4	Lower part of fuse deviation
	Bits 3-0	Higher part of 1/10 of second count from midnight

	Byte 4
	Bits 7-0	Mid part of 1/10 of second count from midnight

	Byte 5
	Bits 7-0	Lower part of 1/10 of second count from midnight
	*/
	time_t rawtime;
	struct tm * ptm;
	unsigned char data[6];
	unsigned char temp;
	FILE *fp;
	DWORD_D secfrommidnight;
	
	if(timestamp==0){
		time ( &rawtime );
	} else {
		rawtime = timestamp;
	}
	
	ptm = localtime ( &rawtime );
	time_t local = mktime( ptm );
	ptm = gmtime ( &rawtime );
	time_t utc = mktime( ptm );
	long offsetFromUTC = difftime(local,utc); // HOUR_IN_SECONDS;
	offsetFromUTC /= 3600;

	secfrommidnight.Val = (ptm->tm_sec*10)+(ptm->tm_min*600)+(ptm->tm_hour*36000);
	
	data[0]=1; //SOR tag code
	
	temp = (ptm->tm_mday<<3) & 0b11111000;
	temp |= (ptm->tm_mon+1)>>1;
	data[1]=temp;
	
	temp=((ptm->tm_mon+1)<<7)&0b10000000;
	temp |= (((ptm->tm_year-100)&0x3f)<<1)&0b11111110;
	temp |= (offsetFromUTC>>31)&0x01;
	data[2]=temp;
	
	temp=((offsetFromUTC&0xf)<<4)&0b11110000;
	temp |= secfrommidnight.b[2]&0xf;
	data[3]=temp;
	
	data[4] = secfrommidnight.b[1];
	data[5] = secfrommidnight.b[0];
	
	return WriteToFile(data,6);
}

int Tag_DATE_TIME_CHANGE(void)
{
	time_t rawtime;
	struct tm * ptm;
	unsigned char data[6];
	unsigned char temp;
	FILE *fp;
	DWORD_D secfrommidnight;
	
	time ( &rawtime );
		
	ptm = localtime ( &rawtime );
	time_t local = mktime( ptm );
	ptm = gmtime ( &rawtime );
	time_t utc = mktime( ptm );
	long offsetFromUTC = difftime(local,utc); // HOUR_IN_SECONDS;
	offsetFromUTC /= 3600;

	secfrommidnight.Val = (ptm->tm_sec*10)+(ptm->tm_min*600)+(ptm->tm_hour*36000);
	
	data[0]=8;//tag code
	
	temp = (ptm->tm_mday<<3) & 0b11111000;
	temp |= (ptm->tm_mon+1)>>1;
	data[1]=temp;
	
	temp=((ptm->tm_mon+1)<<7)&0b10000000;
	temp |= (((ptm->tm_year-100)&0x3f)<<1)&0b11111110;
	temp |= (offsetFromUTC>>31)&0x01;
	data[2]=temp;
	
	temp=((offsetFromUTC&0xf)<<4)&0b11110000;
	temp |= secfrommidnight.b[2]&0xf;
	data[3]=temp;
	
	data[4] = secfrommidnight.b[1];
	data[5] = secfrommidnight.b[0];
		
	return WriteToFile(data,6);
}

int Tag_CURRENTSENSOR (long id)
{
	FILE *fp;
	unsigned char data[6];
	DWORD_D idtemp;
	
	
	idtemp.Val = id;
	data[0]=15; //tag code
	data[1]=idtemp.b[3];
	data[2]=idtemp.b[2];
	data[3]=idtemp.b[1];
	data[4]=idtemp.b[0];
	
	return WriteToFile(data,5);
}

int Tag_Batteria(long value)
{
	FILE *fp;
	unsigned char data[3];
	WORD_D datavalue;
	
	datavalue.Val = value;
	
	WriteDebug("Batt value %d(d)",datavalue.Val);
	
	data[0]=TAG_MEASURE_BATT; //tag code
	data[1]=datavalue.b[1];
	data[2]=datavalue.b[0];
		
	return WriteToFile(data,3);
}

int Tag_Temperatura(long value)
{
	FILE *fp;
	unsigned char data[3];
	WORD_D datavalue;
	
	datavalue.Val = value;
	
	WriteDebug("Temp value %d(d)",datavalue.Val);
	
	data[0]=TAG_MEASURE_TEMP; //tag code
	data[1]=datavalue.b[1];
	data[2]=datavalue.b[0];
		
	return WriteToFile(data,3);
}

int Tag_Umidita(long value)
{
	unsigned char data[3];
	WORD_D datavalue;
	
	datavalue.Val = value;
	
	WriteDebug("Humi value %d(d)",datavalue.Val);
	
	data[0]=TAG_MEASURE_HUMI; //tag code
	data[1]=datavalue.b[1];
	data[2]=datavalue.b[0];
		
	return WriteToFile(data,3);
}

int Tag_SAMPLING_STARTED(void)
{
	unsigned char data;
	
	data = 2;
	
	return WriteToFile(&data,1);
}

int Tag_SAMPLING_STOPPED(void)
{
	unsigned char data;
	
	data = 3;
	
	return WriteToFile(&data,1);
}

int Tag_SAMPLE_TIME_CHANGED(unsigned short second)
{
	unsigned char data[3];
	WORD_D datavalue;
	
	datavalue.Val = second*10;
	
	data[0]=4; //tag code
	data[1]=datavalue.b[1];
	data[2]=datavalue.b[0];
		
	return WriteToFile(data,3);
}

int Tag_DOWNLOAD_STARTED(void)
{
	unsigned char data;
	
	data = 5;
	
	return WriteToFile(&data,1);
}

int Tag_DOWNLOAD_COMPLETED(void)
{
	unsigned char data;
	
	data = 6;
	
	return WriteToFile(&data,1);
}

int Tag_DOWNLOAD_FAILED(void)
{
	unsigned char data;
	
	data = 7;
	
	return WriteToFile(&data,1);
}

int Tag_POWER_OFF(void)
{
	unsigned char data;
	
	data = 9;
	
	return WriteToFile(&data,1);
}

int Tag_DATA_CLEARED(void)
{
	unsigned char data;
	
	data = 10;
	
	return WriteToFile(&data,1);
}

int Tag_TAG_SENSORFAILURE (long id)
{
	unsigned char data[6];
	DWORD_D idtemp;
	
	
	idtemp.Val = id;
	data[0]=12; //tag code
	data[1]=idtemp.b[3];
	data[2]=idtemp.b[2];
	data[3]=idtemp.b[1];
	data[4]=idtemp.b[0];
	
	return WriteToFile(data,5);
}

int Tag_NEWELEMENT (long id, unsigned int RadioAddress, char *Name, unsigned char type, unsigned char mask)
{
	unsigned char data[168];
	int ret;
	DWORD_D idtemp;
	
	idtemp.Val = id;
	
	data[0]=13;//tag code
	data[1]=idtemp.b[3];
	data[2]=idtemp.b[2];
	data[3]=idtemp.b[1];
	data[4]=idtemp.b[0];
	sprintf(&data[5],"%u",RadioAddress);
	if(strlen(Name)<40){
		sprintf(&data[125],"%s",Name);
	} else {
		WriteLog("Nome troppo lungo");
		return -1;
	}
	if((type==TYPE_SENSORE)||(type==TYPE_ATTUATORE)){
		data[165]=type;
	} else {
		WriteLog("Tipo elemento errato");
		return -1;
	}
	if(mask<=7){
		data[166]=0;
		data[167]=mask;
	} else {
		WriteLog("Maschera misure errata");
		return -1;
	}
	
	ret = WriteToFile(data,168);
	return ret;
}

int Tag_THRESHOLD_OVERFLOW_ALERT(long id, unsigned char TagMeasure, unsigned char ud)
{
	unsigned char data[7];
	DWORD_D idtemp;
	
	idtemp.Val = id;
	
	data[0]=16;//tag code
	data[1]=idtemp.b[3];
	data[2]=idtemp.b[2];
	data[3]=idtemp.b[1];
	data[4]=idtemp.b[0];
	
	if((TagMeasure==TAG_MEASURE_BATT)||(TagMeasure==TAG_MEASURE_TEMP)||(TagMeasure==TAG_MEASURE_HUMI)){
		data[5]=TagMeasure;
	} else {
		WriteLog("Tag misura errata %u",TagMeasure);
		return -1;
	}
	
	if((ud==OVERFLOW_UP)|(ud==OVERFLOW_DWN)){
		data[6]=ud;
	} else {
		WriteLog("Overflow Type errato %u",ud);
		return -1;
	}
	
	return WriteToFile(data,7);
}

int Tag_ELEMENTREMOVED (long id)
{
	unsigned char data[5];
	DWORD_D idtemp;
	
	idtemp.Val = id;
	
	data[0]=14;//tag code
	data[1]=idtemp.b[3];
	data[2]=idtemp.b[2];
	data[3]=idtemp.b[1];
	data[4]=idtemp.b[0];
	
	return WriteToFile(data,5);
}

int	Tag_MEMORY_FULL(void)
{	
	unsigned char data;
	
	data = 11;
	
	return WriteToFile(&data,1);
	
}

int Tag_STORAGE_CLEARED(void)
{	
	unsigned char data;
	
	data = 10;
	return WriteToFile(&data,1);
}

unsigned long file_size (char *file)
{
    unsigned long size;
    FILE *fp;

    if (!(fp=fopen(file,"r")))
        return -1;
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    fclose(fp);

    return size;
}

char *LoadFile(char *file, char *buffer, long *size)
{	FILE *fp;
			
	*size=file_size(file);
	buffer = (char*) calloc(*size,sizeof(char));
	if(buffer==NULL){
		WriteLog("*LoadFile: allocating error");
		return 0;
	}
	if (!(fp=fopen(file,"rb"))) {
		WriteLog("*LoadFile: Enable loading %s file",file);
		return 0;
	}
	WriteDebug("*LoadFile: (%s) (%ld)",file,*size); 
	fread(buffer, *size, 1, fp);
	fclose(fp);
	
	return buffer;
}

long Flip (long data, long Bits)
{
    long reflection = 0;
	long bit;
    /*
     * Reflect the data about the center bit.
     */
    for (bit = 0; bit < Bits; ++bit)
    {
        /*
         * If the LSB bit is set, set the reflection of it.
         */
        if (data & 0x01)
            reflection |= (1 << ((Bits - 1) - bit));
        data = (data >> 1);
    }
    return (reflection);
}

int CRC32FromStream (char *p, long dwSize)
{
	long i;
	unsigned char	bit;
	long dwCrc = 0xffffffff;

	for (i = 0; i < dwSize; i++)
	{
		dwCrc ^= ((unsigned char) Flip (p [i], 8) << (32 - 8));
		for (bit = 8; bit > 0; --bit)
		{
			if (dwCrc & 0x80000000)
				dwCrc = (dwCrc << 1) ^ 0x04C11DB7;
			else
				dwCrc = (dwCrc << 1);
		}
	}
	return (Flip (dwCrc, 32) ^ 0xffffffff);
}


int CreateFileToUpload(void)
{
	char *data,*header,*compheader,*compdata,*filecompress;
	DWORD_D sizeheader,sizedata,cLenheader,cLendata,crc32;
	int ret;
	
	data=NULL;
	header=NULL;
	compheader=NULL;
	compdata=NULL;
	filecompress=NULL;
	ret = -1;
	
	RemoveFileToUpload(); //il file lo ricreo ogni volta
	data=LoadFile(SAMPLE_FILE, data, &sizedata.Val);
	if(data && (sizedata.Val>0)){
		header=LoadFile(HEADER_FILE, header, &sizeheader.Val);
		if(header && (sizeheader.Val>0)){
			cLenheader.Val = 1024*10000;
			compheader = (char*) calloc(cLenheader.Val,sizeof(char));
			if(compheader!=NULL){
				ret = compress(compheader,&cLenheader.Val,header,sizeheader.Val);
				if(ret==Z_OK){
					cLendata.Val = 1024*10000;
					compdata = (char*) calloc(cLendata.Val,sizeof(char));
					if(compdata!=NULL){
						ret = compress(compdata,&cLendata.Val,data,sizedata.Val);
						if(ret==Z_OK){
							filecompress=(char*) calloc(sizeheader.Val+sizedata.Val+25,sizeof(char));
							if(filecompress!=NULL){
								filecompress[0]=0x00;
								filecompress[1]=0x00;
								filecompress[2]=0x04;
								filecompress[3]=0x00;
								filecompress[4]=0x01;
								filecompress[5]=cLenheader.b[1];
								filecompress[6]=cLenheader.b[0];
								filecompress[7]=sizeheader.b[1];
								filecompress[8]=sizeheader.b[0];
								memcpy(&filecompress[9],compheader,cLenheader.Val);
								filecompress[9+cLenheader.Val]=cLendata.b[1];
								filecompress[10+cLenheader.Val]=cLendata.b[0];
								filecompress[11+cLenheader.Val]=sizedata.b[1];
								filecompress[12+cLenheader.Val]=sizedata.b[0];
								memcpy(&filecompress[13+cLenheader.Val],compdata,cLendata.Val);
								filecompress[13+cLenheader.Val+cLendata.Val]=0;
								filecompress[14+cLenheader.Val+cLendata.Val]=0;
								filecompress[15+cLenheader.Val+cLendata.Val]=0;
								filecompress[16+cLenheader.Val+cLendata.Val]=0;
		
								crc32.Val = CRC32FromStream(filecompress,17+cLenheader.Val+cLendata.Val);
	
								filecompress[17+cLenheader.Val+cLendata.Val] = crc32.b[3];
								filecompress[18+cLenheader.Val+cLendata.Val] = crc32.b[2];
								filecompress[19+cLenheader.Val+cLendata.Val] = crc32.b[1];
								filecompress[20+cLenheader.Val+cLendata.Val] = crc32.b[0];
								if(WriteToFile2(DATA_FILE,filecompress,(21+cLenheader.Val+cLendata.Val))==0){
									ret = 0;
								} else {
									WriteLog("Error eriting output file");
									ret = -1;
								}
							} else {
								WriteLog("Error output memory file");
								ret = -1;
							}
						} else {
							WriteLog("Error data compression");
							ret = -1;
						}
					} else {
						WriteLog("Error data memory compression");
						ret = -1;
					}
				} else {
					WriteLog("Error header compression");
					ret = -1;
				}
			} else {
				WriteLog("Error header memory compression");
				ret = -1;
			}
		} else {
			WriteLog("Error Loading HEADER_FILE");
			ret = -1;
		}
	} else {
		WriteLog("Error Loading SAMPLE_FILE");
		ret = -1;
	}
	if(data)		free(data);
	if(header)		free(header);
	if(compheader)	free(compheader);
	if(compdata)	free(compdata);
	if(filecompress)free(filecompress);
	return ret;
}

char *Get_UnitName(void)
{
	return UnitName;
}
		
		
int InitRiMMS (void)
{
	lasttransferperiod = GetClockSecond() - transferperiod + 30; //il primo upload lo eseguo dopo 30 secondi dall'accesione!
	FtpUploading=0;
	
	return 0;
}

int ForceUpload(int sec)
{
	lasttransferperiod = GetClockSecond() - transferperiod + sec; 
}

int GetNameUnit(char *filetmp)
{	
	time_t mytime;
    struct tm *mytm;
	char tmp[16];
	
	mytime=time(NULL);
    mytm=localtime(&mytime);
    strftime(tmp,sizeof (tmp),"%Y%m%d%H%M%S",mytm); 
	sprintf(filetmp,"%s_%s.bin",UnitName,tmp);
	return 0;
}

int GestFtp (void)
{char *buffer;
 long size;
 int i;
 FILE *fp;
 char filetmp[40];
 
 
	if((GetClockSecond()-lasttransferperiod)>=(transferperiod)){
		SetModemStatus(1);
		if(GetModemStatus()==0){
			lasttransferperiod = GetClockSecond()-transferperiod+15;
			return 0;
		}
		lasttransferperiod = GetClockSecond();
		FtpUploading = 1;
		WriteDisplayBuf("FTP Upload...");
		Tag_SOR(0);
		Tag_DOWNLOAD_STARTED();
		if(CreateFileToUpload()!=0){
			WriteLog("Error CreateFileToUpload");
			WriteDisplayBuf("FTP Upload ERROR");
			Tag_SOR(0);
			Tag_DOWNLOAD_FAILED();
			return -1;
		} else {
			buffer = LoadFile(DATA_FILE, buffer, &size);
			if(buffer==NULL){
				WriteLog("Error Loading memory to send FTP file");
				WriteDisplayBuf("FTP Upload ERROR");
				RemoveFileToUpload(); //il file lo ricreo ogni volta
				Tag_SOR(0);
				Tag_DOWNLOAD_FAILED();
				FtpUploading = 0;
				return -1;
			} else
			if(size<=0){
				WriteLog("Error Loading file");
				WriteDisplayBuf("FTP Upload ERROR");
				RemoveFileToUpload(); //il file lo ricreo ogni volta
				free(buffer);
				Tag_SOR(0);
				Tag_DOWNLOAD_FAILED();
				FtpUploading = 0;
				return -1;
			}
			GetNameUnit(filetmp);  //creo i nomi per i file da uplodare
			WriteDebug("GestFtp:(%ld)",size);
			if(ftpupload_RiMMS(ftphost,ftpport,ftplogin,ftppwd,filetmp,ftpdir,buffer,size,ftpclient_printf)==0){
				WriteLog("File Uploaded");
				WriteDisplayBuf("FTP Upload OK");
				RemoveFileSample();
				Tag_SOR(0);
				Tag_STORAGE_CLEARED();
				Tag_SOR(0);
				Tag_DOWNLOAD_COMPLETED();
			} else {
				ForceUpload(15); //ritento l'upload
				WriteDisplayBuf("FTP Upload KO");
				WriteLog("Error uploading file");
				Tag_SOR(0);
				Tag_DOWNLOAD_FAILED();
			}
			free(buffer);
			RemoveFileToUpload(); //il file lo ricreo ogni volta
			FtpUploading=0;
			sprintf(filetmp,"cat %s >> %s",SAMPLE_RAM,SAMPLE_FILE);
			system(filetmp);
			sprintf(filetmp,"rm %s",SAMPLE_RAM);
			system(filetmp);
		}
	}
	return 0;
}

/*
int GestMail(void)
{char stringmail[1024];
	
	sprintf(stringmail,"./mailsend -smtp %s -port %d -auth -f %s -t %s -sub \"%s\" -M \"%s\" -user \"%s\" -pass \"%s\"",smtpserver,smtpport,mailfrom,mailto,mailsubject,mailmsg,smtplogin,smtppwd);
	WriteDebug("%s",stringmail);
	system(stringmail);
	return 0;
}
/*
int GestMail(void)
{int ret;

	g_verbose= 0;
	g_wait_for_cr= 0;
	g_do_ssl= 0;
	g_do_starttls= 0;
	g_quiet= 0;
	g_do_auth= 1;
	g_esmtp= 0;
	g_auth_plain= 0;
	g_auth_cram_md5= 1;
	g_auth_login= 1;
	strcpy(g_charset,"us-ascii");
	strcpy(g_username,smtplogin);
	strcpy(g_userpass,smtppwd);
	ret = send_the_mail(from,save_to,save_cc,save_bcc,sub,smtp_server,smtp_port,helo_domain,NULL,NULL,the_msg,1,NULL,NULL,1);
	
	if(ret==0){
		WriteDebug("Mail sent successfully");
	} else {
		WriteLog("Could not send mail");
	}
	
	return ret;
}
*/
