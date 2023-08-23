#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "piepro.h"
#include "utils.h"
#include "gpio.h"

static char* chipname = "gpiochip0";
static char* consumer = "Pi EEPROM Programmer";


const char* EEPROM_MODEL_STRINGS[] = 	{
										"xl2816","xl28c16", 
										"at28c16","at28c64","at28c256", 
										"at24c01","at24c02","at24c04","at24c08","at24c16",
										"at24c32","at24c64","at24c128","at24c256","at24c512"
										};

const int EEPROM_MODEL_SIZES[] = 	{
									2048,2048,
									2048,8192,32768,
									128,256,512,1024,2048,
									4096,8192,16384,32768,65536
									};

const int EEPROM_ADDRESS_LENGTH[] = {	
									10,10,
									10,12,14,
									7,8,9,10,11,
									12,13,14,15,16
									};

const int EEPROM_DATA_LENGTH[] = 	{
									8,8,
									8,8,8,
									8,8,8,8,8,
									8,8,8,8,8
									};
									
const int EEPROM_WRITE_CYCLE_USEC[] = 	{
										10000,10000,
										5000,10000,1000,
										5000,5000,5000,5000,5000,
										5000,5000,5000,5000,5000
										};

/* Local function wrapper to writeGPIO  */
void setPinLevel(struct gpiod_line **gpioLines, int pin, int level, struct CHIP_CONFIG* config){
	writeGPIO(gpioLines,pin, level, config);
}

/* Local function wrapper to readGPIO  */
int getPinLevel(struct gpiod_line **gpioLines, int pin,struct CHIP_CONFIG* config){
	return readGPIO(gpioLines,pin,config);
}

/* Local function wrapper to setPinModeGPIO  */
void setPinMode(struct gpiod_line** gpioLines, int pin, int mode, struct CHIP_CONFIG* config){
	setPinModeGPIO(gpioLines,pin, mode,config );
}

/* Local function wrapper to readByteI2C  */
int getByteI2C(int fd, int address){
	return readByteI2C(fd,address);
}

/* Local function wrapper to writeByteI2C  */
int setByteI2C(int fd, int address, char data){
	return writeByteI2C(fd,address,data);
}

/* Set Address eeprom to value to read from or write to */
void setAddressPins(struct gpiod_line** gpioLines, struct EEPROM* eeprom,unsigned int addressToSet,struct CHIP_CONFIG* config){
	for (char pin = 0;pin<eeprom->maxAddressLength;pin++){
		if (!((eeprom->model == AT28C64) && ((pin == 13) || (pin == 14)))){
			setPinLevel(gpioLines,eeprom->addressPins[(int)pin],(addressToSet & 1), config);
			addressToSet >>= 1;
		}
	}
}

/* Set Data eeprom to value to write */
void setDataPins(struct gpiod_line** gpioLines, struct EEPROM* eeprom,char dataToSet,struct CHIP_CONFIG* config){
	for (char pin = 0;pin<eeprom->maxDataLength;pin++){
		setPinLevel(gpioLines,eeprom->dataPins[(int)pin],(dataToSet & 1), config);
		dataToSet >>= 1;
	}
}

/* More descriptive wrapper around usleep to show the wait time for writes in microseconds */
void waitWriteCycle(int usec){
	usleep(usec);
}

/* Fast forward to start value when reading file */
void seekFileToStartValue(FILE *romFile, struct OPTIONS *sOptions){
	fseek(romFile, 0L, SEEK_END);
	unsigned long size = ftell(romFile);
	rewind(romFile);
	if(sOptions->startValue < sOptions->limit && size > sOptions->startValue){
		fseek(romFile,sOptions->startValue,SEEK_SET);
	}
}

