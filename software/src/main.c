#include <stdio.h>

#include "piepro.h"

int main(int argc, char *argv[]){	
    int error = 0;
	
    if (argc == 1){
		printHelp();
		error = -1;
	} else {
        /*********************************************************************/
        /************************* Init Program ******************************/
        struct EEPROM eeprom;
        struct GPIO_CONFIG gpioConfig;
        struct OPTIONS options;
		
        if(parseCommandLineOptions(&options, argc, argv)){
            fprintf(stderr,"Error parsing command line options\n");
            return -1;
        }
        
        if(initHardware(&options, &eeprom, &gpioConfig)){
            fprintf(stderr,"Error initializing hardware\n");
            return -1;
        }
        
        /*********************************************************************/
        /************************* Program  Start ****************************/
        switch(options.action){
            case WRITE_FILE_TO_ROM:
            case COMPARE_FILE_TO_ROM:
            {
                // open file to read
                FILE* romFile = fopen(options.filename, "r");
                if(romFile == NULL){
                    fprintf(stderr,"Error Opening File\n");
                    error = -1;
                    break;
                }
                if(options.action == WRITE_FILE_TO_ROM){
                    char confirmation = 'n';
                    if(options.promptUser){
                        printf("Are you sure you want to write to the EEPROM? y/N\n");
                        confirmation = getchar();  
                    }    
                    if(confirmation == 'y' || confirmation == 'Y' || !options.promptUser){
                        error = writeFileToEEPROM(&gpioConfig, &eeprom, romFile);
                        fprintf(stdout,"Wrote %i bytes\n", eeprom.byteWriteCounter);
                    } else {
                        printf("Aborting write operation.\n");
                        break;
                    }

                } else {
                    int bytesNotMatched = compareFileToEEPROM(&gpioConfig, &eeprom, romFile);
                    if(bytesNotMatched == 0) {
                        fprintf(stdout,"All bytes match\n");
                    } else if(bytesNotMatched == -1){
                        fprintf(stdout,"Error comparing file.\n");
                        error = -1;
                    } else {
                        error = -1;
                        fprintf(stderr,"%i bytes do not match\n", bytesNotMatched);
                    }
                }
                fclose(romFile);   
                break;
            }
            case DUMP_ROM:
                error = printEEPROMContents(&gpioConfig, &eeprom, options.dumpFormat);
                break;
            case ERASE_ROM:
            {
                char confirmation = 'n';
                if(options.promptUser){
                    printf("Are you sure you want to erase the EEPROM? y/N\n");
                    confirmation = getchar();  
                }
                if(confirmation == 'y' || confirmation == 'Y' || !options.promptUser){
                    error = eraseEEPROM(&gpioConfig, &eeprom, options.eraseByte);
                } else {
                    printf("Aborting erase operation.\n");
                    break;
                }

                if(error == 0) {
                    fprintf(stdout,"Sucessfully Erased %i bytes in EEPROM with 0x%02x\n", \
                                                                eeprom.byteWriteCounter, options.eraseByte);
                } else {
                    error = -1;
                    fprintf(stdout,"Unable to completely erase EEPROM with 0x%02x\n", options.eraseByte);
                }
                break;
            }
            case WRITE_SINGLE_BYTE_TO_ROM:
                error = writeByteToAddress(&gpioConfig, &eeprom, options.addressParam, options.dataParam);
                if(!error){
                    fprintf(stdout,"Wrote Byte: 0x%02x to Address: 0x%02x\n", options.dataParam, options.addressParam);
                } else {
                    fprintf(stderr,"Error writing Byte: 0x%02x to Address: 0x%02x\n",options.dataParam,options.addressParam);
                }
                break;
            case READ_SINGLE_BYTE_FROM_ROM:
            {
                int readVal = readByteFromAddress(&gpioConfig, &eeprom, options.addressParam);
                if(readVal != -1){
                    fprintf(stdout,"0x%02x\n", readVal);
                } else {
                    error = readVal;
                }
                break;
            }
            case NOTHING:
		        fprintf(stdout,"No action specified. Run piepro -h for a list of options\n");
                break;
        }

        /*********************************************************************/
        /************************ Program Cleanup ****************************/
        cleanupHardware(&gpioConfig, &eeprom);

    }

	return error;
}