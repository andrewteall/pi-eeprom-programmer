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


int setupGPIO(struct gpiod_chip *chip, char* chipname, struct gpiod_line **gpioLines,char* consumer){
    int err = 0;
    
    chip = gpiod_chip_open_by_name(chipname);
    
    for(int i=0; i < 28 && err == 0; i++){
        gpioLines[i] = gpiod_chip_get_line(chip, i);
        err = (gpioLines[i] == NULL);

        err = gpiod_line_request_output(gpioLines[i], consumer, 0) || err;
        if(err){
            ulog(ERROR,"Error initially requesting line for OUTPUT");
        }
    }

    return err;
}

int setPinModeGPIO(struct gpiod_chip *chip, struct gpiod_line **gpioLines, int gpioLineNumber,int pinMode){
    int err = 0;

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
        err = 1;
    }
    return err;
}

int writeGPIO(struct gpiod_line **gpioLines,int gpioLineNumber,int value){
    int err = 0;
    err = gpiod_line_set_value(gpioLines[gpioLineNumber],value);
    if(err){
        ulog(ERROR,"Cound not set value: %i on line number: %i",value,gpioLineNumber);
    }
    return err;
}

int readGPIO(struct gpiod_line **gpioLines,int gpioLineNumber){
    int val = 0;
    val = gpiod_line_get_value(gpioLines[gpioLineNumber]);
    if( val < 0){
        ulog(ERROR,"Failed to read input on line %i",gpioLineNumber);
    }
    return val;
}


int cleanupGPIO(struct gpiod_chip *chip,struct gpiod_line **gpioLines){
    int err = 0;

    for(int i=0; i < 28 && err == 0; i++){
        if (i != 2 && i != 3){
            gpiod_line_release(gpioLines[i]);
        }
    }

    gpiod_chip_close(chip);
    return err;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int setupI2C(){
    int  mem_fd;
    void *gpio_map;
    volatile unsigned *gpio;

    /* open /dev/gpiomem */
    if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
        ulog(ERROR,"can't open /dev/gpiomem \n");
        return 1;
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
        return 1;
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;
    
    INP_GPIO(gpio, SDA_PIN);  // Always use INP_GPIO(x) before using SET_GPIO_ALT(x,y)
    SET_GPIO_ALT(gpio, SDA_PIN, ALT_FUNC);

    INP_GPIO(gpio, SCL_PIN);  // Always use INP_GPIO(x) before using SET_GPIO_ALT(x,y)
    SET_GPIO_ALT(gpio, SCL_PIN, ALT_FUNC);
    
    int fd =  open("/dev/i2c-1", O_RDWR );
    ioctl(fd, I2C_SLAVE, 0x50);
    usleep(1000); // Chip Vcc Startup
    return fd;
}

char readByteI2C(int fd, int address){
    char buf[1] = {address};
    int err = write(fd,buf,1);
    err = read(fd,buf,1);
    if(err == -1){
        ulog(ERROR,"Cannot read byte at Address: %i",address);
    }
    return buf[0];
}

char writeByteI2C(int fd, int address, char data){
    int err = 0;
    char buf[2] = {address, data};

    err = write(fd,buf,2);
    usleep(10000);
    return err;
}

void cleanupI2C(int fd){
    close(fd);
}
