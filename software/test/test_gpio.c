#include <stdlib.h>
#include <unistd.h>

#include "uTest.h"
#include "../include/ulog.h"
#include "../include/gpio.h"


#define NUM_LINES_TO_USE 28
#define I2C_ID 0x50

int unit_test_actual_result;
int unit_test_expected;

struct GPIO_CHIP gpioChip;

char* unitSuiteGPIO0 = "suite_setupGPIO";
char* unitSuiteGPIO1 = "test_setPinModeGPIO";
char* unitSuiteGPIO2 = "suite_readGPIO";
char* unitSuiteGPIO3 = "suite_writeGPIO";

char* unitSuiteI2C0 = "suite_setupI2C";

void init_gpio_test(){
    gpioChip.consumer = "test";
    gpioChip.chipname = "gpiochip0";
    gpioChip.numGPIOLines = NUM_LINES_TO_USE;
    gpioChip.isSetup = 0;
}

void init_I2C_test(){
    gpioChip.consumer = "test";
    gpioChip.chipname = "gpiochip0";
    gpioChip.numGPIOLines = NUM_LINES_TO_USE;
    gpioChip.isSetup = 0;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,13, OUTPUT);
    setPinModeGPIO(&gpioChip,19, OUTPUT);
    setPinModeGPIO(&gpioChip,27, OUTPUT);

    writeGPIO(&gpioChip,13, LOW);
    writeGPIO(&gpioChip,19, LOW);
    writeGPIO(&gpioChip,26, LOW);

    setPinModeGPIO(&gpioChip,16, OUTPUT);
    setPinModeGPIO(&gpioChip,12, OUTPUT);
    writeGPIO(&gpioChip,16, HIGH);
    writeGPIO(&gpioChip,12, HIGH);
}

void cleanup_gpio_test(){
    cleanupGPIO(&gpioChip);
}

void cleanup_I2C_test(int fd){
    cleanupGPIO(&gpioChip);
    cleanupI2C(fd);
}

/******************************************************************************/
/******************************** GPIO Tests **********************************/
/******************************************************************************/

