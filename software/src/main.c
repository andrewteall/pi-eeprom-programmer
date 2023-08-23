#include <stdio.h>

#include "utils.h"
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
        struct GPIO_CHIP gpioChip;
        struct OPTIONS sOptions;
		
        if(parseCommandLineOptions(&sOptions,argc,argv)){
            return 1;
        }
        
        if(initHardware(&eeprom, &gpioChip, &sOptions)){
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
                    ulog(ERROR,"Error Opening File");
                    return 1;
                }
                if(sOptions.action == WRITE_FILE_TO_ROM){
                    error = writeFileToEEPROM(&gpioChip, &eeprom,romFile, &sOptions);
                } else {
                    error = compareFileToEEPROM(&gpioChip, &eeprom,romFile, &sOptions);
                }
                fclose(romFile);
                break;
            case DUMP_ROM:
                printEEPROMContents(&gpioChip, &eeprom,&sOptions);
                break;
            case WRITE_SINGLE_BYTE_TO_ROM:
                error = writeByteToAddress(&gpioChip, &eeprom,sOptions.addressParam,sOptions.dataParam,&sOptions,NULL);
                break;
            case READ_SINGLE_BYTE_FROM_ROM:
                error = readByteFromAddress(&gpioChip,&eeprom,sOptions.addressParam);
                if(error != -1){
                    printf("0x%2x\n", error);
                }
                break;
        }
        /*********************************************************************/
        cleanupHardware(&gpioChip, &eeprom);
    }
    if(error == -1){
        error = 1;
    }
	return error;
}