/* Initialize Raspberry Pi to perform action on EEPROM */
int initHardware(struct EEPROM *eeprom, struct GPIO_CHIP* gpioChip, struct OPTIONS *sOptions){
	gpioChip->chipname = sOptions->chipname;
	gpioChip->numGPIOLines = sOptions->numGPIOLines;

	if(setupGPIO(&gpioChip->chip,gpioChip->chipname,gpioChip->gpioLines,sOptions->consumer, \
					gpioChip->numGPIOLines,&gpioChip->config)== -1){
		ulog(ERROR, "Failed to setup GPIO");
		return -1;
	}

	eeprom->i2cId = sOptions->i2cId;
    eeprom->writeCycleWait = sOptions->writeCycleUSec;

	eeprom->model = sOptions->eepromModel;
	eeprom->size = EEPROM_MODEL_SIZES[sOptions->eepromModel]-1;
	eeprom->maxAddressLength = (EEPROM_ADDRESS_LENGTH[sOptions->eepromModel]+1);
	eeprom->maxDataLength = (EEPROM_DATA_LENGTH[sOptions->eepromModel]);
	if(eeprom->writeCycleWait == -1){
		eeprom->writeCycleWait = EEPROM_WRITE_CYCLE_USEC[sOptions->eepromModel];
	}
	
	if (sOptions->eepromModel >= AT24C01 && sOptions->eepromModel <= AT24C256){
		eeprom->fd = setupI2C(eeprom->i2cId);
								// 8; // 2 // 3 // I2C Pins 
								// 9; // 3 // 5 // I2C Pins

		eeprom->addressPins[0] =  13; // 23 // 33
		eeprom->addressPins[1] =  19; // 24 // 35
		eeprom->addressPins[2] =  26; // 25 // 37

		eeprom->vccPin = 		  12; // 26 // 32
		eeprom->writeProtectPin = 16; // 27 // 36


		for(int i=0;i<3;i++){
			setPinMode(gpioChip->gpioLines,eeprom->addressPins[i], OUTPUT, &gpioChip->config);
			setPinLevel(gpioChip->gpioLines,eeprom->addressPins[i], LOW, &gpioChip->config);
		}

		if (eeprom->i2cId != 0x50){
			setAddressPins(gpioChip->gpioLines, eeprom, eeprom->i2cId - 0x50, &gpioChip->config);
		}

		setPinMode(gpioChip->gpioLines,eeprom->writeProtectPin, OUTPUT, &gpioChip->config);
		setPinMode(gpioChip->gpioLines,eeprom->vccPin, OUTPUT, &gpioChip->config);
		setPinLevel(gpioChip->gpioLines,eeprom->writeProtectPin, HIGH, &gpioChip->config);
		setPinLevel(gpioChip->gpioLines,eeprom->vccPin, HIGH, &gpioChip->config);

	} else {
							/*   GPIO // WiPi // Pin   */ 
		eeprom->addressPins[14] =  4; //  7 //  7   !Not Used on AT28C16
		eeprom->addressPins[12] = 17; //  0 // 11 	!Not Used on AT28C16
		
		eeprom->addressPins[7] =  27; //  2 // 13
		eeprom->addressPins[6] =  22; //  3 // 15
		eeprom->addressPins[5] =  10; // 12 // 19
		eeprom->addressPins[4] =   9; // 13 // 21
		eeprom->addressPins[3] =  11; // 14 // 23
		eeprom->addressPins[2] =   0; // 30 // 27
		eeprom->addressPins[1] =   5; // 21 // 29
		eeprom->addressPins[0] =   6; // 22 // 31

		eeprom->dataPins[0] =     13; // 23 // 33
		eeprom->dataPins[1] =     19; // 24 // 35
		eeprom->dataPins[2] =     26; // 25 // 37
		
		if (sOptions->eepromModel == AT28C64 || sOptions->eepromModel == AT28C256){
			eeprom->writeEnablePin =  14; // 15 // 8
			
			eeprom->addressPins[13] = 15; // 16 // 10
			eeprom->addressPins[11] = 24; // 5 // 18
		} else if (sOptions->eepromModel <= AT28C16){
			eeprom->writeEnablePin =  24; // 5 // 18
			eeprom->vccPin = 		  15; // 16 // 10
		}
	
		eeprom->addressPins[8] =    18; //  1 // 12
		eeprom->addressPins[9] =    23; //  4 // 16

		eeprom->outputEnablePin =   25; //  6 // 22
		eeprom->addressPins[10] =    8; // 10 // 23
		eeprom->chipEnablePin =	     7; // 11 // 26
		eeprom->dataPins[7] =        1; // 31 // 28
		eeprom->dataPins[6] =       12; // 26 // 32
		eeprom->dataPins[5] =       16; // 27 // 36
		eeprom->dataPins[4] =        3;//20; // 28 // 38
		eeprom->dataPins[3] =        2;//21; // 29 // 40
		
		for(int i=0;i<eeprom->maxAddressLength;i++){
			if ((eeprom->model == AT28C64) && ((i == 13) || (i == 14))){
				// handle NC pins
				setPinMode(gpioChip->gpioLines,eeprom->addressPins[i], INPUT, &gpioChip->config);	
			} else {
				// ulog(DEBUG,"Setting Mode for Pin: %i",i);
				setPinMode(gpioChip->gpioLines,eeprom->addressPins[i], OUTPUT, &gpioChip->config);	
			}
		}
	
		for(int i=0;i<eeprom->maxDataLength;i++){
			setPinMode(gpioChip->gpioLines,eeprom->dataPins[i], INPUT, &gpioChip->config);
		}

		setPinMode(gpioChip->gpioLines,eeprom->chipEnablePin, OUTPUT, &gpioChip->config);
		setPinMode(gpioChip->gpioLines,eeprom->outputEnablePin, OUTPUT, &gpioChip->config);
		setPinMode(gpioChip->gpioLines,eeprom->writeEnablePin, OUTPUT, &gpioChip->config);
		setPinMode(gpioChip->gpioLines,eeprom->vccPin, OUTPUT, &gpioChip->config);
		setPinLevel(gpioChip->gpioLines,eeprom->chipEnablePin, LOW, &gpioChip->config);
		setPinLevel(gpioChip->gpioLines,eeprom->outputEnablePin, HIGH, &gpioChip->config);
		setPinLevel(gpioChip->gpioLines,eeprom->writeEnablePin, HIGH, &gpioChip->config);
		setPinLevel(gpioChip->gpioLines,eeprom->vccPin, HIGH, &gpioChip->config);
		
	}
	usleep(5000); //startup delay
	ulog(DEBUG,"Finished GPIO Initialization");
	return 0;
}

