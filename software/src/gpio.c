#include <gpiod.h>
#include <unistd.h>

#include <linux/i2c-dev.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>			//Needed for I2C port
#include <sys/mman.h>

#include "gpio.h"
#include "utils.h"

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(gpio,g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(gpio,g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(gpio,g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock


#define SDA_PIN 2
#define SCL_PIN 3
#define ALT_FUNC 0

int checkConfig(int gpioLineNumber, struct CHIP_CONFIG* config){
    int rtnVal = 0;
    if (config->isSetup != 1){
        ulog(ERROR,"GPIO is not setup.");
        rtnVal = -1;
    }
    if( gpioLineNumber >= config->numLinesInUse){
        ulog(ERROR,"Pin Number: %i is out of range of configured Pins",gpioLineNumber);
        rtnVal = -1;
    }
    return rtnVal;
}

/* Sets up GPIO to be used */
int setupGPIO(struct gpiod_chip** chip, char* chipname, struct gpiod_line** gpioLines, \
                char* consumer,int numGPIOLines, struct CHIP_CONFIG* config){
    int err = 0;
    if( config->isSetup == 1){
        ulog(ERROR,"GPIO is already setup");
        err = -1;
    } else {
        *chip = gpiod_chip_open_by_name(chipname);
        if (*chip == NULL){
            ulog(ERROR,"Unable to get chip by name: %s",chipname);
            return -1;
        }
        if(numGPIOLines >= gpiod_chip_num_lines(*chip)){
            ulog(ERROR,"Too Many Lines Requested for Chip. Max: %i",gpiod_chip_num_lines(*chip));
            err = -1;
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
    return err;
}

/* Set the GPIO Pin mode to be INPUT or OUTPUT */
int setPinModeGPIO(struct gpiod_line **gpioLines, int gpioLineNumber,int pinMode, struct CHIP_CONFIG* config){
    int err = 0;
    
    if(checkConfig(gpioLineNumber, config)){
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
int readGPIO(struct gpiod_line **gpioLines,int gpioLineNumber,struct CHIP_CONFIG* config){
    int val = 0;
    
    if(checkConfig(gpioLineNumber, config)){
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
int writeGPIO(struct gpiod_line **gpioLines,int gpioLineNumber,int value, struct CHIP_CONFIG* config){
    int err = 0;

    if(checkConfig(gpioLineNumber, config)){
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
void cleanupGPIO(struct gpiod_chip *chip,struct gpiod_line **gpioLines, int numGPIOLines, struct CHIP_CONFIG* config){
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
