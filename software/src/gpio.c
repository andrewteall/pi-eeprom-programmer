#include <fcntl.h>
#include <gpiod.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gpio.h"
#include "utils.h"

#define MAX_USABLE_GPIO_LINES 34
#define SDA_PIN 2
#define SCL_PIN 3
#define ALT_FUNC 0

#define BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(gpio,g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define SET_GPIO_ALT(gpio,g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

int checkConfig(struct GPIO_CHIP* gpioChip, int gpioLineNumber){
    int rtnVal = 0;

    if(gpioChip->gpioLines == NULL){
        ulog(ERROR,"gpioLines cannot be NULL.");
        rtnVal = -1;
    }
    if (gpioChip != NULL && gpioChip->isSetup != 1){
        ulog(ERROR,"GPIO is not setup.");
        rtnVal = -1;
    }
    if( gpioLineNumber < 0 || gpioLineNumber >= gpioChip->numLinesInUse){
        ulog(ERROR,"Pin Number: %i is out of range of configured Pins",gpioLineNumber);
        rtnVal = -1;
    }
    return rtnVal;
}

/* Sets up GPIO to be used */
int setupGPIO(struct GPIO_CHIP* gpioChip){
    int err = 0;
    
    if(gpioChip == NULL){
        ulog(ERROR,"Config cannot be NULL");
        return -1;
    }
    if(gpioChip->chip == NULL ){
        ulog(ERROR,"Chip cannot be NULL");
        return -1;
    }
    if(gpioChip->chipname == NULL){
        ulog(ERROR,"Chipname cannot be NULL");
        return -1;
    }
    if(gpioChip->gpioLines == NULL){
        ulog(ERROR,"gpioLines cannot be NULL");
        return -1;
    }
    if(gpioChip->consumer == NULL){
        ulog(ERROR,"Consumer cannot be NULL");
        return -1;
    }
    if(gpioChip->isSetup){
        ulog(ERROR,"GPIO is already setup");
        return -1;
    }
    
    gpioChip->chip = gpiod_chip_open_by_name(gpioChip->chipname);
    if (gpioChip->chip == NULL){
        ulog(ERROR,"Unable to get chip by name: %s",gpioChip->chipname);
        return -1;
    }
    if(gpioChip->numGPIOLines > MAX_USABLE_GPIO_LINES && gpioChip->numGPIOLines < gpiod_chip_num_lines(gpioChip->chip)){
        ulog(ERROR,"Invalid Line Count Requested for Chip. Max for Chip: %i",MAX_USABLE_GPIO_LINES);
        return -1;
    }
    if(gpioChip->numGPIOLines >= gpiod_chip_num_lines(gpioChip->chip) || gpioChip->numGPIOLines < 0){
        ulog(ERROR,"Invalid Number of Lines Requested for Chip. Max: %i",gpiod_chip_num_lines(gpioChip->chip));
        return -1;
    }

    for(int i=0; i < gpioChip->numGPIOLines && err == 0; i++){
        gpioChip->gpioLines[i] = gpiod_chip_get_line(gpioChip->chip, i);
        if(gpioChip->gpioLines[i] == NULL){
            ulog(ERROR,"Unable to get line: %i",i);
            err = -1;
        } else {
            err = gpiod_line_request_output(gpioChip->gpioLines[i], gpioChip->consumer, 0);
            if(err){
                ulog(ERROR,"Error initially requesting line for OUTPUT");
            }
        }
    }
    if(err != -1){
        gpioChip->isSetup = 1;
        gpioChip->numLinesInUse = gpioChip->numGPIOLines;
    }
    
    return err;
}

/* Set the GPIO Pin mode to be INPUT or OUTPUT */
int setPinModeGPIO(struct GPIO_CHIP* gpioChip, int gpioLineNumber, int pinMode){
    int err = 0;
    
    if(checkConfig(gpioChip,gpioLineNumber)){
        return -1;
    }

    if(pinMode == OUTPUT){
        // output
        err = gpiod_line_set_config(gpioChip->gpioLines[gpioLineNumber],GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,0,0);
        if(err){
            ulog(ERROR,"Error requesting OUTPUT");
        }
    } else if(pinMode == INPUT){
        // input
        err = gpiod_line_set_config(gpioChip->gpioLines[gpioLineNumber],GPIOD_LINE_REQUEST_DIRECTION_INPUT,0,0);
        if(err){
            ulog(ERROR,"Error requesting INPUT");
        }
    } else {
        ulog(ERROR,"Invalid Pin Mode: %i",pinMode);
        err = -1;
    }
    return err;
}

