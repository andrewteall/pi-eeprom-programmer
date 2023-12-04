#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "uTest.h"
#include "../include/piepro.h"
#include "../include/ulog.h"
#include "../include/utils.h"

char* defaultPath = "test/data";
char* defaultFilename = "test/data/eeprom.bin";
char* defaultNonMatchingFilename = "test/data/eeprom-unmatched.bin";
char* defaultTextFilename = "test/data/eeprom.txt";
char* defaultNonMatchingTextFilename = "test/data/eeprom-unmatched.txt";
char* defaultOversizeFilename = "test/data/eeprom-oversized.bin";
char* defaultOversizeTextFilename = "test/data/eeprom-oversized.txt";

struct EEPROM eeprom;
struct GPIO_CONFIG gpioConfig;
struct OPTIONS options;
int error;
int i2cDevice;

int testFuncNum = 0;

char* filename;
char* nonMatchingFilename;
char* textFilename;
char* nonMatchingTextFilename;
char* oversizeFilename;
char* oversizeTextFilename;

int eepromModel;
FILE* romFile;
int actual_result;
int expected;
int address;
char data;

int useLimit = 0;
int limitSize = 0;

void reset_filenames(){
    setLoggingLevel(OFF);
    if(getenv("PIEPRO_FILENAME") == NULL){
        filename = defaultFilename;
    } else {
        filename = getenv("PIEPRO_FILENAME");
    }

    if(getenv("PIEPRO_NON_MATCHING_FILENAME") == NULL){
        nonMatchingFilename = defaultNonMatchingFilename;
    } else {
        nonMatchingFilename = getenv("PIEPRO_NON_MATCHING_FILENAME");
    }

    if(getenv("PIEPRO_TEXT_FILENAME") == NULL){
        textFilename = defaultTextFilename;
    } else {
        textFilename = getenv("PIEPRO_TEXT_FILENAME");
    }

    if(getenv("PIEPRO_NON_MATCHING_TEXT_FILENAME") == NULL){
        nonMatchingTextFilename = defaultNonMatchingTextFilename;
    } else {
        nonMatchingTextFilename = getenv("PIEPRO_NON_MATCHING_TEXT_FILENAME");
    }

    if(getenv("OVERSIZE_FILENAME") == NULL){
        oversizeFilename = defaultOversizeFilename;
    } else {
        oversizeFilename = getenv("OVERSIZE_FILENAME");
    }

    if(getenv("OVERSIZE_TEXT_FILENAME") == NULL){
        oversizeTextFilename = defaultOversizeTextFilename;
    } else {
        oversizeTextFilename = getenv("OVERSIZE_TEXT_FILENAME");
    }
   
}

FILE* init_test_romFile(char* file){
    options.filename = file;
    
    romFile = fopen(options.filename, "r");
    if(romFile == NULL){
        fprintf(stderr,"Error Opening File\n");
        error = -1;
    }
    return romFile;
}

int get_file_size(FILE* file){
    unsigned long currentPos = ftell(file);
    rewind(file);
    fseek(file, 0L, SEEK_END);
	unsigned long size = ftell(file);
	rewind(file);
    fseek(file, 0L, currentPos);
    return size;
}

int get_random_data(int limit){
    return (rand() % (limit - 0 + 1));
}

void init_test(){
    setDefaultOptions(&options);
    init_test_romFile(filename);
    options.eepromModel = eepromModel;
    initHardware(&options, &eeprom, &gpioConfig);

    if(useLimit){
        eeprom.size = limitSize;
        eeprom.limit = limitSize;
    }
}

void cleanup_test(){
    fclose(romFile);
    cleanupHardware(&gpioConfig, &eeprom);
}

