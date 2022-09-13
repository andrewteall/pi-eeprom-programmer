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

struct Eeprom{
    int addressPins[NUM_ADDRESS_PINS];
    int dataPins[NUM_DATA_PINS];
    int writeEnablePin;
    int outputEnablePin;
    int chipEnablePin;
    int type;
};

int init(int,struct Eeprom*);
void printHelp(void);
void printROMContents(struct Eeprom*,long,long,int);
char *num2binStr(char*,int,int) ;
void setAddressPins(struct Eeprom*,unsigned short);
char readByteFromAddress(struct Eeprom*,unsigned short);
int binStr2num(const char*);
int writeByteToAddress(struct Eeprom*,unsigned short, char, char,int*);
void setDataPins(struct Eeprom*,char);
int str2num(char*);
long expo(int, int);

int compareBinaryFileToEEPROM();
int compareTextFileToEEPROM();

int writeTextFileToEEPROM();
int writeBinaryFileToEEPROM();
