#ifndef ULOG_H
#define ULOG_H
void ulog(int,char*,...);
enum LOGLEVEL {OFF,FATAL,ERROR,WARNING,INFO,DEBUG};
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};
#endif

int init(void);
void printHelp(void);
void printROMContents(long,long,int);
char *num2binStr(char*,int,int) ;
void setAddressPins(unsigned short);
char readByteFromAddress(unsigned short);
int binStr2num(const char*);
int writeByteToAddress(unsigned short, char, char,int*);
void setDataPins(char);
void backupWriter(char*);
int str2num(char*);
long expo(int, int);

int compareBinaryFileToEEPROM();
int compareTextFileToEEPROM();

int writeTextFileToEEPROM();
int writeBinaryFileToEEPROM();