/******************************** Tests Start *********************************/
/******************************************************************************/
// SUITE - Write Binary File to EEPROM
/******************************************************************************/
// TEST - Force Write Binary File to EEPROM
void test_forceWriteBinaryFileToEEPROM(){
    init_test();

    eeprom.forceWrite = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + get_file_size(romFile) + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Default Write Binary File to EEPROM
void test_writeBinaryFileToEEPROM(){
    init_test();

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Binary File to EEPROM with Negative start value
void test_writeBinaryFileToEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = -1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Binary File to EEPROM with start value
void test_writeBinaryFileToEEPROMWithStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 87;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Binary File to EEPROM with Limit
void test_writeBinaryFileToEEPROMWithLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.limit = 145;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.limit + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Binary File to EEPROM with start value and limit
void test_writeBinaryFileToEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 120;
    eeprom.limit = 145;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.limit - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Binary File to EEPROM with Excess Limit
void test_writeBinaryFileToEEPROMWithExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.limit = eeprom.size + 9;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Binary File to EEPROM with Start Value and Excess Limit
void test_writeBinaryFileToEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Binary File to EEPROM with Oversize file
void test_writeBinaryFileToEEPROMWithOversizeFile(){
    init_test();

    eeprom.forceWrite = 1;
    fclose(romFile);
    init_test_romFile(oversizeFilename);

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// QUICK TESTS

// TEST - Quick Write Binary File to EEPROM
void test_quickWriteBinaryFileToEEPROM(){
    init_test();

    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + get_file_size(romFile) + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Negative start value
void test_quickWriteBinaryFileToEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.startValue = -1;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with start value
void test_quickWriteBinaryFileToEEPROMWithStartValue(){
    init_test();

    eeprom.startValue = 87;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Limit
void test_quickWriteBinaryFileToEEPROMWithLimit(){
    init_test();

    eeprom.limit = 145;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.limit + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with start value and limit
void test_quickWriteBinaryFileToEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.startValue = 120;
    eeprom.limit = 145;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
   
    expected = eeprom.limit - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Excess Limit
void test_quickWriteBinaryFileToEEPROMWithExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Paged Excess Limit
void test_quickWriteBinaryFileToEEPROMWithPagedExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Start Value and Excess Limit
void test_quickWriteBinaryFileToEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Start Value and Paged Excess Limit
void test_quickWriteBinaryFileToEEPROMWithStartValuePagedExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Start Value and Limit Less than Page
void test_quickWriteBinaryFileToEEPROMWithStartValueLimitLessThanPage(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 5;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.limit - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Start Value and Excess Limit Less than Page
void test_quickWriteBinaryFileToEEPROMWithStartValueExcessLimitLessThanPage(){
    init_test();

    eeprom.startValue = eeprom.size - 3;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 1;
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Write Binary File to EEPROM with Oversize file
void test_quickWriteBinaryFileToEEPROMWithOversizeFile(){
    init_test();

    fclose(romFile);
    init_test_romFile(oversizeFilename);
    eeprom.quick = 1;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}


/******************************************************************************/
// SUITE - Compare Binary File to EEPROM
/******************************************************************************/
// TEST - Default Compare Binary File to EEPROM
void test_compareBinaryFileToEEPROM(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + get_file_size(romFile);

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Binary File to EEPROM with Negative start value
void test_compareBinaryFileToEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = -1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Binary File to EEPROM with start value
void test_compareBinaryFileToEEPROMWithStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 87;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + eeprom.size - eeprom.startValue;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Binary File to EEPROM with Limit
void test_compareBinaryFileToEEPROMWithLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = 145;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + eeprom.limit;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Binary File to EEPROM with start value and limit
void test_compareBinaryFileToEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 120;
    eeprom.limit = 145;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + eeprom.limit - eeprom.startValue;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Binary File to EEPROM with Excess Limit
void test_compareBinaryFileToEEPROMWithExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = eeprom.size + 9;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Binary File to EEPROM with Start Value and Excess Limit
void test_compareBinaryFileToEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Binary File to EEPROM with Oversize file
void test_compareBinaryFileToEEPROMWithOversizeFile(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    fclose(romFile);
    init_test_romFile(oversizeFilename);

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Default Compare Unmatched Binary File to EEPROM
void test_compareUnmatchedBinaryFileToEEPROM(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);

    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (get_file_size(romFile)*2);

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Binary File to EEPROM with Negative start value
void test_compareUnmatchedBinaryFileToEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = -1;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Binary File to EEPROM with start value
void test_compareUnmatchedBinaryFileToEEPROMWithStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 87;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (eeprom.size - eeprom.startValue)*2;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Binary File to EEPROM with Limit
void test_compareUnmatchedBinaryFileToEEPROMWithLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = 145;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (eeprom.limit*2);

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Binary File to EEPROM with start value and limit
void test_compareUnmatchedBinaryFileToEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 120;
    eeprom.limit = 145;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (eeprom.limit - eeprom.startValue)*2;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Binary File to EEPROM with Excess Limit
void test_compareUnmatchedBinaryFileToEEPROMWithExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = eeprom.size + 9;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size*2) + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Binary File to EEPROM with Start Value and Excess Limit
void test_compareUnmatchedBinaryFileToEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// QUICK TESTS

// TEST - Quick Compare Binary File to EEPROM
void test_quickCompareBinaryFileToEEPROM(){
    init_test();
    
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + get_file_size(romFile) + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Negative start value
void test_quickCompareBinaryFileToEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.startValue = -1;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with start value
void test_quickCompareBinaryFileToEEPROMWithStartValue(){
    init_test();

    eeprom.startValue = 87;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Limit
void test_quickCompareBinaryFileToEEPROMWithLimit(){
    init_test();

    eeprom.limit = 145;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.limit + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with start value and limit
void test_quickCompareBinaryFileToEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.startValue = 120;
    eeprom.limit = 145;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.limit - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Excess Limit
void test_quickCompareBinaryFileToEEPROMWithExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Paged Excess Limit
void test_quickCompareBinaryFileToEEPROMWithPagedExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Start Value and Excess Limit
void test_quickCompareBinaryFileToEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Start Value and Paged Excess Limit
void test_quickCompareBinaryFileToEEPROMWithStartValuePagedExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Start Value and Limit Less than Page
void test_quickCompareBinaryFileToEEPROMWithStartValueLimitLessThanPage(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 5;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.limit - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Start Value and Excess Limit Less than Page
void test_quickCompareBinaryFileToEEPROMWithStartValueExcessLimitLessThanPage(){
    init_test();

    eeprom.startValue = eeprom.size - 3;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 1;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Binary File to EEPROM with Oversize file
void test_quickCompareBinaryFileToEEPROMWithOversizeFile(){
    init_test();

    fclose(romFile);
    init_test_romFile(oversizeFilename);
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM
void test_quickCompareUnmatchedBinaryFileToEEPROM(){
    init_test();
    
    eeprom.quick = 1;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + get_file_size(romFile)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Negative start value
void test_quickCompareUnmatchedBinaryFileToEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.startValue = -1;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with start value
void test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValue(){
    init_test();

    eeprom.startValue = 87;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Limit
void test_quickCompareUnmatchedBinaryFileToEEPROMWithLimit(){
    init_test();

    eeprom.limit = 145;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.limit*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with start value and limit
void test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.startValue = 120;
    eeprom.limit = 145;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.limit - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Excess Limit
void test_quickCompareUnmatchedBinaryFileToEEPROMWithExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Paged Excess Limit
void test_quickCompareUnmatchedBinaryFileToEEPROMWithPagedExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Start Value and Excess Limit
void test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Start Value and Paged Excess Limit
void test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValuePagedExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Start Value and Limit Less than Page
void test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueLimitLessThanPage(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 5;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.limit - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Compare Unmatched Binary File to EEPROM with Start Value and Excess Limit Less than Page
void test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueExcessLimitLessThanPage(){
    init_test();

    eeprom.startValue = eeprom.size - 3;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 1;
    eeprom.quick = 1;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}


/******************************************************************************/
// SUITE - Write Text File to EEPROM
/******************************************************************************/
// TEST - Force Write Text File to EEPROM
void test_forceWriteTextFileToEEPROM(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + eeprom.limit-eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Default Write Text File to EEPROM
void test_writeTextFileToEEPROM(){
    filename = textFilename;
    init_test();

    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Text File to EEPROM with Negative start value
void test_writeTextFileToEEPROMWithNegativeStartValue(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = -1;
    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter;
    actual_result += compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = 0 + 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Text File to EEPROM with start value
void test_writeTextFileToEEPROMWithStartValue(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 87;
    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Text File to EEPROM with Limit
void test_writeTextFileToEEPROMWithLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.limit = 145;
    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.limit + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Text File to EEPROM with start value and limit
void test_writeTextFileToEEPROMWithStartValueAndLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 120;
    eeprom.limit = 145;
    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.limit - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Text File to EEPROM with Excess Limit
void test_writeTextFileToEEPROMWithExcessLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.limit = eeprom.size + 9;
    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Text File to EEPROM with Start Value and Excess Limit
void test_writeTextFileToEEPROMWithStartValueExcessLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    eeprom.fileType = TEXT_FILE;

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Text File to EEPROM with Oversize file
void test_writeTextFileToEEPROMWithOversizeFile(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.fileType = TEXT_FILE;
    fclose(romFile);
    init_test_romFile(oversizeTextFilename);

    actual_result = writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteWriteCounter + compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}


/******************************************************************************/
// SUITE - Compare Text File to EEPROM
/******************************************************************************/
// TEST - Default Compare Matching Text File to EEPROM
void test_compareMatchingTextFileToEEPROM(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (eeprom.limit - eeprom.startValue);

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Matching Text File to EEPROM with Negative start value
void test_compareMatchingTextFileToEEPROMWithNegativeStartValue(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = -1;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Matching Text File to EEPROM with start value
void test_compareMatchingTextFileToEEPROMWithStartValue(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 87;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + eeprom.size - eeprom.startValue;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Matching Text File to EEPROM with Limit
void test_compareMatchingTextFileToEEPROMWithLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = 145;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + eeprom.limit;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Matching Text File to EEPROM with start value and limit
void test_compareMatchingTextFileToEEPROMWithStartValueAndLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 120;
    eeprom.limit = 145;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + eeprom.limit - eeprom.startValue;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Matching Text File to EEPROM with Excess Limit
void test_compareMatchingTextFileToEEPROMWithExcessLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = eeprom.size + 9;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Matching Text File to EEPROM with Start Value and Excess Limit
void test_compareMatchingTextFileToEEPROMWithStartValueExcessLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Matching Text File to EEPROM with Oversize file
void test_compareMatchingTextFileToEEPROMWithOversizeFile(){
    filename = oversizeTextFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.fileType = TEXT_FILE;

    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    rewind(romFile);
    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = eeprom.size - eeprom.startValue + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Default Compare Unmatched Text File to EEPROM
void test_compareUnmatchedTextFileToEEPROM(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.fileType = TEXT_FILE;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);

    fclose(romFile);
    init_test_romFile(nonMatchingTextFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + ((eeprom.limit - eeprom.startValue)*2);

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Text File to EEPROM with start value
void test_compareUnmatchedTextFileToEEPROMWithStartValue(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 87;
    eeprom.fileType = TEXT_FILE;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingTextFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (eeprom.size - eeprom.startValue)*2;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Text File to EEPROM with Limit
void test_compareUnmatchedTextFileToEEPROMWithLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = 145;
    eeprom.fileType = TEXT_FILE;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingTextFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (eeprom.limit*2);

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Text File to EEPROM with start value and limit
void test_compareUnmatchedTextFileToEEPROMWithStartValueAndLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 120;
    eeprom.limit = 145;
    eeprom.fileType = TEXT_FILE;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingTextFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = 0 + (eeprom.limit - eeprom.startValue)*2;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Text File to EEPROM with Excess Limit
void test_compareUnmatchedTextFileToEEPROMWithExcessLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.limit = eeprom.size + 9;
    eeprom.fileType = TEXT_FILE;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingTextFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size*2) + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Compare Unmatched Text File to EEPROM with Start Value and Excess Limit
void test_compareUnmatchedTextFileToEEPROMWithStartValueExcessLimit(){
    filename = textFilename;
    init_test();

    eeprom.forceWrite = 1;
    eeprom.validateWrite = 0;
    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    eeprom.fileType = TEXT_FILE;
    
    writeFileToEEPROM(&gpioConfig,&eeprom,romFile);
    
    fclose(romFile);
    init_test_romFile(nonMatchingTextFilename);

    actual_result = compareFileToEEPROM(&gpioConfig,&eeprom,romFile);
    actual_result += eeprom.byteReadCounter;
    expected = (eeprom.size - eeprom.startValue)*2 + 0;

    expect(expected, actual_result);
    
    cleanup_test();
}


/******************************************************************************/
// SUITE - Write Byte to EEPROM
/******************************************************************************/
// TEST - Write Random byte at Random Address to EEPROM
void test_writeRandomByteToRandomAddressToEEPROM(){
    init_test();

    address = get_random_data(eeprom.size);
    data = get_random_data(255);

    writeByteToAddress(&gpioConfig,&eeprom,address,data);
    actual_result = readByteFromAddress(&gpioConfig,&eeprom,address);
    expected = data;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Random byte at Max Address to EEPROM
void test_writeRandomByteToMaxAddressToEEPROM(){
    init_test();

    address = eeprom.size-1;
    data = get_random_data(255);

    writeByteToAddress(&gpioConfig,&eeprom,address,data);
    actual_result = readByteFromAddress(&gpioConfig,&eeprom,address);
    expected = data;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Random byte above Max Address to EEPROM
void test_writeRandomByteAboveMaxAddressToEEPROM(){
    init_test();

    address = eeprom.size;
    data = get_random_data(255);

    writeByteToAddress(&gpioConfig,&eeprom,address,data);
    actual_result = readByteFromAddress(&gpioConfig,&eeprom,address);
    expected = -1;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Random byte at First Address to EEPROM
void test_writeRandomByteToFirstAddressToEEPROM(){
    init_test();

    address = 0;
    data = get_random_data(255);

    writeByteToAddress(&gpioConfig,&eeprom,address,data);
    actual_result = readByteFromAddress(&gpioConfig,&eeprom,address);
    expected = data;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Random byte at Random Address to EEPROM with Write Delay
void test_writeRandomByteToRandomAddressToEEPROMWithWriteDelay(){
    init_test();

    address = get_random_data(eeprom.size);
    data = get_random_data(255);

    writeByteToAddress(&gpioConfig,&eeprom,address,data);
    actual_result = readByteFromAddress(&gpioConfig,&eeprom,address);
    expected = data;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Write Negative byte at Random Address to EEPROM
void test_writeNegativeByteToRandomAddressToEEPROM(){
    init_test();

    address = get_random_data(eeprom.size);
    data = -1;

    writeByteToAddress(&gpioConfig,&eeprom,address,data);
    actual_result = readByteFromAddress(&gpioConfig,&eeprom,address);
    expected = data;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

/******************************************************************************/
// SUITE - Erase EEPROM
/******************************************************************************/
// TEST - Erase EEPROM with 0
void test_eraseEEPROMWith0(){
    init_test();

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,0);
    expected = eeprom.size;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Force Erase EEPROM
void test_forceEraseEEPROM(){
    init_test();

    eeprom.forceWrite = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Default Erase EEPROM
void test_defaultEraseEEPROM(){
    init_test();

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += 0;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Erase EEPROM with Negative start value
void test_eraseEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = -1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    expected = 0;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Erase EEPROM with start value
void test_eraseEEPROMWithStartValue(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 87;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size-eeprom.startValue;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Erase EEPROM with Limit
void test_eraseEEPROMWithLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.limit = 145;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.limit; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.limit;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Erase EEPROM with start value and limit
void test_eraseEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 120;
    eeprom.limit = 145;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.limit; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.limit - eeprom.startValue;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Erase EEPROM with Excess Limit
void test_eraseEEPROMWithExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.limit = eeprom.size + 9;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Erase EEPROM with Start Value and Excess Limit
void test_eraseEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.forceWrite = 1;
    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size-eeprom.startValue;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// QUICK TESTS

// TEST - Erase EEPROM
void test_quickEraseEEPROM(){
    init_test();

    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Erase EEPROM with Negative start value
void test_quickEraseEEPROMWithNegativeStartValue(){
    init_test();

    eeprom.startValue = -1;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    expected = 0;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with start value
void test_quickEraseEEPROMWithStartValue(){
    init_test();

    eeprom.startValue = 87;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size-eeprom.startValue;
    
    expect(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with Limit
void test_quickEraseEEPROMWithLimit(){
    init_test();

    eeprom.limit = 145;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.limit; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.limit;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with start value and limit
void test_quickEraseEEPROMWithStartValueAndLimit(){
    init_test();

    eeprom.startValue = 120;
    eeprom.limit = 145;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.limit; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.limit - eeprom.startValue;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with Excess Limit
void test_quickEraseEEPROMWithExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with Paged Excess Limit
void test_quickEraseEEPROMWithPagedExcessLimit(){
    init_test();

    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=0; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with Start Value and Excess Limit
void test_quickEraseEEPROMWithStartValueExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + 9;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size - eeprom.startValue;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with Start Value and Paged Excess Limit
void test_quickEraseEEPROMWithStartValuePagedExcessLimit(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.size + eeprom.pageSize + 9;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size - eeprom.startValue;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with Start Value and Limit Less than Page
void test_quickEraseEEPROMWithStartValueLimitLessThanPage(){
    init_test();

    eeprom.startValue = 140;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 5;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.limit; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.limit - eeprom.startValue;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}

// TEST - Quick Erase EEPROM with Start Value and Excess Limit Less than Page
void test_quickEraseEEPROMWithStartValueExcessLimitLessThanPage(){
    init_test();

    eeprom.startValue = eeprom.size - 3;
    eeprom.limit = eeprom.startValue + eeprom.pageSize - 1;
    eeprom.quick = 1;

    actual_result = eraseEEPROM(&gpioConfig,&eeprom,options.eraseByte);
    actual_result += eeprom.byteWriteCounter;
    
    for(int i=eeprom.startValue; i < eeprom.size; i++){
        if(readByteFromAddress(&gpioConfig,&eeprom,i) != options.eraseByte){
            ++expected;
        }
    }
    expected += eeprom.size - eeprom.startValue;
    
    expectNot(expected, actual_result);
    
    cleanup_test();
}


/******************************************************************************/
// TEST RUNNER
/******************************************************************************/
int run_piepro_functional_tests(int testType, int eepromModelToTest, int generateData, \
                                int suiteToRun, int testToRun, int limit){
    char* suite0 = "suite_writeBinaryFileToEEPROM";
    char* suite1 = "suite_compareBinaryFileToEEPROM";
    char* suite2 = "suite_writeTextFileToEEPROM";
    char* suite3 = "suite_compareTextFileToEEPROM";
    char* suite4 = "suite_writeByteToEEPROM";
    char* suite5 = "suite_eraseEEPROM";

    i2cDevice = testType;

    if(eepromModel != END){
        eepromModel = eepromModelToTest;
    } else {
        fprintf(stderr,"No EEPROM model specified. Please Specify an EEPROM model with the -m or --model flag.\n");
    }

    if(limit != -1){
        printf("Setting articfical limit to: %i\n", limit);
        useLimit = 1;
        limitSize = limit;
    } else {
        limitSize = EEPROM_MODEL_SIZE[eepromModel];
    }

    reset_filenames();

    if(generateData){
        mkdir(defaultPath, 0755);
        generateTestData(defaultFilename, limitSize);
        if(generateData == 2){
            return 0;
        }
    }
    
    addFuncSuite(suite0);
    addFuncTest("Force Write Binary File to EEPROM", getCurrentFuncSuite(), test_forceWriteBinaryFileToEEPROM);
    addFuncTest("Default Write Binary File to EEPROM", getCurrentFuncSuite(), test_writeBinaryFileToEEPROM);
    addFuncTest("Write Binary File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_writeBinaryFileToEEPROMWithNegativeStartValue);
    addFuncTest("Write Binary File to EEPROM with Start value", getCurrentFuncSuite(), test_writeBinaryFileToEEPROMWithStartValue);
    addFuncTest("Write Binary File to EEPROM with Limit", getCurrentFuncSuite(), test_writeBinaryFileToEEPROMWithLimit);
    addFuncTest("Write Binary File to EEPROM with Start value and limit", getCurrentFuncSuite(), test_writeBinaryFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Write Binary File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_writeBinaryFileToEEPROMWithExcessLimit);
    addFuncTest("Write Binary File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_writeBinaryFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Write Binary File to EEPROM with Oversize file", getCurrentFuncSuite(), test_writeBinaryFileToEEPROMWithOversizeFile);
    addFuncTest("Quick Write Binary File to EEPROM", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROM);
    addFuncTest("Quick Write Binary File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithNegativeStartValue);
    addFuncTest("Quick Write Binary File to EEPROM with Start value", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithStartValue);
    addFuncTest("Quick Write Binary File to EEPROM with Limit", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithLimit);
    addFuncTest("Quick Write Binary File to EEPROM with Start value and limit",  getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Quick Write Binary File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithExcessLimit);
    addFuncTest("Quick Write Binary File to EEPROM with Paged Excess Limit", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithPagedExcessLimit);
    addFuncTest("Quick Write Binary File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Quick Write Binary File to EEPROM with Start Value and Paged Excess Limit", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithStartValuePagedExcessLimit);
    addFuncTest("Quick Write Binary File to EEPROM with Start Value and Limit Less than Page", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithStartValueLimitLessThanPage);
    addFuncTest("Quick Write Binary File to EEPROM with Start Value and Excess Limit Less than Page", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithStartValueExcessLimitLessThanPage);
    addFuncTest("Quick Write Binary File to EEPROM with Oversize file", getCurrentFuncSuite(), test_quickWriteBinaryFileToEEPROMWithOversizeFile);

    addFuncSuite(suite1);
    addFuncTest("Default Compare Binary File to EEPROM", getCurrentFuncSuite(), test_compareBinaryFileToEEPROM);
    addFuncTest("Compare Binary File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_compareBinaryFileToEEPROMWithNegativeStartValue);
    addFuncTest("Compare Binary File to EEPROM with Start value", getCurrentFuncSuite(), test_compareBinaryFileToEEPROMWithStartValue);
    addFuncTest("Compare Binary File to EEPROM with Limit", getCurrentFuncSuite(), test_compareBinaryFileToEEPROMWithLimit);
    addFuncTest("Compare Binary File to EEPROM with Start value and limit", getCurrentFuncSuite(), test_compareBinaryFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Compare Binary File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_compareBinaryFileToEEPROMWithExcessLimit);
    addFuncTest("Compare Binary File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_compareBinaryFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Compare Binary File to EEPROM with Oversize file", getCurrentFuncSuite(), test_compareBinaryFileToEEPROMWithOversizeFile);
    addFuncTest("Default Compare Unmatched Binary File to EEPROM", getCurrentFuncSuite(), test_compareUnmatchedBinaryFileToEEPROM);
    addFuncTest("Compare Unmatched Binary File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_compareUnmatchedBinaryFileToEEPROMWithNegativeStartValue);
    addFuncTest("Compare Unmatched Binary File to EEPROM with Start value", getCurrentFuncSuite(), test_compareUnmatchedBinaryFileToEEPROMWithStartValue);
    addFuncTest("Compare Unmatched Binary File to EEPROM with Limit", getCurrentFuncSuite(), test_compareUnmatchedBinaryFileToEEPROMWithLimit);
    addFuncTest("Compare Unmatched Binary File to EEPROM with Start value and limit", getCurrentFuncSuite(), test_compareUnmatchedBinaryFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Compare Unmatched Binary File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_compareUnmatchedBinaryFileToEEPROMWithExcessLimit);
    addFuncTest("Compare Unmatched Binary File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_compareUnmatchedBinaryFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Quick Compare Binary File to EEPROM", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROM);
    addFuncTest("Quick Compare Binary File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithNegativeStartValue);
    addFuncTest("Quick Compare Binary File to EEPROM with Start value", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithStartValue);
    addFuncTest("Quick Compare Binary File to EEPROM with Limit", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithLimit);
    addFuncTest("Quick Compare Binary File to EEPROM with Start value and limit",  getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Quick Compare Binary File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithExcessLimit);
    addFuncTest("Quick Compare Binary File to EEPROM with Paged Excess Limit", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithPagedExcessLimit);
    addFuncTest("Quick Compare Binary File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Quick Compare Binary File to EEPROM with Start Value and Paged Excess Limit", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithStartValuePagedExcessLimit);
    addFuncTest("Quick Compare Binary File to EEPROM with Start Value and Limit Less than Page", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithStartValueLimitLessThanPage);
    addFuncTest("Quick Compare Binary File to EEPROM with Start Value and Excess Limit Less than Page", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithStartValueExcessLimitLessThanPage);
    addFuncTest("Quick Compare Binary File to EEPROM with Oversize file", getCurrentFuncSuite(), test_quickCompareBinaryFileToEEPROMWithOversizeFile);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROM);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithNegativeStartValue);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Start value", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValue);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Limit", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithLimit);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Start value and limit",  getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithExcessLimit);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Paged Excess Limit", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithPagedExcessLimit);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Start Value and Paged Excess Limit", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValuePagedExcessLimit);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Start Value and Limit Less than Page", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueLimitLessThanPage);
    addFuncTest("Quick Compare Unmatched Binary File to EEPROM with Start Value and Excess Limit Less than Page", getCurrentFuncSuite(), test_quickCompareUnmatchedBinaryFileToEEPROMWithStartValueExcessLimitLessThanPage);

    addFuncSuite(suite2);
    addFuncTest("Force Write Text File to EEPROM", getCurrentFuncSuite(), test_forceWriteTextFileToEEPROM);
    addFuncTest("Default Write Text File to EEPROM", getCurrentFuncSuite(), test_writeTextFileToEEPROM);
    addFuncTest("Write Text File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_writeTextFileToEEPROMWithNegativeStartValue);
    addFuncTest("Write Text File to EEPROM with Start value", getCurrentFuncSuite(), test_writeTextFileToEEPROMWithStartValue);
    addFuncTest("Write Text File to EEPROM with Limit", getCurrentFuncSuite(), test_writeTextFileToEEPROMWithLimit);
    addFuncTest("Write Text File to EEPROM with Start value and limit", getCurrentFuncSuite(), test_writeTextFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Write Text File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_writeTextFileToEEPROMWithExcessLimit);
    addFuncTest("Write Text File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_writeTextFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Write Text File to EEPROM with Oversize file", getCurrentFuncSuite(), test_writeTextFileToEEPROMWithOversizeFile);

    addFuncSuite(suite3);
    addFuncTest("Default Compare Matching Text File to EEPROM", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROM);
    addFuncTest("Compare Matching Text File to EEPROM with Negative Start value", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROMWithNegativeStartValue);
    addFuncTest("Compare Matching Text File to EEPROM with Start value", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROMWithStartValue);
    addFuncTest("Compare Matching Text File to EEPROM with Limit", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROMWithLimit);
    addFuncTest("Compare Matching Text File to EEPROM with Start value and limit", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Compare Matching Text File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROMWithExcessLimit);
    addFuncTest("Compare Matching Text File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROMWithStartValueExcessLimit);
    addFuncTest("Compare Matching Text File to EEPROM with Oversize file", getCurrentFuncSuite(), test_compareMatchingTextFileToEEPROMWithOversizeFile);
    addFuncTest("Default Compare Unmatched Text File to EEPROM", getCurrentFuncSuite(), test_compareUnmatchedTextFileToEEPROM);
    addFuncTest("Compare Unmatched Text File to EEPROM with Start value", getCurrentFuncSuite(), test_compareUnmatchedTextFileToEEPROMWithStartValue);
    addFuncTest("Compare Unmatched Text File to EEPROM with Limit", getCurrentFuncSuite(), test_compareUnmatchedTextFileToEEPROMWithLimit);
    addFuncTest("Compare Unmatched Text File to EEPROM with Start value and limit", getCurrentFuncSuite(), test_compareUnmatchedTextFileToEEPROMWithStartValueAndLimit);
    addFuncTest("Compare Unmatched Text File to EEPROM with Excess Limit", getCurrentFuncSuite(), test_compareUnmatchedTextFileToEEPROMWithExcessLimit);
    addFuncTest("Compare Unmatched Text File to EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_compareUnmatchedTextFileToEEPROMWithStartValueExcessLimit);

    addFuncSuite(suite4);
    addFuncTest("Write Random byte at Random Address to EEPROM", getCurrentFuncSuite(), test_writeRandomByteToRandomAddressToEEPROM);
    addFuncTest("Write Random byte at Max Address to EEPROM", getCurrentFuncSuite(), test_writeRandomByteToMaxAddressToEEPROM);
    addFuncTest("Write Random byte above Max Address to EEPROM", getCurrentFuncSuite(), test_writeRandomByteAboveMaxAddressToEEPROM);
    addFuncTest("Write Random byte at First Address to EEPROM", getCurrentFuncSuite(), test_writeRandomByteToFirstAddressToEEPROM);
    addFuncTest("Write Random byte at Random Address to EEPROM with Write Delay", getCurrentFuncSuite(), test_writeRandomByteToRandomAddressToEEPROMWithWriteDelay);
    addFuncTest("Write Negative byte at Random Address to EEPROM", getCurrentFuncSuite(), test_writeNegativeByteToRandomAddressToEEPROM);

    addFuncSuite(suite5);
    addFuncTest("Erase EEPROM with 0", getCurrentFuncSuite(), test_eraseEEPROMWith0);
    addFuncTest("Force Erase EEPROM", getCurrentFuncSuite(), test_forceEraseEEPROM);
    addFuncTest("Default Erase EEPROM", getCurrentFuncSuite(), test_defaultEraseEEPROM);
    addFuncTest("Erase EEPROM with Negative Start value", getCurrentFuncSuite(), test_eraseEEPROMWithNegativeStartValue);
    addFuncTest("Erase EEPROM with Start value", getCurrentFuncSuite(), test_eraseEEPROMWithStartValue);
    addFuncTest("Erase EEPROM with Limit", getCurrentFuncSuite(), test_eraseEEPROMWithLimit);
    addFuncTest("Erase EEPROM with Start value and limit", getCurrentFuncSuite(), test_eraseEEPROMWithStartValueAndLimit);
    addFuncTest("Erase EEPROM with Excess Limit", getCurrentFuncSuite(), test_eraseEEPROMWithExcessLimit);
    addFuncTest("Erase EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_eraseEEPROMWithStartValueExcessLimit);
    addFuncTest("Quick Erase EEPROM", getCurrentFuncSuite(), test_quickEraseEEPROM);
    addFuncTest("Quick Erase EEPROM with Negative Start value", getCurrentFuncSuite(), test_quickEraseEEPROMWithNegativeStartValue);
    addFuncTest("Quick Erase EEPROM with Start value", getCurrentFuncSuite(), test_quickEraseEEPROMWithStartValue);
    addFuncTest("Quick Erase EEPROM with Limit", getCurrentFuncSuite(), test_quickEraseEEPROMWithLimit);
    addFuncTest("Quick Erase EEPROM with Start value and limit",  getCurrentFuncSuite(), test_quickEraseEEPROMWithStartValueAndLimit);
    addFuncTest("Quick Erase EEPROM with Excess Limit", getCurrentFuncSuite(), test_quickEraseEEPROMWithExcessLimit);
    addFuncTest("Quick Erase EEPROM with Paged Excess Limit", getCurrentFuncSuite(), test_quickEraseEEPROMWithPagedExcessLimit);
    addFuncTest("Quick Erase EEPROM with Start Value and Excess Limit", getCurrentFuncSuite(), test_quickEraseEEPROMWithStartValueExcessLimit);
    addFuncTest("Quick Erase EEPROM with Start Value and Paged Excess Limit", getCurrentFuncSuite(), test_quickEraseEEPROMWithStartValuePagedExcessLimit);
    addFuncTest("Quick Erase EEPROM with Start Value and Limit Less than Page", getCurrentFuncSuite(), test_quickEraseEEPROMWithStartValueLimitLessThanPage);
    addFuncTest("Quick Erase EEPROM with Start Value and Excess Limit Less than Page", getCurrentFuncSuite(), test_quickEraseEEPROMWithStartValueExcessLimitLessThanPage);


    addFuncTestInit(reset_filenames);
    runFuncTests(suiteToRun, testToRun);

    return getTestFailCounter();
}