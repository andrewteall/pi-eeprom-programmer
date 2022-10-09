#define NUM_ADDRESS_PINS 15
#define NUM_DATA_PINS 8

#ifndef ULOG_H
#define ULOG_H
void ulog(int,const char*,...);
enum LOGLEVEL {OFF,FATAL,ERROR,WARNING,INFO,DEBUG};
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};
#endif

enum EEPROM_MODEL {AT28C16,AT28C64,AT28C256,AT24C01,AT24C02,AT24C256,AT24C512};
const int EEPROMMODELSIZES[] = {2048,8192,32768,1024,2048,32768,65536};

struct Eeprom{
    int addressPins[NUM_ADDRESS_PINS];
    int dataPins[NUM_DATA_PINS];
    int writeEnablePin;
    int outputEnablePin;
    int chipEnablePin;
    int vccPin;
    int model;
    int size;
};

int init(struct Eeprom*, int);

int compareBinaryFileToEEPROM(struct Eeprom*, FILE*, long, unsigned long);
int compareTextFileToEEPROM(struct Eeprom*, FILE*,unsigned long);
int writeTextFileToEEPROM(struct Eeprom*, FILE*, int, char,unsigned long);
int writeBinaryFileToEEPROM(struct Eeprom*, FILE*, int, char, long, unsigned long);

char readByteFromAddress(struct Eeprom*,unsigned int);
int writeByteToAddress(struct Eeprom*,unsigned int, char, char,char,int*);

void setDataPins(struct Eeprom*,char);
void setAddressPins(struct Eeprom*,unsigned int);

void printHelp(void);
void printROMContents(struct Eeprom*,long,long,int);

char *num2binStr(char*,int,int);
int binStr2num(const char*);
int str2num(char*);

long expo(int, int);