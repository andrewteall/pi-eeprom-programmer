#ifndef ULOG_H
    #define ULOG_H 1

    /**
     * @brief Defines the Logging Levels used by uLog.
     */
    enum LOGLEVEL {OFF, FATAL, ERROR, WARNING, INFO, DEBUG};

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

    /**
     * @brief Gets the logging level used by the program
     * @returns int Returns the loggingLevel.
     */
    int getLoggingLevel(void);
#endif