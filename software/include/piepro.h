#include "utils.h"

#define MAX_ADDRESS_PINS 15
#define MAX_DATA_PINS 8

/* Delete After Adding GPIO Calls*/
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 0
#define PUD_DOWN 0
#define PUD_UP 0
#define PUD_OFF 0
/*********************************/

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
                                    8,8,8,8};
const int EEPROM_WRITE_CYCLE_USEC[] = {10000,10000, \
                                        5000,10000,1000, \
                                        5000,5000,5000,5000};

struct EEPROM{
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

int init(struct EEPROM*, int);

int compareBinaryFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);
int compareTextFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);
int writeTextFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);
int writeBinaryFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);

char readByteFromAddress(struct EEPROM*,unsigned int);
int writeByteToAddress(struct EEPROM*,unsigned int, char, struct OPTIONS*,int*);

void setDataPins(struct EEPROM*,char);
void setAddressPins(struct EEPROM*,unsigned int);

void waitWriteCycle(int);

void printHelp(void);
void printROMContents(struct EEPROM*, struct OPTIONS*);

char *num2binStr(char*,int,int);
int binStr2num(const char*);
int str2num(char*);

long expo(int, int);