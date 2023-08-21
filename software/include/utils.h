#ifndef UTILS_H
    #define UTILS_H 1

    #define MAJOR "0"
    #define MINOR "9"
    #define PATCH "2"
    #define VERSION  MAJOR "." MINOR "." PATCH

    /**
     * @brief Defines the Logging Levels used by uLog.
     */
    enum LOGLEVEL {OFF,FATAL,ERROR,WARNING,INFO,DEBUG};

    enum FILE_TYPE {TEXT_FILE,BINARY_FILE};
	enum APP_FUNCTIONS {NOTHING,WRITE_FILE_TO_ROM,COMPARE_FILE_TO_ROM,DUMP_ROM,WRITE_SINGLE_BYTE_TO_ROM,\
                            READ_SINGLE_BYTE_FROM_ROM,PRINT_VERSION};

    enum BOARD_TYPE{RPI4};


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
        };  

    /**
     * @brief Parses all the command line Arguments and sets the appropriate options
     *        in the OPTIONS struct.
     * @param sOptions A pointer to the OPTIONS struct to store all the configured
     *        options.
     * @param argc Count of command line arguments.
     * @param argv A pointer to the Array of command line arguments.
     * @return 
     */
    int parseCommandLineOptions(struct OPTIONS* sOptions,int argc, char* argv[]);

    void printHelp(void);
    void printVersion();
    

    char *num2binStr(char*,int,int);
    int binStr2num(const char*);
    int str2num(char*);

    long expo(int, int);


    /**
     * @brief Prints a message to stdout with the loggingLevel prepended.
     * @param verbosity The logging level of the generated log.
     * @param logMessage The formatted string to output to stdout.
     * @param ... The formatted string parameters.
     */
    void ulog(int , const char* logMessage, ...);
    
    /**
     * @brief Sets the logging level used by the program
     * @param newLogLevel The logging level to set the program to.
     * @returns uint8_t Returns 0 if loggingLevel is successfully set. 1 if not.
     */
    int setLoggingLevel(int logLevel);

    int getLoggingLevel(void);
#endif