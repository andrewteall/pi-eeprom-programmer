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

    
    /**
     * @brief Converts a number to it's corresponding binary string.
     * @param *binStrBuf The buffer to store the converted string.
     * @param num The number to convert.
     * @param strLenBuf Size of the binStrBuf buffer.
     * @return char* The pointer to the buffer of the converted number.
     */
    char* num2binStr(char* binStrBuf, int num, int strBufLen);

    /**
     * @brief Converts a binary string to it's corresponding value.
     * @param *binStr The string to convert
     * @return int The resulting number of the conversion. -1 if error.
     */
    int binStr2num(char *binStr);
    
    /**
     * @brief Converts a string to it's corresponding value.
     * @param numStr The string to convert
     * @return int The resulting number of the conversion. -1 if error.
     */
    int str2num(char *numStr);

    /**
     * @brief Performs exponentiation of two numbers.
     * @param base The number to be exponentiated
     * @param power The exponent
     * @return long The result of the performed calculation.
     */
    long expo(int base, int power);

    /**
     * @brief Rounds a float up to the nearest whole number.
     * @param floatNumber The number to round up.
     * @return int Number that has been rounded up.
     */
    int roundUp(float floatNumber);

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