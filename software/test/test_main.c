#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/piepro.h" // For END and EEPROM_MODEL_STRINGS

int main(int argc,char *argv[]){
    int testType = 0;
    int eepromModel = END;
    int generateData = 0;
    int suiteToRun = -1;
    int testToRun = -1;
    int runUnitTests = 1;
    int runFuncTests = 1;
    int numTestsFailed = 0;
    int limit = -1;
    
    // Parse Command Line Parameters
    for(int i = 1; i < argc; i++){
        // -f --functional
        if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--functional")){
            runUnitTests = 0;
        }

        // -g --generate
        if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "--generate")){
            generateData = 1;
        }

        // -go --generate-only
        if (!strcmp(argv[i], "-go") || !strcmp(argv[i], "--generate-only")){
            generateData = 2;
        }

        // -i2c
        if (!strcmp(argv[i], "-i2c")){
            testType = 1;
        }

        // -l --limit
        if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--limit")){
            if (i != argc-1) {
                limit = (int)strtol(argv[i+1], NULL, 0);
            } else {
                printf("%s Flag must have a value specified\n", argv[i]);
                return 1;
            }
        }

        // -m --model
        if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--model")){
            if (i != argc-1) {
                eepromModel = 0;
                while(eepromModel < END && strcmp(argv[i+1], EEPROM_MODEL_STRINGS[eepromModel])){
                    eepromModel++;
                }
                if(eepromModel == END){
                    printf("Unsupported EEPROM Model\n");
                    return 1;
                }
            } else {
                printf("%s Flag must have a model specified\n", argv[i]);
                return 1;
            }
        }

        // -s --suite
        if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--suite")){
            if (i != argc-1) {
                suiteToRun = (int)strtol(argv[i+1], NULL, 0);
            } else {
                printf("%s Flag must have a value specified\n", argv[i]);
                return 1;
            }
        }

        // -t --test
        if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--test")){
            if (i != argc-1) {
                testToRun = (int)strtol(argv[i+1], NULL, 0);
            } else {
                printf("%s Flag must have a value specified\n", argv[i]);
                return 1;
            }
        }

        // -u --unit
        if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--unit")){
            runFuncTests = 0;
        }
    }

    // Make sure EEPROM Model is set
    if(eepromModel == END){
        fprintf(stderr,"No EEPROM model specified. Please Specify an EEPROM model with the -m or --model flag.\n");
        return 1;
    }

    // Determine which tests to run
    if(testType && runUnitTests){
        numTestsFailed = run_I2C_tests(suiteToRun, testToRun);
    } else if(runUnitTests){
        numTestsFailed = run_gpio_tests(suiteToRun, testToRun);
    }
    if(runFuncTests){
        numTestsFailed = run_piepro_functional_tests(testType, eepromModel, generateData, suiteToRun, testToRun, limit);
    }
    
    // Print the results of the tests
    printResults();

    // Return the number of failed tests
    return numTestsFailed;
}