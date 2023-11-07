#ifndef UTILS_H
    #define UTILS_H 1

    #define MAJOR "0"
    #define MINOR "9"
    #define PATCH "2"
    #define VERSION  MAJOR "." MINOR "." PATCH

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

#endif