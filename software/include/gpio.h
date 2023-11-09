#ifndef GPIO_H
    #define GPIO_H 1

    /**
     * @brief Levels used by GPIO.
     */
    enum LEVEL {LOW=0, HIGH};

    /**
     * @brief Pin Modes used by GPIO.
     */
    enum PIN_MODE {INPUT=0, OUTPUT};

    struct GPIO_CHIP{
        // GPIO
        int isSetup;
        int numLinesInUse;
        struct gpiod_chip* chip;
        struct gpiod_line* gpioLines[40];
        
        int numGPIOLines;
        char *chipname;
        char *consumer;
        
        // I2C
        int isI2CSetup;
        int fd;
    };

    /**
     * @brief Setups Raspberry Pi GPIO Pins.
     *        Must be called before any GPIO is used.
     * @param *gpioChip Pointer to the GPIO_CHIP struct to be used
     * @return int 0 if successful -1 if any error occurs.
     */
    int setupGPIO(struct GPIO_CHIP* gpioChip);

    /**
     * @brief Set GPIO Pins to input or output.
     * @param *gpioChip Pointer to the GPIO_CHIP struct to be used
     * @param **gpioLines Pointer to an array of gpiod_lines.
     * @param gpioLineNumber the GPIO number to be control direction.
     * @param direction The mode of the GPIO Pin. INPUT or OUTPUT
     * @return int 0 if successful -1 if any error occurs.
     */
    int setPinModeGPIO(struct GPIO_CHIP* gpioChip, int gpioLineNumber, int direction);

    /**
     * @brief Read the value of the specified GPIO Pin.
     * @param *gpioChip Pointer to the GPIO_CHIP struct to be used
     * @param **gpioLines Pointer to an array of gpiod_lines.
     * @param gpioLineNumber the GPIO number to be read from.
     * @return int 0 if successful -1 if any error occurs.
     */
    int readGPIO(struct GPIO_CHIP* gpioChip, int gpioLineNumber);

    /**
     * @brief Write the value to the specified GPIO Pin.
     * @param *gpioChip Pointer to the GPIO_CHIP struct to be used
     * @param **gpioLines Pointer to an array of gpiod_lines.
     * @param gpioLineNumber the GPIO number to be written to.
     * @param level The value to write. HIGH or LOW.
     * @return int 0 if successful -1 or non-zero if any error occurs.
     */
    int writeGPIO(struct GPIO_CHIP* gpioChip,int gpioLineNumber,int level);
    
    /**
     * @brief Releases a gpiod_chip and an array of gpiod_lines.
     * @param *gpioChip Pointer to the GPIO_CHIP struct to be used
     */
    void cleanupGPIO(struct GPIO_CHIP* gpioChip);

    /**
     * @brief Sets up GPIO pins to use the dedicated I2C function.
     * @param I2CId The address of the I2C device. Default 0x50.
     * @return int The file descriptor of the I2C device. -1 if error.
     */
    int setupI2C(char I2CId);

    /**
     * @brief Reads a byte via the I2C bus.
     * @param int The file descriptor of the I2C device to be used.
     * @param char* Buffer for the bytes that are read.
     * @param int The number of bytes to read including the address.
     * @return int* The byte read from the I2C device or -1 if error.
     */
    int* readI2C(int fd, int* buf, int numBytesToRead);
    
    /**
     * @brief Writes a byte via the I2C bus.
     * @param int The file descriptor of the I2C device to be used.
     * @param char* The data to be written.
     * @param int The number of bytes to write including the address.
     * @return int number of bytes writen if successful -1 if any error occurs.
     */
    int writeI2C(int fd, char* data, int numBytesToWrite);

    /**
     * @brief Closes the specified File Descriptor(fd).
     * @param int The file descriptor of the I2C device to be closed.
     */
    void cleanupI2C(int fd);

#endif