#ifndef GPIO_H
    #define GPIO_H 1
    #include <gpiod.h>
    
    /**
     * @brief Levels used by GPIO.
     */
    #define LOW 0
    #define HIGH 1

    /**
     * @brief Pin Modes used by GPIO.
     */
    #define INPUT 0
    #define OUTPUT 1

    struct CHIP_CONFIG{
        int isSetup;
        int numLinesInUse;
        int numLinesAvailable;
    };

    /**
     * @brief Setups Raspberry Pi GPIO Pins.
     *        Must be called before any GPIO is used.
     * @param *chip A pointer to a gpiod_chip struct for the chip to be opened.
     * @param *chipname String name of the chip whose GPIO is to be used.
     * @param **gpioLines A pointer to an array of gpiod_lines to be init.
     * @param *consumer String name of the consumer.
     * @param numGPIOLines Number of lines used to be setup
     * @param *config Pointer to the chip configuration to be used
     * @return int 0 if successful -1 if any error occurs.
     */
    int setupGPIO(struct CHIP_CONFIG* config, struct gpiod_chip** chip, char* chipname, struct gpiod_line** gpioLines, \
                    char* consumer, int numGPIOLines);

    /**
     * @brief Set GPIO Pins to input or output.
     * @param *config Pointer to the chip configuration to be used
     * @param **gpioLines Pointer to an array of gpiod_lines.
     * @param gpioLineNumber the GPIO number to be control direction.
     * @param direction The mode of the GPIO Pin. INPUT or OUTPUT
     * @return int 0 if successful -1 if any error occurs.
     */
    int setPinModeGPIO(struct CHIP_CONFIG* config, struct gpiod_line **gpioLines, int gpioLineNumber, int direction);

    /**
     * @brief Read the value of the specified GPIO Pin.
     * @param *config Pointer to the chip configuration to be used
     * @param **gpioLines Pointer to an array of gpiod_lines.
     * @param gpioLineNumber the GPIO number to be read from.
     * @return int 0 if successful -1 if any error occurs.
     */
    int readGPIO(struct CHIP_CONFIG* config, struct gpiod_line **gpioLines, int gpioLineNumber);

    /**
     * @brief Write the value to the specified GPIO Pin.
     * @param *config Pointer to the chip configuration to be used
     * @param **gpioLines Pointer to an array of gpiod_lines.
     * @param gpioLineNumber the GPIO number to be written to.
     * @param value The value to write. HIGH or LOW.
     * @return int 0 if successful -1 or non-zero if any error occurs.
     */
    int writeGPIO(struct CHIP_CONFIG* config, struct gpiod_line **gpioLines,int gpioLineNumber,int value);
    
    /**
     * @brief Releases a gpiod_chip and an array of gpiod_lines.
     * @param *config Pointer to the chip configuration to be used
     * @param *chip Pointer to a gpiod_chip struct for the chip to be released.
     * @param **gpioLines Pointer to an array of gpiod_lines to be released.
     * @param numGPIOLines Number of lines used to be cleanedup
     */
    void cleanupGPIO(struct CHIP_CONFIG* config, struct gpiod_chip *chip, struct gpiod_line **gpioLines,int numGPIOLines);

    /**
     * @brief Sets up GPIO pins to use the dedicated I2C function.
     * @param I2CId The address of the I2C device. Default 0x50.
     * @return int The file descriptor of the I2C device. -1 if error.
     */
    int setupI2C(char I2CId);

    /**
     * @brief Reads a byte via the I2C bus.
     * @param int The file descriptor of the I2C device to be used.
     * @param int The address to be read.
     * @return int The byte read from the I2C device or -1 if error.
     */
    int readByteI2C(int fd, int address);
    
    /**
     * @brief Writes a byte via the I2C bus.
     * @param int The file descriptor of the I2C device to be used.
     * @param int The address to be written.
     * @param char The byte to be written.
     * @return int number of bytes writen if successful -1 if any error occurs.
     */
    int writeByteI2C(int fd, int address, char data);

    /**
     * @brief Closes the specified File Descriptor(fd).
     * @param int The file descriptor of the I2C device to be closed.
     */
    void cleanupI2C(int fd);

#endif