/* Read byte from specified Address */
int readByteFromAddress(struct GPIO_CHIP* gpioChip, struct EEPROM* eeprom,unsigned int addressToRead){
	int byteVal = 0;
	if(addressToRead > eeprom->size){
		ulog(ERROR,"Address out of range of EEPROM: %i",addressToRead);
		return -1;
	}
	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C512){
		setPinLevel(gpioChip->gpioLines,eeprom->writeProtectPin,HIGH, &gpioChip->config);
		byteVal = getByteI2C(eeprom->fd,addressToRead);
	} else {
		// set the address
		setAddressPins(gpioChip->gpioLines, eeprom,addressToRead,&gpioChip->config);
		// enable output from the chip
		setPinLevel(gpioChip->gpioLines,eeprom->outputEnablePin,LOW, &gpioChip->config);
		// set the rpi to input on it's gpio data lines
		for(int i=0;i<eeprom->maxDataLength;i++){
			setPinMode(gpioChip->gpioLines,eeprom->dataPins[i],INPUT, &gpioChip->config);
		}
		// read the eeprom and store to string
		for(int i=eeprom->maxDataLength-1;i>=0;i--){
			byteVal <<= 1;
			byteVal |= (getPinLevel(gpioChip->gpioLines,eeprom->dataPins[i],&gpioChip->config) & 1);		
		}
	}
	// return the number
	return byteVal;
}

