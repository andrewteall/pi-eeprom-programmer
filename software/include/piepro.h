#ifndef PIEPRO_H
    #define PIEPRO_H 1
    #include "utils.h"
        
    #define MAX_ADDRESS_PINS 15
    #define MAX_DATA_PINS 8
    
    enum EEPROM_MODEL {XL2816,XL28C16,AT28C16,AT28C64,AT28C256,AT24C01,AT24C02,AT24C256,AT24C512,END};

    extern const char* EEPROM_MODEL_STRINGS[];
    extern const int   EEPROM_MODEL_SIZES[];
    extern const int   EEPROM_NUM_ADDRESS_PINS[];
    extern const int   EEPROM_NUM_DATA_PINS[];
    extern const int   EEPROM_WRITE_CYCLE_USEC[];


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
        int numAddressPins;
        int numDataPins;
        unsigned int writeCycleWait;

        int fd;
        int i2cId;
        char writeProtectPin;
    };

    int init(struct EEPROM*, int,struct OPTIONS *sOptions);
    
    void setDataPins(struct EEPROM*,char);
    void setAddressPins(struct EEPROM*,unsigned int);

    void waitWriteCycle(int);

    int compareFileToEEPROM(struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions);
    int writeFileToEEPROM(struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions);

    char readByteFromAddress(struct EEPROM*,unsigned int);
    int writeByteToAddress(struct EEPROM*,unsigned int, char, struct OPTIONS*,int*);



    void printEEPROMContents(struct EEPROM*, struct OPTIONS*);

#endif