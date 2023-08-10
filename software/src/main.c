#include <stdio.h>
#include <inttypes.h>

#include "utils.h"
#include "piepro.h"



int main(int argc, char *argv[]){
	
	// TODO: Setup data Polling
	// TODO: Support Page Writes if supported
	// TODO: Restructure Project
	// TODO: Convert from wiringPi to chardev GPIO
	// TODO: Update docs for new build and installation min 4.8 kernel
	// TODO: Add make install
	// TODO: Add docs for hardware
	// TODO: Multi .gitignore
	
    uint8_t error = 0;
	if (argc == 1){
		printHelp();
        ulog(ERROR,"Filename to open must be specified. Exiting now");
		return 1;
	} else {

        /*********************************************************************/
        /************************* Init Program ******************************/
        FILE *romFile;
        struct EEPROM eeprom;
        struct OPTIONS sOptions;
		error = parseCommandLineOptions(&sOptions,argc,argv);

    
        // init
        if (sOptions.eepromModel >= AT24C01 && sOptions.eepromModel <= AT24C512){
            eeprom.fd = wiringPiI2CSetup(sOptions.i2cId);		
        } else {
            if (-1 == wiringPiSetup()) {
                ulog(FATAL,"Failed to setup Wiring Pi!");
                return 1;
            }
        }	

        eeprom.i2cId = sOptions.i2cId;
        eeprom.writeCycleWait = sOptions.writeCycleUSec;
        if ( 0 != init(&eeprom, sOptions.eepromModel)){
            return 1;
        }


        /*********************************************************************/
        /************************* Program  Start ****************************/
        switch(sOptions.action){
            case WRITE_FILE_TO_ROM: case COMPARE_ROM_TO_FILE:
                // open file to read
                romFile = fopen(sOptions.filename, "r");
                if(romFile == NULL){
                    ulog(FATAL,"Error Opening File");
                    return 1;
                }

                switch(sOptions.fileType | (sOptions.action<<8)){
                    case TEXT_FILE| (WRITE_FILE_TO_ROM<<8):
                        writeTextFileToEEPROM(&eeprom,romFile, &sOptions);
                        break;
                    case BINARY_FILE | (WRITE_FILE_TO_ROM<<8):
                        writeBinaryFileToEEPROM(&eeprom,romFile,&sOptions);
                        break;
                    case TEXT_FILE | (COMPARE_ROM_TO_FILE<<8):
                        if(compareTextFileToEEPROM(&eeprom,romFile, &sOptions)){
                            ulog(ERROR,"EEPROM does not match file");
                        }
                        break;
                    case BINARY_FILE | (COMPARE_ROM_TO_FILE<<8):
                        if(compareBinaryFileToEEPROM(&eeprom,romFile,&sOptions)){
                            ulog(ERROR,"EEPROM does not match file");
                        }
                        break;
                }
                
                fclose(romFile);
                break;
            case DUMP_ROM:
                printROMContents(&eeprom,&sOptions);
                break;
            case WRITE_SINGLE_BYTE_TO_ROM:
                writeByteToAddress(&eeprom,sOptions.addressParam,sOptions.dataParam,&sOptions,NULL);
                break;
        }
        /*********************************************************************/
    }
	return error;
}