/* Write specified byte to specified address */
int writeByteToAddress(struct GPIO_CHIP* gpioChip, struct EEPROM* eeprom,unsigned int addressToWrite, \
						char dataToWrite, struct OPTIONS *sOptions, int* byteWriteCounter){
	int err = 0;
	if(addressToWrite > eeprom->size){
		ulog(ERROR,"Address out of range of EEPROM: %i",addressToWrite);
		return -1;
	}
	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C512){
		setPinLevel(gpioChip->gpioLines,eeprom->writeProtectPin,HIGH, &gpioChip->config);
		if ( sOptions->force || dataToWrite != getByteI2C(eeprom->fd,addressToWrite )){
			setPinLevel(gpioChip->gpioLines,eeprom->writeProtectPin,LOW, &gpioChip->config);
			waitWriteCycle(eeprom->writeCycleWait);
			if (-1 != setByteI2C(eeprom->fd,addressToWrite, dataToWrite )){
				waitWriteCycle(eeprom->writeCycleWait);
				ulog(INFO,"Wrote Byte %i at Address %i",dataToWrite,addressToWrite);
			} else {
				ulog(WARNING,"Failed to Write Byte %i at Address %i",dataToWrite,addressToWrite);
				if(byteWriteCounter != NULL){
					(*byteWriteCounter)--;
				}
				err = -1;
			}
			if(byteWriteCounter != NULL){
				(*byteWriteCounter)++;
			}
		}
	} else {
		if ( sOptions->force || dataToWrite != readByteFromAddress(gpioChip, eeprom,addressToWrite)){
			// set the address
			setAddressPins(gpioChip->gpioLines, eeprom,addressToWrite,&gpioChip->config);
			// disable output from the chip
			setPinLevel(gpioChip->gpioLines,eeprom->outputEnablePin,HIGH, &gpioChip->config);
			// set the rpi to output on it's gpio data lines
			for(int i=0;i<eeprom->maxDataLength;i++){
					setPinMode(gpioChip->gpioLines,eeprom->dataPins[i], OUTPUT, &gpioChip->config);
			}
			// Set the data eeprom to the data to be written
			setDataPins(gpioChip->gpioLines, eeprom,dataToWrite, &gpioChip->config);
			
			// perform the write
			setPinLevel(gpioChip->gpioLines,eeprom->writeEnablePin,LOW, &gpioChip->config);
			usleep(1);
			setPinLevel(gpioChip->gpioLines,eeprom->writeEnablePin,HIGH, &gpioChip->config);
			waitWriteCycle(eeprom->writeCycleWait);
			if (sOptions->validateWrite == 1){
				if ( dataToWrite != readByteFromAddress(gpioChip, eeprom,addressToWrite)){
					ulog(WARNING,"Failed to Write Byte %i at Address %i",dataToWrite,addressToWrite);
					if(byteWriteCounter != NULL){
						(*byteWriteCounter)--;
					}
					err = -1;
				} else {
					ulog(INFO,"Wrote Byte %i at Address %i",dataToWrite,addressToWrite);
				}
			}
			if(byteWriteCounter != NULL){
				(*byteWriteCounter)++;
			}
		}
	}
	return err;
}

