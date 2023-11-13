#ifndef UTILS_H
    #define UTILS_H 1

    /**
     * @brief Converts a number to it's corresponding binary string.
     * @param num The number to convert.
     * @param *binStrBuf The buffer to store the converted string.
     * @param strLenBuf Size of the binStrBuf buffer.
     * @return char* The pointer to the buffer of the converted number.
     */
    char* num2binStr(int num, char* binStrBuf, int strBufLen);

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
    
#endif