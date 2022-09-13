#define NUM_ADDRESS_PINS 15
#define NUM_DATA_PINS 8

#ifndef ULOG_H
#define ULOG_H
void ulog(int,char*,...);
enum LOGLEVEL {OFF,FATAL,ERROR,WARNING,INFO,DEBUG};
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};
#endif

enum EEPROM_TYPE {AT28C16,AT28C64,AT28C256};
const char *EEPROMTYPESTRINGS[] = {"at28c16","at28c64","at28c256"};

struct Eeprom{
    int addressPins[NUM_ADDRESS_PINS];
    int dataPins[NUM_DATA_PINS];
    int writeEnablePin;
    int outputEnablePin;
    int chipEnablePin;
    int type;
};

int init(struct Eeprom*, int);

int compareBinaryFileToEEPROM(struct Eeprom*, FILE*, long, unsigned long);
int compareTextFileToEEPROM(struct Eeprom*, FILE*,unsigned long);
int writeTextFileToEEPROM(struct Eeprom*, FILE*, int, unsigned long);
int writeBinaryFileToEEPROM(struct Eeprom*, FILE*, int, long, unsigned long);

char readByteFromAddress(struct Eeprom*,unsigned short);
int writeByteToAddress(struct Eeprom*,unsigned short, char, char,int*);

void setDataPins(struct Eeprom*,char);
void setAddressPins(struct Eeprom*,unsigned short);

void printHelp(void);
void printROMContents(struct Eeprom*,long,long,int);

char *num2binStr(char*,int,int);
int binStr2num(const char*);
int str2num(char*);

long expo(int, int);