#ifndef UTILS_H
    #define UTILS_H 1
    /**
     * @brief Defines the Logging Levels used by uLog.
     */
    enum LOGLEVEL {OFF,FATAL,ERROR,WARNING,INFO,DEBUG};
    // const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};

    enum FILE_TYPE {TEXT_FILE,BINARY_FILE};
	enum APP_FUNCTIONS {WRITE_FILE_TO_ROM,COMPARE_ROM_TO_FILE,DUMP_ROM,WRITE_SINGLE_BYTE_TO_ROM};


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

        int addressParam;
        int dataParam;
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