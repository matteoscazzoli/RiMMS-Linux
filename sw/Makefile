#CC = gcc 
CC = arm-linux-gnueabi-gcc 
#CFLAGS = #-O2 -Wall -static -fpack-struct -I/usr/include -I./include -march=amdfam10 -mtune=amdfam10 -pthread  
ROOTDIR=${PWD}
CFLAGS=#"-I${ROOTDIR}/zlib/include -I${ROOTDIR}/mail"
LDFLAGS="-L${ROOTDIR}/zlib/lib"

# uncomment the following line if compiling on a 64 bit machine 
#BIT64 = -Dbit64

TARGET = RiMMS.exe 
RIMMSOBJ = rimms.o protocolrimms.o
FTPOBJ = ftpclient.o
SYSTEMOBJ = main.o utility.o sm.o gpioFastLib.o display.o
MODEMOBJ = modem.o
COMMANDOBJ = command.o
SQLITE3 = sqlite3/sqlite3.c
DBOBJ = db.o
KEYBOARD = keyboard.o
NETOBJ = rimmsnet.o

.PHONY: install 

all:	$(TARGET) 

$(TARGET):  $(SYSTEMOBJ) $(FTPOBJ) $(RIMMSOBJ) $(MODEMOBJ) $(COMMANDOBJ) $(SQLITE3) $(DBOBJ) $(KEYBOARD) $(NETOBJ)
	$(CC)  $(SYSTEMOBJ) $(FTPOBJ) $(RIMMSOBJ) $(MODEMOBJ) $(COMMANDOBJ) $(SQLITE3) $(DBOBJ) $(KEYBOARD) $(NETOBJ) -o $(TARGET)-pthread $(LDFLAGS) -lz -lrt -ldl
	
.o:
	$(CC) $(CFLAGS) -c ${ROOTDIR}*.c
	
clean:
	rm -f $(TARGET) *.o 

	
