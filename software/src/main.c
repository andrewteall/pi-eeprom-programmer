#include <stdio.h>

#include "piepro.h"

int main(int argc, char *argv[]){	
    int error = 0;
	
    if (argc == 1){
		printHelp();
		return 1;
	} else {
        /*********************************************************************/
        /************************* Init Program ******************************/
        FILE *romFile;
        struct EEPROM eeprom;
        struct GPIO_CONFIG gpioConfig;
        struct OPTIONS sOptions;
		
        if(parseCommandLineOptions(&sOptions,argc,argv)){
            return 1;
        }
        
        if(initHardware(&eeprom, &gpioConfig, &sOptions)){
            return 1;
        }
        /*********************************************************************/
        /************************* Program  Start ****************************/
        switch(sOptions.action){
            case WRITE_FILE_TO_ROM:
            case COMPARE_FILE_TO_ROM:
                // open file to read
                romFile = fopen(sOptions.filename, "r");
                if(romFile == NULL){
                    fprintf(stderr,"Error Opening File\n");
                    return 1;
                }
                if(sOptions.action == WRITE_FILE_TO_ROM){
                    error = writeFileToEEPROM(&gpioConfig, &eeprom, romFile, &sOptions);
                } else {
                    error = compareFileToEEPROM(&gpioConfig, &eeprom, romFile, &sOptions);
                }
                fclose(romFile);
                break;
            case DUMP_ROM:
                printEEPROMContents(&gpioConfig, &eeprom, &sOptions);
                break;
            case WRITE_SINGLE_BYTE_TO_ROM:
                error = writeByteToAddress(&gpioConfig, &eeprom, sOptions.addressParam, sOptions.dataParam);
                break;
            case READ_SINGLE_BYTE_FROM_ROM:
                error = readByteFromAddress(&gpioConfig,&eeprom,sOptions.addressParam);
                if(error != -1){
                    printf("0x%02x\n", error);
                }
                break;
        }
        /*********************************************************************/
        cleanupHardware(&gpioConfig, &eeprom);
    }
    if(error == -1){
        error = 1;
    }
	return error;
}