// SUITE - setupGPIO
// TEST - Test to Setup GPIO
void test_setupGPIO(){
    init_gpio_test();
    
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = 0;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO Twice
void test_setupGPIOTwice(){
    init_gpio_test();
    
    setupGPIO(&gpioChip);
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO with Chip Null
void test_setupGPIOWithChipNull(){
    init_gpio_test();
    
    unit_test_actual_result = setupGPIO(NULL);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO with Chipname Null
void test_setupGPIOWithChipnameNull(){
    init_gpio_test();
    gpioChip.chipname = NULL;
    
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO with Consumer Null
void test_setupGPIOWithConsumerNull(){
    init_gpio_test();
    gpioChip.consumer = NULL;
    
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO with Invalid Chipname
void test_setupGPIOWithInvalidChipname(){
    init_gpio_test();
    gpioChip.chipname = "invalid";
    
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO with number of lines too great for chip
void test_setupGPIOWithNumLinesTooHigh(){
    init_gpio_test();
    gpioChip.numGPIOLines = 82;
    
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO with number of lines negative
void test_setupGPIOWithNumLinesNegative(){
    init_gpio_test();
    gpioChip.numGPIOLines = -1;
    
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Test to Setup GPIO with number out of range
void test_setupGPIOWithNumLinesOutOfRange(){
    init_gpio_test();
    gpioChip.numGPIOLines = 35;
    
    unit_test_actual_result = setupGPIO(&gpioChip);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}


// SUITE - Set Pin Mode GPIO
// TEST - Set Pin Mode when gpio has not been setup
void test_setPinModeGPIOWithoutSetup(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int pinMode = OUTPUT;

    unit_test_actual_result = setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Set Pin Mode to Output
void test_setPinModeGPIOToOutput(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int pinMode = OUTPUT;

    setupGPIO(&gpioChip);
    unit_test_actual_result = setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_expected = 0;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Set Pin Mode to Input
void test_setPinModeGPIOToInput(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    unit_test_actual_result = setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_expected = 0;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Set Pin Mode When Chip is NULL
void test_setPinModeGPIOWhenChipNull(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    unit_test_actual_result = setPinModeGPIO(NULL,gpioLineNumber,pinMode);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Set Pin Mode When Line Number is negative
void test_setPinModeGPIOWhenLineNegative(){
    init_gpio_test();
    
    int gpioLineNumber =  -1;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    unit_test_actual_result = setPinModeGPIO(&gpioChip, gpioLineNumber, pinMode);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Set Pin Mode When pinMode is negative
void test_setPinModeGPIOWhenModeNegative(){
    init_gpio_test();
    
    int gpioLineNumber =  0;

    setupGPIO(&gpioChip);
    unit_test_actual_result = setPinModeGPIO(&gpioChip, gpioLineNumber, -1);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Set Pin Mode When Line Number is Out Of Range
void test_setPinModeGPIOWhenLineOutOfRange(){
    init_gpio_test();
    
    int gpioLineNumber =  NUM_LINES_TO_USE;;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    unit_test_actual_result = setPinModeGPIO(&gpioChip, gpioLineNumber, pinMode);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}


// SUITE - Read GPIO
// TEST - Read GPIO when gpio has not been setup
void test_readGPIOWithoutSetup(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    
    unit_test_actual_result = readGPIO(&gpioChip, gpioLineNumber);
    unit_test_expected = -1;
    expect(unit_test_expected, unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Read GPIO set to Input
void test_readGPIOSetToInput(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = readGPIO(&gpioChip,gpioLineNumber);
    unit_test_expected = -1;
    expectNot(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Read GPIO set to Output
void test_readGPIOSetToOutput(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int pinMode = OUTPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = readGPIO(&gpioChip,gpioLineNumber);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Read GPIO Chip Null
void test_readGPIOChipNull(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = readGPIO(NULL,gpioLineNumber);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Read GPIO with Negative Line
void test_readGPIOLineNegative(){
    init_gpio_test();
    
    int gpioLineNumber =  -1;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = readGPIO(&gpioChip,gpioLineNumber);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Read GPIO with Line Out of Range
void test_readGPIOLineOutOfRange(){
    init_gpio_test();
    
    int gpioLineNumber =  NUM_LINES_TO_USE;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = readGPIO(&gpioChip,gpioLineNumber);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}


// SUITE - Write GPIO
// TEST - Read GPIO when gpio has not been setup
void test_writeGPIOWithoutSetup(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int valueToWrite = 1;
    
    unit_test_actual_result = writeGPIO(&gpioChip, gpioLineNumber, valueToWrite);
    unit_test_expected = -1;
    expect(unit_test_expected, unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Write GPIO set to Input
void test_writeGPIOSetToInput(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int valueToWrite = 1;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = writeGPIO(&gpioChip,gpioLineNumber, valueToWrite);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Write GPIO set to Output
void test_writeGPIOSetToOutput(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int valueToWrite = 1;
    int pinMode = OUTPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = writeGPIO(&gpioChip,gpioLineNumber, valueToWrite);
    unit_test_expected = -1;
    expectNot(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Write GPIO Chip Null
void test_writeGPIOChipNull(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int valueToWrite = 1;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = writeGPIO(NULL,gpioLineNumber, valueToWrite);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Write GPIO with Negative Line
void test_writeGPIOLineNegative(){
    init_gpio_test();
    
    int gpioLineNumber =  -1;
    int valueToWrite = 1;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = writeGPIO(&gpioChip,gpioLineNumber, valueToWrite);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Write GPIO with Line Out of Range
void test_writeGPIOLineOutOfRange(){
    init_gpio_test();
    
    int gpioLineNumber =  NUM_LINES_TO_USE;
    int valueToWrite = 1;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = writeGPIO(&gpioChip,gpioLineNumber, valueToWrite);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

// TEST - Write GPIO with invalid value
void test_writeGPIOInvalidValue(){
    init_gpio_test();
    
    int gpioLineNumber =  0;
    int valueToWrite = 3;
    int pinMode = INPUT;

    setupGPIO(&gpioChip);
    setPinModeGPIO(&gpioChip,gpioLineNumber,pinMode);
    unit_test_actual_result = writeGPIO(&gpioChip,gpioLineNumber, valueToWrite);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanupGPIO(&gpioChip);
}

/******************************************************************************/
/******************************** I2C Tests ***********************************/
/******************************************************************************/

// SUITE - Setup I2C
// TEST - Test to Setup I2C with Wrong Device
void test_setupI2CWithWrongID(){
    init_I2C_test();
    int wrongI2cId = 0x55;
    
    unit_test_actual_result = setupI2C(wrongI2cId);
    unit_test_expected = -1;
    expect(unit_test_expected,unit_test_actual_result);

    cleanup_I2C_test(unit_test_actual_result);
}

// TEST - Test to Setup I2C
void test_setupI2C(){
    init_I2C_test();
    
    unit_test_actual_result = setupI2C(I2C_ID);
    unit_test_expected = -1;
    expectNot(unit_test_expected,unit_test_actual_result);

    cleanup_I2C_test(unit_test_actual_result);
}

// TEST - Test to Setup I2C Twice
void test_setupI2CTwice(){
    init_I2C_test();
    
    setupI2C(I2C_ID);
    unit_test_actual_result = setupI2C(I2C_ID);
    unit_test_expected = -1;
    expectNot(unit_test_expected,unit_test_actual_result);

    cleanup_I2C_test(unit_test_actual_result);
}


/******************************************************************************/
/******************************* Test Runners *********************************/
/******************************************************************************/

int run_gpio_tests(int suiteToRun, int testToRun){
    setLoggingLevel(OFF);

    addUnitSuite(unitSuiteGPIO0);
    addUnitTest("Setup GPIO", getCurrentUnitSuite(), test_setupGPIO);
    addUnitTest("Setup GPIO Twice", getCurrentUnitSuite(), test_setupGPIOTwice);
    addUnitTest("Setup GPIO with Chip Null", getCurrentUnitSuite(), test_setupGPIOWithChipNull);
    addUnitTest("Setup GPIO with Chipname Null", getCurrentUnitSuite(), test_setupGPIOWithChipnameNull);
    addUnitTest("Setup GPIO with Consumer Null", getCurrentUnitSuite(), test_setupGPIOWithConsumerNull);
    addUnitTest("Setup GPIO with Invalid", getCurrentUnitSuite(), test_setupGPIOWithInvalidChipname);
    addUnitTest("Setup GPIO with number of lines too great for chip", getCurrentUnitSuite(), test_setupGPIOWithNumLinesTooHigh);
    addUnitTest("Setup GPIO with number of lines negative", getCurrentUnitSuite(), test_setupGPIOWithNumLinesNegative);
    addUnitTest("Setup GPIO with number out of range", getCurrentUnitSuite(), test_setupGPIOWithNumLinesOutOfRange);

    addUnitSuite(unitSuiteGPIO1);
    addUnitTest("Set Pin Mode when gpio has not been setup", getCurrentUnitSuite(), test_setPinModeGPIOWithoutSetup);
    addUnitTest("Set Pin Mode to Output", getCurrentUnitSuite(), test_setPinModeGPIOToOutput);
    addUnitTest("Set Pin Mode to Input", getCurrentUnitSuite(), test_setPinModeGPIOToInput);
    addUnitTest("Set Pin Mode When Chip is NULL", getCurrentUnitSuite(), test_setPinModeGPIOWhenChipNull);
    addUnitTest("Set Pin Mode When Line Number is negative", getCurrentUnitSuite(), test_setPinModeGPIOWhenLineNegative);
    addUnitTest("Set Pin Mode When pinMode is negative", getCurrentUnitSuite(), test_setPinModeGPIOWhenModeNegative);
    addUnitTest("Set Pin Mode When Line Number is Out Of Range", getCurrentUnitSuite(), test_setPinModeGPIOWhenLineOutOfRange);

    addUnitSuite(unitSuiteGPIO2);
    addUnitTest("Read GPIO when gpio has not been setup", getCurrentUnitSuite(), test_readGPIOWithoutSetup);
    addUnitTest("Read GPIO set to Input", getCurrentUnitSuite(), test_readGPIOSetToInput);
    addUnitTest("Read GPIO set to Output", getCurrentUnitSuite(), test_readGPIOSetToOutput);
    addUnitTest("Read GPIO Chip Null", getCurrentUnitSuite(), test_readGPIOChipNull);
    addUnitTest("Read GPIO with Negative Line", getCurrentUnitSuite(), test_readGPIOLineNegative);
    addUnitTest("Read GPIO with Line Out of Range", getCurrentUnitSuite(), test_readGPIOLineOutOfRange);
    
    addUnitSuite(unitSuiteGPIO3);
    addUnitTest("Write GPIO when gpio has not been setup", getCurrentUnitSuite(), test_writeGPIOWithoutSetup);
    addUnitTest("Write GPIO set to Input", getCurrentUnitSuite(), test_writeGPIOSetToInput);
    addUnitTest("Write GPIO set to Output", getCurrentUnitSuite(), test_writeGPIOSetToOutput);
    addUnitTest("Write GPIO Chip Null", getCurrentUnitSuite(), test_writeGPIOChipNull);
    addUnitTest("Write GPIO with Negative Line", getCurrentUnitSuite(), test_writeGPIOLineNegative);
    addUnitTest("Write GPIO with Line Out of Range", getCurrentUnitSuite(), test_writeGPIOLineOutOfRange);
    addUnitTest("Write GPIO with invalid value", getCurrentUnitSuite(), test_writeGPIOInvalidValue);


    runUnitTests(suiteToRun,testToRun);


    return getTestFailCounter();
}

int run_I2C_tests(int suiteToRun, int testToRun){
    setLoggingLevel(OFF);

    addUnitSuite(unitSuiteI2C0);
    addUnitTest("Setup I2C with Wrong Device", getCurrentUnitSuite(), test_setupI2CWithWrongID);
    addUnitTest("Setup I2C", getCurrentUnitSuite(), test_setupI2C);
    addUnitTest("Setup I2C Twice", getCurrentUnitSuite(), test_setupI2CTwice);

    runUnitTests(suiteToRun,testToRun);

    return getTestFailCounter();
}