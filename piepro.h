#define MAX_ADDRESS_PINS 15
#define MAX_DATA_PINS 8

#ifndef ULOG_H
#define ULOG_H
void ulog(int,const char*,...);
enum LOGLEVEL {OFF,FATAL,ERROR,WARNING,INFO,DEBUG};
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};
#endif

enum EEPROM_MODEL {XL2816,XL28C16,AT28C16,AT28C64,AT28C256,AT24C01,AT24C02,AT24C256,AT24C512,END};
const char* EEPROM_MODEL_STRINGS[] = {"xl2816","xl28c16", \
                                    "at28c16","at28c64","at28c256", \
                                    "at24c01","at24c02","at24c256","at24c512"};
const int EEPROM_MODEL_SIZES[] = {2048,2048, \
                                    2048,8192,32768, \
                                    1024,2048,32768,65536};
const int EEPROM_NUM_ADDRESS_PINS[] = {11,11, \
                                        11,13,15, \
                                        10,11,1,1};
const int EEPROM_NUM_DATA_PINS[] = {8,8, \
                                    8,8,8, \
                                    8,8,1,1};
const int EEPROM_WRITE_CYCLE_USEC[] = {10000,10000, \
                                        5000,10000,1000, \
                                        5000,5000,5000,5000};

struct Eeprom{
    int addressPins[MAX_ADDRESS_PINS];
    int dataPins[MAX_DATA_PINS];
    int writeEnablePin;
    int outputEnablePin;
    int chipEnablePin;
    int vccPin;
    
    int model;
    char type;
    int size;
    char numAddressPins;
    char numDataPins;
    unsigned int writeCycleWait;

    int fd;
    int i2cId;
    char writeProtectPin;
};

int init(struct Eeprom*, int);

int compareBinaryFileToEEPROM(struct Eeprom*, FILE*, long, unsigned long);
int compareTextFileToEEPROM(struct Eeprom*, FILE*, unsigned long, unsigned long);
int writeTextFileToEEPROM(struct Eeprom*, FILE*, int, char, unsigned long, unsigned long);
int writeBinaryFileToEEPROM(struct Eeprom*, FILE*, int, char, long, unsigned long);

char readByteFromAddress(struct Eeprom*,unsigned int);
int writeByteToAddress(struct Eeprom*,unsigned int, char, char,char,int*);

void setDataPins(struct Eeprom*,char);
void setAddressPins(struct Eeprom*,unsigned int);

void waitWriteCycle(int);

void printHelp(void);
void printROMContents(struct Eeprom*,long,long,int);

char *num2binStr(char*,int,int);
int binStr2num(const char*);
int str2num(char*);

long expo(int, int);