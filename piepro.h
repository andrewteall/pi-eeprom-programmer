#define NUM_ADDRESS_PINS 15
#define NUM_DATA_PINS 8
#define NUM_CONTROL_PINS 3

#ifndef ULOG_H
#define ULOG_H
void ulog(int,char*,...);
enum LOGLEVEL {OFF,FATAL,ERROR,WARNING,INFO,DEBUG};
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};
#endif

enum ROM_TYPE {AT28C16,AT28C64,AT28C256};
const char *ROMTYPESTRINGS[] = {"at28c16","at28c64","at28c256"};

struct Pins{
    int addressPins[NUM_ADDRESS_PINS];
    int dataPins[NUM_DATA_PINS];
    int writeEnable;
    int outputEnable;
    int chipEnable;

};

int init(int,struct Pins*);
void printHelp(void);
void printROMContents(struct Pins*,long,long,int);
char *num2binStr(char*,int,int) ;
void setAddressPins(struct Pins*,unsigned short);
char readByteFromAddress(struct Pins*,unsigned short);
int binStr2num(const char*);
int writeByteToAddress(struct Pins*,unsigned short, char, char,int*);
void setDataPins(struct Pins*,char);
void backupWriter(char*);
int str2num(char*);
long expo(int, int);

int compareBinaryFileToEEPROM();
int compareTextFileToEEPROM();

int writeTextFileToEEPROM();
int writeBinaryFileToEEPROM();
