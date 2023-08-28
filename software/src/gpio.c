#include <fcntl.h>
#include <gpiod.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <sys/ioctl.h>			//Needed for I2C port
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

int checkConfig(struct CHIP_CONFIG* config, struct gpiod_line **gpioLines, int gpioLineNumber){
    int rtnVal = 0;
    
    if(gpioLines == NULL){
        ulog(ERROR,"gpioLines cannot be NULL.");
        rtnVal = -1;
    }
    if (config != NULL && config->isSetup != 1){
        ulog(ERROR,"GPIO is not setup.");
        rtnVal = -1;
    }
    if( gpioLineNumber < 0 || gpioLineNumber >= config->numLinesInUse){
        ulog(ERROR,"Pin Number: %i is out of range of configured Pins",gpioLineNumber);
        rtnVal = -1;
    }
    return rtnVal;
}

/* Sets up GPIO to be used */
int setupGPIO(struct CHIP_CONFIG* config, struct gpiod_chip** chip, char* chipname, struct gpiod_line** gpioLines, \
                char* consumer, int numGPIOLines){
    int err = 0;
    
    if(chip == NULL ){
        ulog(ERROR,"Chip cannot be NULL");
        err = -1;
    }
    if(chipname == NULL){
        ulog(ERROR,"Chipname cannot be NULL");
        err = -1;
    }
    if(gpioLines == NULL){
        ulog(ERROR,"gpioLines cannot be NULL");
        err = -1;
    }
    if(consumer == NULL){
        ulog(ERROR,"Consumer cannot be NULL");
        err = -1;
    }
    if(config == NULL){
        ulog(ERROR,"Config cannot be NULL");
        err = -1;
    }
    
    if(err != -1){
        if(config->isSetup == 1 ){
            ulog(ERROR,"GPIO is already setup");
            err = -1;
        } else {
            *chip = gpiod_chip_open_by_name(chipname);
            if (*chip == NULL){
                ulog(ERROR,"Unable to get chip by name: %s",chipname);
                return -1;
            }
            if(numGPIOLines > MAX_USABLE_GPIO_LINES && numGPIOLines < gpiod_chip_num_lines(*chip)){
                ulog(ERROR,"Invalid Line Count Requested for Chip. Max for Chip: %i",MAX_USABLE_GPIO_LINES);
                return -1;
            }
            if(numGPIOLines >= gpiod_chip_num_lines(*chip) || numGPIOLines < 0){
                ulog(ERROR,"Invalid Number of Lines Requested for Chip. Max: %i",gpiod_chip_num_lines(*chip));
                return -1;
            }

            for(int i=0; i < numGPIOLines && err == 0; i++){
                gpioLines[i] = gpiod_chip_get_line(*chip, i);
                if(gpioLines[i] == NULL){
                    ulog(ERROR,"Unable to get line: %i",i);
                    err = -1;
                } else {
                    err = gpiod_line_request_output(gpioLines[i], consumer, 0);
                    if(err){
                        ulog(ERROR,"Error initially requesting line for OUTPUT");
                    }
                }
            }
            if(err != -1){
                config->isSetup = 1;
                config->numLinesInUse = numGPIOLines;
            }
        }
    }
    return err;
}

/* Set the GPIO Pin mode to be INPUT or OUTPUT */
int setPinModeGPIO(struct CHIP_CONFIG* config, struct gpiod_line **gpioLines, int gpioLineNumber, int pinMode){
    int err = 0;
    
    if(checkConfig(config,gpioLines,gpioLineNumber)){
        return -1;
    }

    if(pinMode == OUTPUT){
        // output
        err = gpiod_line_set_config(gpioLines[gpioLineNumber],GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,0,0);
        if(err){
            ulog(ERROR,"Error requesting OUTPUT");
        }
    } else if(pinMode == INPUT){
        // input
        err = gpiod_line_set_config(gpioLines[gpioLineNumber],GPIOD_LINE_REQUEST_DIRECTION_INPUT,0,0);
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
int readGPIO(struct CHIP_CONFIG* config, struct gpiod_line **gpioLines, int gpioLineNumber){
    int val = 0;
 
    if(checkConfig(config,gpioLines,gpioLineNumber)){
        return -1;
    }

    if((gpiod_line_direction(gpioLines[gpioLineNumber]) == GPIOD_LINE_DIRECTION_INPUT)){
        val = gpiod_line_get_value(gpioLines[gpioLineNumber]);
        if( val == -1){
            ulog(ERROR,"Failed to read input on Pin %i",gpioLineNumber);
        }
    } else {
        val = -1;
        ulog(ERROR,"Pin: %i not configured as INPUT. Cannot be read.",gpioLineNumber);
    }
    return val;
}

/* Write a value to a specified GPIO Pin */
int writeGPIO(struct CHIP_CONFIG* config, struct gpiod_line **gpioLines,int gpioLineNumber,int value){
    int err = 0;

    if(checkConfig(config,gpioLines,gpioLineNumber)){
        return -1;
    }
    if(value != 0 && value != 1){
        ulog(ERROR,"Invalid value %i for line: %i", value,gpioLineNumber);
        return -1;
    }

    if((gpiod_line_direction(gpioLines[gpioLineNumber]) == GPIOD_LINE_DIRECTION_OUTPUT)){
        err = gpiod_line_set_value(gpioLines[gpioLineNumber],value);
        if(err){
            ulog(ERROR,"Cound not set value: %i on line number: %i",value,gpioLineNumber);
        }
    } else {
        err = -1;
        ulog(ERROR,"Pin: %i not configured as OUTPUT. Cannot be written.",gpioLineNumber);
    }
    return err;
}

/* Release the chip and GPIO lines */
void cleanupGPIO(struct CHIP_CONFIG* config, struct gpiod_chip *chip, struct gpiod_line **gpioLines, int numGPIOLines){
    if(config->isSetup){
        config->isSetup = 0;
        // Releasing the gpioLines may not be needed since I think it's handled by
        // gpiod_chip_close
        for(int i=0; i < numGPIOLines; i++){
            gpiod_line_release(gpioLines[i]);
        }
        gpiod_chip_close(chip);
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
    
    int fd =  open("/dev/i2c-1", O_RDWR );
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
int readByteI2C(int fd, int address){
    int buf[1] = {address};
    int bytesWritten = write(fd,buf,1);
    if(bytesWritten == -1){
        ulog(ERROR,"Error reading byte at Address: %i",address);
        return bytesWritten;
    }
    int bytesRead = read(fd,buf,1);
    if(bytesRead == -1){
        buf[0] = bytesRead;
        ulog(ERROR,"Error reading byte at Address: %i",address);
    }
    return buf[0];
}

/* Write to a specified address via I2C */
int writeByteI2C(int fd, int address, char data){
    int buf[2] = {address, data};

    int bytesWritten = write(fd,buf,2);
    if(bytesWritten == -1){
        ulog(ERROR,"Error writing byte: %i at Address: %i",data, address);
    }
    // usleep(10000);
    return bytesWritten;
}

/* Closes a specified I2C device */
void cleanupI2C(int fd){
    close(fd);
}
