#include <fcntl.h>
#include <gpiod.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gpio.h"
#include "utils.h"
#include "ulog.h"

#define MAX_USABLE_GPIO_LINES 34
#define SDA1_PIN 2
#define SCL1_PIN 3
#define BLOCK_SIZE 4096

enum ALT_MODE {ALT0=0,ALT1,ALT2,ALT3,ALT4,ALT5};

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(gpio,g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define SET_GPIO_ALT(gpio,g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

/* Set the ALT MODE of the specified pin */
void setPinAltModeGPIO(volatile unsigned int* gpio, int pin, enum ALT_MODE altMode){
    INP_GPIO(gpio,pin);
    SET_GPIO_ALT(gpio,pin,altMode);
}

/* Ensure chip and line number are valid */
int checkConfigGPIO(struct GPIO_CHIP* gpioChip, int gpioLineNumber){
    int rtnVal = 0;

    if(gpioChip == NULL){
        ulog(ERROR,"gpioChip cannot be NULL.");
        return -1;
    }
    if (gpioChip != NULL && gpioChip->isSetup != 1){
        ulog(ERROR,"GPIO is not setup.");
        return -1;
    }
    if( gpioLineNumber < 0 || gpioLineNumber >= gpioChip->numLinesInUse){
        ulog(ERROR,"Pin Number: %i is out of range of configured Pins",gpioLineNumber);
        return -1;
    }
    return rtnVal;
}

/* Sets up GPIO to be used */
int setupGPIO(struct GPIO_CHIP* gpioChip){
    int err = 0;
    
    if(gpioChip == NULL){
        ulog(ERROR,"gpioChip cannot be NULL");
        return -1;
    }
    if(gpioChip->chipname == NULL){
        ulog(ERROR,"Chipname cannot be NULL");
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
    
    if(checkConfigGPIO(gpioChip,gpioLineNumber)){
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
 
    if(checkConfigGPIO(gpioChip,gpioLineNumber)){
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

    if(checkConfigGPIO(gpioChip,gpioLineNumber)){
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

/* Map memory used by GPIO */
volatile unsigned int* mapGPIOMemory(){
    int  mem_fd;
    void *gpio_map;

    /* open /dev/gpiomem */
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        ulog(ERROR,"Error opening /dev/gpiomem");
        return (volatile unsigned int*)-1;
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
        return (volatile unsigned int*)-1;
    }
    return (volatile unsigned int*)gpio_map;
}

/* Sets up an I2C device to be used via the built in I2C pins */
int setupI2C(char I2CId){
    // Always use volatile pointer!
    volatile unsigned int* gpio = mapGPIOMemory();
    if(gpio == (volatile unsigned int*)-1){
        return -1;
    }
    
    setPinAltModeGPIO(gpio, SDA1_PIN, ALT0);
    setPinAltModeGPIO(gpio, SCL1_PIN, ALT0);
    
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
    // Dummy write to make sure device is setup
    if(write(fd, NULL, 0) != 0){
        ulog(ERROR,"Device not available");
        return -1;
    }
    return fd;
}

/* Read from a specified address via I2C */
int readI2C(int fd, char* buf, int numBytesToRead, int addressSize){
    int bytesWritten = write(fd, buf, addressSize);
    if(bytesWritten == -1){
        ulog(ERROR,"Error reading byte(s) via I2C");
        return -1;
    }
    int bytesRead = read(fd,buf,numBytesToRead);
    if(bytesRead == -1){
        buf[0] = bytesRead;
        ulog(ERROR,"Error reading byte(s) via I2C");
        return -1;
    }
    return bytesRead;
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
