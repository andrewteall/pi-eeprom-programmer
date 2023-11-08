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
                // open file to read
                FILE* romFile = fopen(options.filename, "r");
                if(romFile == NULL){
                    fprintf(stderr,"Error Opening File\n");
                    error = -1;
                    break;
                }
                if(options.action == WRITE_FILE_TO_ROM){
                    error = writeFileToEEPROM(&gpioConfig, &eeprom, romFile);
                    fprintf(stdout,"Wrote %i bytes\n", eeprom.byteWriteCounter);
                } else {
                    int bytesNotMatched = compareFileToEEPROM(&gpioConfig, &eeprom, romFile);
                    if(bytesNotMatched == 0) {
                        fprintf(stdout,"All bytes match\n");
                    } else {
                        error = -1;
                        fprintf(stderr,"%i bytes do not match\n", bytesNotMatched);
                    }
                }
                fclose(romFile);
                break;
            case DUMP_ROM:
                printEEPROMContents(&gpioConfig, &eeprom, options.dumpFormat);
                break;
            case WRITE_SINGLE_BYTE_TO_ROM:
                error = writeByteToAddress(&gpioConfig, &eeprom, options.addressParam, options.dataParam);
                if(!error){
                    fprintf(stdout,"Wrote Byte: 0x%02x to Address: 0x%02x\n", options.dataParam, options.addressParam);
                } else {
                    fprintf(stderr,"Error writing Byte: 0x%02x to Address: 0x%02x\n",options.dataParam,options.addressParam);
                }
                break;
            case READ_SINGLE_BYTE_FROM_ROM:
                int readVal = readByteFromAddress(&gpioConfig, &eeprom, options.addressParam);
                if(readVal != -1){
                    fprintf(stdout,"0x%02x\n", readVal);
                } else {
                    error = readVal;
                }
                break;
            case NOTHING:
            default:
		        fprintf(stdout,"No action specified. Run piepro -h for a list of options\n");
                break;
        }

        /*********************************************************************/
        /************************ Program Cleanup ****************************/
        cleanupHardware(&gpioConfig, &eeprom);

    }

	return error;
}