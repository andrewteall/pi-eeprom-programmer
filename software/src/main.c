#include <stdio.h>
#include <inttypes.h>

// #include "utils.h"
#include "piepro.h"


int main(int argc, char *argv[]){
	
	// TODO: Setup data Polling
	// TODO: Support Page Writes if supported
	// TODO: Update docs for new build and installation min 4.8 kernel
	// TODO: Add docs for hardware
	
    uint8_t error = 0;
	
    if (argc == 1){
		printHelp();
		return 1;
	} else {

        /*********************************************************************/
        /************************* Init Program ******************************/
        FILE *romFile;
        struct EEPROM eeprom;
        struct OPTIONS sOptions;
		
        error = parseCommandLineOptions(&sOptions,argc,argv);
        error = init(&eeprom, sOptions.eepromModel, &sOptions) || error;
        if(error){
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
                    writeFileToEEPROM(&eeprom,romFile, &sOptions);
                } else {
                    compareFileToEEPROM(&eeprom,romFile, &sOptions);
                }
                fclose(romFile);
                break;
            case DUMP_ROM:
                printEEPROMContents(&eeprom,&sOptions);
                break;
            case WRITE_SINGLE_BYTE_TO_ROM:
                writeByteToAddress(&eeprom,sOptions.addressParam,sOptions.dataParam,&sOptions,NULL);
                break;
            case READ_SINGLE_BYTE_FROM_ROM:
                printf("%i\n", readByteFromAddress(&eeprom,sOptions.addressParam));
                break;
        }
        /*********************************************************************/
    }
	return error;
}