/* Read from a specified GPIO Pin */
int readGPIO(struct GPIO_CHIP* gpioChip, int gpioLineNumber){
    int val = 0;
 
    if(checkConfig(gpioChip,gpioLineNumber)){
        return -1;
    }

    if((gpiod_line_direction(gpioChip->gpioLines[gpioLineNumber]) == GPIOD_LINE_DIRECTION_INPUT)){
        val = gpiod_line_get_value(gpioChip->gpioLines[gpioLineNumber]);
        if( val == -1){
            ulog(ERROR,"Failed to read input on Pin %i",gpioLineNumber);
        }
    } else {
        val = -1;
        ulog(ERROR,"Pin: %i not configured as INPUT. Cannot be read.",gpioLineNumber);
    }
    return val;
}

/* Write a level to a specified GPIO Pin */
int writeGPIO(struct GPIO_CHIP* gpioChip,int gpioLineNumber,int level){
    int err = 0;

    if(checkConfig(gpioChip,gpioLineNumber)){
        return -1;
    }
    if(level != 0 && level != 1){
        ulog(ERROR,"Invalid level %i for line: %i", level,gpioLineNumber);
        return -1;
    }

    if((gpiod_line_direction(gpioChip->gpioLines[gpioLineNumber]) == GPIOD_LINE_DIRECTION_OUTPUT)){
        err = gpiod_line_set_value(gpioChip->gpioLines[gpioLineNumber],level);
        if(err){
            ulog(ERROR,"Cound not set level: %i on line number: %i",level,gpioLineNumber);
        }
    } else {
        err = -1;
        ulog(ERROR,"Pin: %i not configured as OUTPUT. Cannot be written.",gpioLineNumber);
    }
    return err;
}

/* Release the chip and GPIO lines */
void cleanupGPIO(struct GPIO_CHIP* gpioChip){
    if(gpioChip->isSetup){
        gpioChip->isSetup = 0;
        // Releasing the gpioLines may not be needed since I think it's handled by
        // gpiod_chip_close
        for(int i=0; i < gpioChip->numLinesInUse; i++){
            gpiod_line_release(gpioChip->gpioLines[i]);
        }
        gpiod_chip_close(gpioChip->chip);
    }
    
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* Sets up an I2C device to be used via the built in I2C pins */
int setupI2C(char I2CId){
    int  mem_fd;
    void *gpio_map;
    volatile unsigned *gpio;
    
    /* open /dev/gpiomem */
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        ulog(ERROR,"Error opening /dev/gpiomem");
        return -1;
    }

    /* mmap GPIO */
    gpio_map = mmap(
        NULL,                   //Any adddress in our space will do
        BLOCK_SIZE,             //Map length
        PROT_READ|PROT_WRITE,   // Enable reading & writting to mapped memory
        MAP_SHARED,             //Shared with other processes
        mem_fd,                 //File to map
        0                       //Offset to GPIO peripheral
    );

    close(mem_fd); //No need to keep mem_fd open after mmap

    if (gpio_map == MAP_FAILED) {
        ulog(ERROR,"mmap error");//errno also set!
        return -1;
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;
    
    INP_GPIO(gpio, SDA_PIN);  // Always use INP_GPIO(x) before using SET_GPIO_ALT(x,y)
    SET_GPIO_ALT(gpio, SDA_PIN, ALT_FUNC);

    INP_GPIO(gpio, SCL_PIN);  // Always use INP_GPIO(x) before using SET_GPIO_ALT(x,y)
    SET_GPIO_ALT(gpio, SCL_PIN, ALT_FUNC);
    
    ulog(INFO,"Setting up I2C Device with ID: 0x%02x",I2CId);
    int fd = open("/dev/i2c-1", O_RDWR );
    if(fd == -1){
        ulog(ERROR,"Error opening device.");
        return -1;
    }
    if(ioctl(fd, I2C_SLAVE, I2CId) == -1){
         ulog(ERROR,"Error configuring device.");
         return -1;
    }
    usleep(1000); // Chip Vcc Startup
    return fd;
}

/* Read from a specified address via I2C */
int* readI2C(int fd, int* buf, int numBytesToRead){
    // TODO: Fix data sizings
    int bytesWritten = write(fd,buf,1);
    if(bytesWritten == -1){
        ulog(ERROR,"Error reading byte(s) via I2C");
        return (int*) -1;
    }
    int bytesRead = read(fd,buf,1);
    if(bytesRead == -1){
        buf[0] = bytesRead;
        ulog(ERROR,"Error reading byte(s) via I2C");
        return (int*) -1;
    }
    return buf;
}

/* Write page to a specified address via I2C */
int writeI2C(int fd, char* data, int numBytesToWrite){
    // Write the byte[s]
    int bytesWritten = write(fd, data, numBytesToWrite);
    if(bytesWritten == -1 && data != NULL && numBytesToWrite != 0){
        ulog(ERROR,"Error writing byte(s) via I2C");
    }
    return bytesWritten;
}

/* Closes a specified I2C device */
void cleanupI2C(int fd){
    close(fd);
}
