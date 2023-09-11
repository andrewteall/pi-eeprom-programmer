#ifndef PIEPRO_H
    #define PIEPRO_H 1
    #include "gpio.h"
    
    #define MAX_ADDRESS_PINS 15
    #define MAX_DATA_PINS 8
    #define MAX_PAGE_SIZE 256
    #define MAX_ADDRESS_SIZE 3
    
    /**
     * @brief Enumeration of the different supported EEProm models.
     */
    enum EEPROM_MODEL {XL2816,XL28C16,
                        AT28C16,AT28C64,AT28C256,
                        AT24C01,AT24C02,AT24C04,AT24C08,AT24C16,AT24C32,AT24C64,AT24C128,AT24C256,AT24C512,END};

    /**
     * @brief Enumeration of the different types of supported EEProm protocols.
     */
    enum EEPROM_TYPE {PARALLEL,I2C};       

    /**
     * @brief Defines file types supported.
     */
    enum FILE_TYPE {TEXT_FILE,BINARY_FILE};

    /**
     * @brief Defines the supported functions of the programmer.
     */
	enum APP_FUNCTIONS {
                        NOTHING,
                        WRITE_FILE_TO_ROM,
                        COMPARE_FILE_TO_ROM,
                        WRITE_SINGLE_BYTE_TO_ROM,
                        READ_SINGLE_BYTE_FROM_ROM,
                        DUMP_ROM
                        };

    /**
     * @brief Defines the different SoC board types supported.
     */
    enum BOARD_TYPE{RPI4};

    /**
     * @brief Array of EEProm model strings correlating to the EEProm models.
     */
    extern const char* EEPROM_MODEL_STRINGS[];

    /**
     * @brief Array of EEProm model sizes correlating to the EEProm models.
     */
    extern const int   EEPROM_MODEL_SIZES[];

    /**
     * @brief Array of EEProm model address pin counts correlating to the EEProm models.
     */
    extern const int   EEPROM_ADDRESS_LENGTH[];

    /**
     * @brief Array of EEProm model data pin counts correlating to the EEProm models.
     */
    extern const int   EEPROM_DATA_LENGTH[];

    /**
     * @brief Array of EEProm model write cycle timings correlating to the EEProm models.
     */
    extern const int   EEPROM_WRITE_CYCLE_USEC[];

    /**
     * @struct OPTIONS
     * @brief This structure contains all the configuration parameters for a given
     *        system.
     */
    struct OPTIONS {
        char* filename;
        long limit;
        long startValue;
        int dumpFormat;
        int validateWrite;
        int force;
        int action;
        int fileType;
        int eepromModel;
        int writeCycleUSec;
        char i2cId;
        int boardType;

        int addressParam;
        int dataParam;

        char* consumer;
        char* chipname;
        int numGPIOLines;
        };  

    /**
     * @brief EEPROM struct to hold eeprom configuration parameters.
     */
    struct EEPROM{
        // All
        int type;
        int forceWrite;
        int validateWrite;
        int addressPins[MAX_ADDRESS_PINS];
        int dataPins[MAX_DATA_PINS];
        int writeEnablePin;
        int outputEnablePin;
        int chipEnablePin;
        int vccPin;
        int writeCycleTime;
        int useWriteCyclePolling;
        
        int size;
        int maxAddressLength;
        int maxDataLength;

        // I2C
        int fd;
        int i2cId;
        char writeProtectPin;
        int pageSize;
        int addressSize;

        // Info
        int model;
        int byteWriteCounter;
        int byteReadCounter;
    };

    /**
     * @brief GPIO_CONFIG struct to hold gpio chip configuration parameters.
     */
    struct GPIO_CONFIG{
        int numGPIOLines;
        char *chipname;
        char *consumer;
        struct GPIO_CHIP gpioChip;
    };

    /**
     * @brief Initializes and EEProm struct with the porvided options.
     * @param *eeprom A eeprom struct that contains the eeprom info.
     * @param *sOptions A pointer to the OPTIONS struct to reference all the configured options.
     * @param *gpioChip A pointer to the GPIO_CONFIG struct to reference gpio chip to be used.
     * @return int 0 if successful. Non-zero if error.
     */
    int initHardware(struct EEPROM* eeprom, struct GPIO_CONFIG* gpioChip, struct OPTIONS *sOptions);
    
    /**
     * @brief Compares a file to the EEPROM given the specified options.
     * @param *gpioChip A pointer to the GPIO_CONFIG struct to reference gpio chip to be used.
     * @param *eeprom A eeprom struct that contains the eeprom info.
     * @param *romFile A pointer to File to compare.
     * @param *sOptions A pointer to the OPTIONS struct to reference all the configured options.
     * @return int 0 if successful. Non-zero if error.
     */
    int compareFileToEEPROM(struct GPIO_CONFIG* gpioChip, struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions);

    /**
     * @brief Writes a file to the EEPROM given the specified options.
     * @param *gpioChip A pointer to the GPIO_CONFIG struct to reference gpio chip to be used.
     * @param *eeprom A eeprom struct that contains the eeprom info.
     * @param *romFile A pointer to File to read from.
     * @param *sOptions A pointer to the OPTIONS struct to reference all the configured options.
     * @return int 0 if successful. Non-zero if error.
     */
    int writeFileToEEPROM(struct GPIO_CONFIG* gpioChip,struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions);

    /**
     * @brief Read a byte from a specified address.
     * @param *gpioChip A pointer to the GPIO_CONFIG struct to reference gpio chip to be used.
     * @param *eeprom A eeprom struct that contains the eeprom info.
     * @param addressToRead The address to read from on the EEPROM.
     * @return int Returns the value of the byte read.
     */
    int readByteFromAddress(struct GPIO_CONFIG* gpioChip, struct EEPROM* eeprom,unsigned int addressToRead);

    /**
     * @brief Writes a byte to a specified address.
     * @param *gpioChip A pointer to the GPIO_CONFIG struct to reference gpio chip to be used.
     * @param *eeprom A eeprom struct that contains the eeprom info.
     * @param addressToWrite The address to write to on the EEPROM.
     * @param dataToWrite The data to write to on the EEPROM.
     * @return int Returns 0 if successful 1 if error.
     */
    int writeByteToAddress(struct GPIO_CONFIG* gpioChip, struct EEPROM* eeprom, int addressToWrite, char dataToWrite);
    
    /**
     * @brief Prints the contents of the EEPROM accoring to sOPtions.
     * @param *gpioChip A pointer to the GPIO_CONFIG struct to reference gpio chip to be used.
     * @param *eeprom A eeprom struct that contains the eeprom info.
     * @param *sOptions A pointer to the OPTIONS struct to reference all the configured options.
     */
    void printEEPROMContents(struct GPIO_CONFIG* gpioChip, struct EEPROM* eeprom, struct OPTIONS* sOptions);

    /**
     * @brief Releases and frees GPIO hardware.
     * @param *gpioChip A pointer to the GPIO_CONFIG struct to reference gpio chip to be used.
     * @param *eeprom A eeprom struct that contains the eeprom info.
     */
    void cleanupHardware(struct GPIO_CONFIG* gpioChip, struct EEPROM* eeprom);

    /**
     * @brief Parses all the command line Arguments and sets the appropriate options
     *        in the OPTIONS struct.
     * @param sOptions A pointer to the OPTIONS struct to store all the configured
     *        options.
     * @param argc Count of command line arguments.
     * @param argv A pointer to the Array of command line arguments.
     * @return int 0 is no errors occured.
     */
    int parseCommandLineOptions(struct OPTIONS* sOptions,int argc, char* argv[]);

    /**
     * @brief Prints the command usage.
     */
    void printHelp(void);

    /**
     * @brief Prints the current version of the application.
     */
    void printVersion(void);
#endif