#ifndef GPIO_H
    #define GPIO_H 1
    #include <gpiod.h>
    
    #define LOW 0
    #define HIGH 1
    #define INPUT 0
    #define OUTPUT 1

    int setupGPIO(struct gpiod_chip *chip, char* chipname, struct gpiod_line **gpioLines,char* consumer);

    int cleanupGPIO(struct gpiod_chip *chip,struct gpiod_line **gpioLines);

    int setPinModeGPIO(struct gpiod_chip *chip, struct gpiod_line **gpioLines, int gpioLineNumber,int direction);

    int writeGPIO(struct gpiod_line **gpioLines,int gpioLineNumber,int value);

    int readGPIO(struct gpiod_line **gpioLines,int gpioLineNumber);


    int setupI2C();
    void cleanupI2C(int fd);
    char readByteI2C(int fd, int address);
    char writeByteI2C(int fd, int address, char data);

#endif