/* Open and write a text file to EEPROM */
int writeTextFileToEEPROM(struct GPIO_CHIP* gpioChip,struct EEPROM *eeprom, FILE *romFile, struct OPTIONS *sOptions){
	int counter = 0;
	
	char textFileAddress[eeprom->maxAddressLength+1];
	char textFiledata[eeprom->maxDataLength+1];
	int c;
	int err = 0;
	int haveNotReachedSeparator = 1;
	int addressLength = 0;
	int dataLength = 0;
	int byteWriteCounter = 0;

	textFileAddress[eeprom->maxAddressLength] = 0;
	textFiledata[eeprom->maxDataLength] = 0;

	while( ((c = fgetc(romFile)) != EOF ) && counter < sOptions->limit){
		if (c != '\n'){
			if ((addressLength < eeprom->maxAddressLength) && (haveNotReachedSeparator) ){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				} else if (c == ' '){
					haveNotReachedSeparator = 0;
				}
			} else {
				if (dataLength < eeprom->maxDataLength){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			if (addressLength < eeprom->maxAddressLength){
				for(int i = addressLength-1,j=eeprom->maxAddressLength-1;i>=0;i--,j--){
					textFileAddress[j]= textFileAddress[i];
					textFileAddress[i] = '0';
				}
			}
			if (binStr2num(textFileAddress) >= sOptions->startValue){
				err = writeByteToAddress( gpioChip, \
							eeprom,binStr2num(textFileAddress),binStr2num(textFiledata),sOptions,&byteWriteCounter) || err;
			}
			addressLength = 0;
			dataLength = 0;
			haveNotReachedSeparator = 1;
			counter++;
		}
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return err;
}

/* Compare a text file to EEPRom */
int compareTextFileToEEPROM(struct GPIO_CHIP* gpioChip, struct EEPROM *eeprom,FILE *romFile, struct OPTIONS *sOptions){
	int err = 0;
	int dataToCompare = 0;
	int addressToCompare = 0;
	int bytesNotMatched = 0;

	int counter = 0;
	
	char textFileAddress[eeprom->maxAddressLength+1];
	char textFiledata[eeprom->maxDataLength+1];
	int c;
	int haveNotReachedSeparator = 1;
	int addressLength = 0;
	int dataLength = 0;

	textFileAddress[eeprom->maxAddressLength] = 0;
	textFiledata[eeprom->maxDataLength] = 0;

	while( ((c = fgetc(romFile)) != EOF ) && (counter < sOptions->limit)){
		if (c != '\n'){
			if ((addressLength < eeprom->maxAddressLength) && (haveNotReachedSeparator) ){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				} else if (c == ' '){
					haveNotReachedSeparator = 0;
				}
			} else {
				if (dataLength < eeprom->maxDataLength){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			if (addressLength < eeprom->maxAddressLength){
				for(int i = addressLength-1,j=eeprom->maxAddressLength-1;i>=0;i--,j--){
					textFileAddress[j]= textFileAddress[i];
					textFileAddress[i] = '0';

				}
			}
			addressToCompare  = binStr2num(textFileAddress);
			dataToCompare = binStr2num(textFiledata);
			if (addressToCompare >= sOptions->startValue){
				if (readByteFromAddress(gpioChip, eeprom,addressToCompare) != dataToCompare){
					ulog(DEBUG,"Byte at Address 0x%02x does not match. EEPROM: %i File: %i", \
						addressToCompare,readByteFromAddress(gpioChip, eeprom,addressToCompare),dataToCompare);
					bytesNotMatched++;
					err = 1;
				}
			}
			addressLength = 0;
			dataLength = 0;
			haveNotReachedSeparator = 1;
			counter++;
		}
	}
	if ( bytesNotMatched == 0) {
		ulog(INFO,"All bytes match.");
	} else {
		ulog(INFO,"%i bytes do not match",bytesNotMatched);
	}
	return err;
}

/* Open and write a binary file to the EEPROM */
int writeBinaryFileToEEPROM(struct GPIO_CHIP* gpioChip,struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	char dataToWrite;
	int addressToWrite = 0;
	int err = 0;
	int byteWriteCounter = 0;

	seekFileToStartValue(romFile,sOptions);
	
	while(((dataToWrite = fgetc(romFile)) != EOF) && addressToWrite < sOptions->limit) {
		err = writeByteToAddress(gpioChip, eeprom,addressToWrite++,dataToWrite,sOptions,&byteWriteCounter) || err;
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return err;
}

/* Compare a binary file to EEPRom */
int compareBinaryFileToEEPROM(struct GPIO_CHIP* gpioChip, struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	int err = 0;
	char dataToCompare;
	int addressToCompare = 0;
	int bytesNotMatched = 0;

	seekFileToStartValue(romFile,sOptions);

	while(((dataToCompare = fgetc(romFile)) != EOF) && addressToCompare < sOptions->limit) {
		if (readByteFromAddress(gpioChip, eeprom,addressToCompare) != dataToCompare){
			ulog(DEBUG,"Byte at Address 0x%02x does not match. EEPROM: %i File: %i", \
				addressToCompare,readByteFromAddress(gpioChip, eeprom,addressToCompare),dataToCompare);
			bytesNotMatched++;
			err = 1;
		}
		addressToCompare++;
	}
	if ( bytesNotMatched == 0) {
		ulog(INFO,"All bytes match.");
	} else {
		ulog(INFO,"%i bytes do not match",bytesNotMatched);
	}
	return err;
}

/* Compare a file to EEPRom */
int compareFileToEEPROM(struct GPIO_CHIP* gpioChip, struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	if (sOptions->fileType == TEXT_FILE){
		return compareTextFileToEEPROM(gpioChip, eeprom,romFile,sOptions);
	} else {
		return compareBinaryFileToEEPROM(gpioChip, eeprom,romFile,sOptions);
	}
}

/* Open and write a file to EEPROM */
int writeFileToEEPROM(struct GPIO_CHIP* gpioChip,struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	if (sOptions->fileType == TEXT_FILE){
		return writeTextFileToEEPROM(gpioChip, eeprom,romFile,sOptions);
	} else {
		return writeBinaryFileToEEPROM(gpioChip, eeprom,romFile,sOptions);
	}
}

/* Prints the EEPRom's Contents to the specified limit */
void printEEPROMContents(struct GPIO_CHIP* gpioChip, struct EEPROM* eeprom, struct OPTIONS *sOptions){
	if (sOptions->limit == -1 || sOptions->limit > EEPROM_MODEL_SIZES[eeprom->model]){
		sOptions->limit = EEPROM_MODEL_SIZES[eeprom->model];
	}
	
	switch (sOptions->dumpFormat) {
	case 0:
		for (int i=sOptions->startValue;i<sOptions->limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(gpioChip, eeprom,i));
		}
		break;
	case 1:	// binary
		setLoggingLevel(OFF);
		for (int i=0;i<sOptions->startValue;i++) {
			putc(0xFF,stdout);
		}
		for (int i=sOptions->startValue;i<sOptions->limit;i++) {
			putc(readByteFromAddress(gpioChip, eeprom,i),stdout);
		}
		break;
	case 2: // text
		for (int i=sOptions->startValue;i<sOptions->limit;i++) {
			char addressBinStr[eeprom->maxAddressLength+2];
			char dataBinStr[eeprom->maxDataLength+1];

			num2binStr(addressBinStr,i,eeprom->maxAddressLength+1);
			num2binStr(dataBinStr,readByteFromAddress(gpioChip, eeprom,i),eeprom->maxDataLength);
			if ( sOptions->startValue < 0x100 && sOptions->limit < 0x100){
				char shortAddressBinStr[9];
				strncpy(shortAddressBinStr,&addressBinStr[8],8);
				printf("%s %s \n", shortAddressBinStr,dataBinStr);
			} else {
				printf("%s %s \n",addressBinStr,dataBinStr);
			}
		}
		break;

	case 3: // pretty
	default:
		if ((sOptions->startValue % 16) != 0){
			sOptions->startValue = sOptions->startValue - ((sOptions->startValue % 16));
		}

		printf("       00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\n");
		printf("Device ===============================================================\n");
		for (int i=sOptions->startValue;i<sOptions->limit;i++){
			printf("%04x | ",i);
			int j=0;
			while(j<16 && i<sOptions->limit){
				printf("%02x  ",readByteFromAddress(gpioChip, eeprom,i++));
				j++;
			}
			i--;
			printf("\n");
		}

		break;
	}
}

/* Free and release hardware */
void cleanupHardware(struct GPIO_CHIP* gpioChip, struct EEPROM* eeprom){
	cleanupGPIO(gpioChip->chip,gpioChip->gpioLines,gpioChip->numGPIOLines,&gpioChip->config);
	cleanupI2C(eeprom->fd);
}

/* Prints help message */
void printHelp(){
	printf("piepro v%s\n",VERSION);
	printf("Usage: piepro [options] [file]\n");
	printf("Options:\n");
	printf(" -b,        --board         Specify the SoC board used. Default: Raspberry Pi 4/400\n");
	printf(" -c,        --compare       Compare file and EEPROM and print differences.\n");
	printf("            --chipname      Specify the chipname to use. Default: gpiochip0\n");
	printf(" -d N, 	    --dump N        Dump the contents of the EEPROM, 0=DEFAULT, 1=BINARY, 2=TEXT, 3=PRETTY.\n");
	printf(" -f,   	    --force         Force writing of every byte instead of checking for existing value first.\n");
	printf(" -i FILE,   --image FILE    The Filename to use.\n");
	printf(" -id,       --i2c-device-id The Address id of the I2C device.\n");
	printf(" -h,   	    --help          Print this message and exit.\n");
	printf(" -l N, 	    --limit N       Specify the maximum address to operate.\n");
	printf("       	    --no-validate-write Do not perform a read directly after writing to verify the data was written.\n");
	printf(" -m MODEL,  --model MODEL   Specify EERPOM device model. Default: AT28C16.\n");
	printf(" -r N, 	    --read ADDRESS  Read the contents of the EEPROM, 0=DEFAULT, 1=BINARY, 2=TEXT, 3=PRETTY.\n");
	printf(" -rb N,     --read-byte ADDRESS  Read From specified ADDRESS.\n");
	printf(" -s N, 	    --start N       Specify the minimum address to operate.\n");
	printf(" -t,        --text          Interpret file as a binary. Default: binary\n");
	printf("                            Text File format:\n");
	printf("                            [00000000]00000000 00000000\n");
	printf(" -v N, 	    --v[vvvv]       Set the log verbosity to N, 0=OFF, 1=FATAL, 2=ERROR, 3=WARNING, 4=INFO, 5=DEBUG.\n");
	printf("            --version       Print the piepro version and exit.\n");
	printf(" -w,   	    --write         Write EEPROM with specified file.\n");
	printf(" -wb ADDRESS DATA, --write-byte ADDRESS DATA	Write specified DATA to ADDRESS.\n");
	printf(" -wd N,     --write-delay N Number of microseconds to delay between writes.\n");
	printf("\n");
}

/* Prints version number */
void printVersion(){
	printf("piepro v%s\n",VERSION);
}

/* Sets default options */
void setDefaultOptions(struct OPTIONS* sOptions){
    sOptions->limit = -1;
    sOptions->startValue = 0;
    sOptions->dumpFormat = 3;
    sOptions->validateWrite = 1;
    sOptions->force = 0;
    sOptions->action = NOTHING;
    sOptions->fileType = BINARY_FILE;
    sOptions->eepromModel = AT28C16;
    sOptions->writeCycleUSec = -1;
    sOptions->i2cId = 0x50;
	sOptions->boardType = RPI4;
	sOptions->filename = NULL;

    sOptions->addressParam = 0;
    sOptions->dataParam = 0;

	
	sOptions->consumer = consumer;
    sOptions->chipname = chipname;
    sOptions->numGPIOLines = 28;
}

/* Parses and processes all command line arguments */
int  parseCommandLineOptions(struct OPTIONS* sOptions,int argc, char* argv[]){
    setDefaultOptions(sOptions);

    sOptions->filename = argv[argc-1];
		for(int i=argc-1;i>0;i--){
			// -h --help
			if (!(strcmp(argv[i],"-h")) || (!strcmp(argv[i],"--help"))){
				printHelp();
				sOptions->action = NOTHING;
				return 0;
			}

			// --version
			if (!strcmp(argv[i],"--version")){
				printVersion();
				sOptions->action = NOTHING;
				return 0;
			}

			//-v -vvvv
			if (!strcmp(argv[i],"-v") || !strcmp(argv[i],"--v") || !strcmp(argv[i],"--vv") \
				|| !strcmp(argv[i],"--vvv") || !strcmp(argv[i],"--vvvv")|| !strcmp(argv[i],"--vvvvv")){
				int verbosity = 0;
				if (!strcmp(argv[i],"-v")){
					verbosity = str2num(argv[i+1]);
				} else {
					verbosity = strlen(argv[i])-2;
				}
				if (setLoggingLevel(verbosity)){
					return 1;
				}
			}
		}

		for(int i=argc-1;i>0;i--){
			// -s --start
			if (!strcmp(argv[i],"-s") || !strcmp(argv[i],"--start")){
				ulog(INFO,"Setting starting value to %i",str2num(argv[i+1]));
				sOptions->startValue = str2num(argv[i+1]);
				if ( sOptions->startValue == -1){
					ulog(FATAL,"Unsupported starting value");
					return 1;
				}
			}

			// -i --image
			if (!strcmp(argv[i],"-i") || !strcmp(argv[i],"--image")){
				ulog(INFO,"Setting filename to %s",argv[i+1]);
				sOptions->filename = argv[i+1];
			}

			// -l --limit
			if (!strcmp(argv[i],"-l") || !strcmp(argv[i],"--limit")){
				ulog(INFO,"Setting limit to %i",str2num(argv[i+1]));
				sOptions->limit = str2num(argv[i+1]);
				if ( sOptions->limit== -1){
					ulog(FATAL,"Unsupported limit value");
					return 1;
				}
			}

			// -b --board 
			if (!strcmp(argv[i],"-b") || !strcmp(argv[i],"--board")){
				ulog(INFO,"Setting SoC Board type to Raspberry Pi 4/400");
				ulog(INFO,"Right now only 1 board is supported");
				// sOptions->fileType = BINARY_FILE;
			}

			// -t --text 
			if (!strcmp(argv[i],"-t") || !strcmp(argv[i],"--text")){
				ulog(INFO,"Setting filetype to text");
				sOptions->fileType = TEXT_FILE;
			}

			// -wd --write-delay
			if (!strcmp(argv[i],"-wd") || !strcmp(argv[i],"--write-delay")){
				ulog(INFO,"Setting write cycle delay time to %i",str2num(argv[i+1]));
				sOptions->writeCycleUSec = str2num(argv[i+1]);
				if ( sOptions->writeCycleUSec == -1){
					ulog(FATAL,"Unsupported delay value");
					return 1;
				}
			}

			// -id --i2c-device-id
			if (!strcmp(argv[i],"-id") || !strcmp(argv[i],"--i2c-device-id")){
				ulog(INFO,"Setting I2C id to %i",str2num(argv[i+1]));
				sOptions->i2cId = str2num(argv[i+1]);
				if ( sOptions->i2cId == -1){
					ulog(FATAL,"Unsupported I2C id value");
					return 1;
				}
			}

			// --chipname
			if (!strcmp(argv[i],"--chipname")){
				ulog(INFO,"Setting Chipname to %i",str2num(argv[i+1]));
				sOptions->chipname = argv[i+1];
			}

			// -d --dump || -r --read
			if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dump") || !strcmp(argv[i],"-r") || !strcmp(argv[i],"--read")){
				if (sOptions->action != DUMP_ROM && sOptions->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Dumping EEPROM to standard out");
					sOptions->dumpFormat = str2num(argv[i+1]);
					sOptions->action = DUMP_ROM;
				}
			}

			// -wb --write-byte
			if (!strcmp(argv[i],"-wb") || !strcmp(argv[i],"--write-byte")){
				if (sOptions->action != WRITE_SINGLE_BYTE_TO_ROM && sOptions->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					
					sOptions->addressParam = str2num(argv[i+1]);
					sOptions->dataParam = str2num(argv[i+2]);
					if ( sOptions->addressParam == -1 || sOptions->dataParam == -1) {
						ulog(ERROR,"Unsupported number format. Exiting.");
						return 1;
					}
					if (sOptions->dataParam > 0xFF){
						ulog(ERROR,"Data byte too large. Please specify a value less than 256.");
						return 1;
					}
					ulog(INFO,"Writing Byte %s to Address %s",argv[i+2],argv[i+1]);
					sOptions->action = WRITE_SINGLE_BYTE_TO_ROM;
				}
			}

			// -rb --read-byte
			if (!strcmp(argv[i],"-rb") || !strcmp(argv[i],"--read-byte")){
				if (sOptions->action != READ_SINGLE_BYTE_FROM_ROM && sOptions->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					
					sOptions->addressParam = str2num(argv[i+1]);
					if ( sOptions->addressParam == -1) {
						ulog(ERROR,"Unsupported number format. Exiting.");
						return 1;
					}
					ulog(INFO,"Reading single byte from Address %s",argv[i+1]);
					sOptions->action = READ_SINGLE_BYTE_FROM_ROM;
				}
			}

			// -c --compare
			if (!strcmp(argv[i],"-c") || !strcmp(argv[i],"--compare")){
				if (sOptions->action != COMPARE_FILE_TO_ROM && sOptions->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Comparing EEPROM to File");
					sOptions->action = COMPARE_FILE_TO_ROM;
				}
			}

			// -w --write
			if (!strcmp(argv[i],"-w") || !strcmp(argv[i],"--write")){
				if (sOptions->action != WRITE_FILE_TO_ROM && sOptions->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Writing File to EEPROM");
					sOptions->action = WRITE_FILE_TO_ROM;
				}
			}

			// --no-validate-write
			if (!strcmp(argv[i],"--no-validate-write")){
				ulog(WARNING,"Disabling write verification");
				sOptions->validateWrite = 0;
			}

			// -f --force
			if (!strcmp(argv[i],"-f") || !strcmp(argv[i],"--force")){
					ulog(INFO,"Forcing all writes even if value is already present.");
					sOptions->force = 1;
			}

			// -m --model
			if (!strcmp(argv[i],"-m") || !strcmp(argv[i],"--model")){
				sOptions->eepromModel = 0;
				while(sOptions->eepromModel < END && strcmp(argv[i+1],EEPROM_MODEL_STRINGS[sOptions->eepromModel])){
					sOptions->eepromModel++;
				}
				if(sOptions->eepromModel == END){
					ulog(INFO,"Supported EEPROM Models:");
					for(int i=0; i < END-1;i++){
						ulog(INFO,"\t%s",EEPROM_MODEL_STRINGS[i]);
					}
					ulog(FATAL,"Unsupported EEPROM Model");
					return 1;
				}
				ulog(INFO,"Setting EEPROM model to %s",EEPROM_MODEL_STRINGS[sOptions->eepromModel]);
			}
		}
	
	if(sOptions->action == NOTHING){
		ulog(WARNING,"No action specified. Doing Nothing. Run piepro -h for a list of options");
	}
	return 0;
}