#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "piepro.h"
#include "utils.h"
#include "gpio.h"
#include "ulog.h"

static char* const chipname = "gpiochip0";
static char* const consumer = "Pi EEPROM Programmer";

const char* EEPROM_MODEL_STRINGS[] = 	{
										"xl2816","xl28c16", 
										"at28c16","at28c64","at28c256", 
										"at24c01","at24c02","at24c04","at24c08","at24c16",
										"at24c32","at24c64","at24c128","at24c256","at24c512"
										};

const int EEPROM_MODEL_SIZE[] = 	{
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

const int EEPROM_PAGE_SIZE[] = 	{
									-1,-1,
									-1,-1,-1,
									8,8,16,16,16,
									32,32,64,64,128
									};

const int EEPROM_ADDRESS_SIZE[] = 	{
										-1,-1,
										-1,-1,-1,
										1,1,2,2,2,
										2,2,2,2,2
										};

/******************************************************************************
*******************************************************************************
******************************************************************************/

/* Local function wrapper to writeGPIO  */
void setPinLevel(struct GPIO_CONFIG* gpioConfig, int pin, int level){
	writeGPIO(&gpioConfig->gpioChip, pin, level);
}

/* Local function wrapper to readGPIO  */
int getPinLevel(struct GPIO_CONFIG* gpioConfig, int pin){
	return readGPIO(&gpioConfig->gpioChip, pin);
}

/* Local function wrapper to setPinModeGPIO  */
void setPinMode(struct GPIO_CONFIG* gpioConfig, int pin, int mode){
	setPinModeGPIO(&gpioConfig->gpioChip, pin, mode);
}

/* Set Address eeprom to value to read from or write to */
void setAddressPins(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, unsigned int addressToSet){
	for (char pin = 0;pin<eeprom->maxAddressLength;pin++){
		if (!((eeprom->model == AT28C64) && ((pin == 13) || (pin == 14)))){
			setPinLevel(gpioConfig,eeprom->addressPins[(int)pin],(addressToSet & 1));
			addressToSet >>= 1;
		}
	}
}

/* Set Data eeprom to value to write */
void setDataPins(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom,char dataToSet){
	for (char pin = 0;pin<eeprom->maxDataLength;pin++){
		setPinLevel(gpioConfig,eeprom->dataPins[(int)pin],(dataToSet & 1));
		dataToSet >>= 1;
	}
}

/* Poll device or wait until write cycle finishes */
int finishWriteCycle(struct EEPROM* eeprom){
	// Finish Write Cycle
	if(eeprom->useWriteCyclePolling){
		// Wait for Ack from write
		if (eeprom->type == I2C){
			while(writeI2C(eeprom->fd,NULL,0) == -1){
				;;
			}
		} else {
			// TODO: Implement Parallel EEPROM polling
		}
		return 0;
	} else {
		// Wait Write Cycle time as per datasheet
		usleep(eeprom->writeCycleTime);
	}
	return 0;
}

/* Writes bytes to an EEPROM via I2C */
int setBytesParallel(struct GPIO_CONFIG* gpioConfig,struct EEPROM* eeprom,int addressToWrite,char* data,int numBytesToWrite){
	for(int j=0;j < numBytesToWrite; j++){
		// set the address
		setAddressPins(gpioConfig, eeprom, addressToWrite);
		// disable output from the chip
		setPinLevel(gpioConfig,eeprom->outputEnablePin,HIGH);
		// set the rpi to output on it's gpio data lines
		for(int i=0;i<eeprom->maxDataLength;i++){
			setPinMode(gpioConfig,eeprom->dataPins[i], OUTPUT);
		}
		// Set the data eeprom to the data to be written
		setDataPins(gpioConfig, eeprom, data[addressToWrite+j]);
		
		// perform the write
		setPinLevel(gpioConfig,eeprom->writeEnablePin,LOW);
		usleep(1);
		setPinLevel(gpioConfig,eeprom->writeEnablePin,HIGH);
		finishWriteCycle(eeprom);
	}
	return 0;
}

/* Write a single byte to an EEPROM via I2C */
int setByteParallel(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, int addressToWrite, char data){
	return setBytesParallel(gpioConfig, eeprom, addressToWrite, &data, 1);
}

/* Reads bytes from an EEPROM via Parallel GPIO */
int* getBytesParallel(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, int addressToRead, int numBytesToRead,int* buf){
	for(int j=0; j<numBytesToRead;j++){
		int byteVal = 0;
		// set the address
		setAddressPins(gpioConfig, eeprom, addressToRead);
		// enable output from the chip
		setPinLevel(gpioConfig, eeprom->outputEnablePin, LOW);
		// set the rpi to input on it's gpio data lines
		for(int i=0;i<eeprom->maxDataLength;i++){
			setPinMode(gpioConfig, eeprom->dataPins[i], INPUT);
		}
		// read the eeprom and store to string
		for(int i=eeprom->maxDataLength-1;i>=0;i--){
			byteVal <<= 1;
			byteVal |= (getPinLevel(gpioConfig,eeprom->dataPins[i]) & 1);		
		}
		buf[j] = byteVal;
		addressToRead++;
	}
	return buf;
}

/* Read a single byte from and EEPROM via Parallel GPIO */
int getByteParallel(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, int address){
	int buf[eeprom->addressSize + 1];
	getBytesParallel(gpioConfig, eeprom, address,1, buf);
	return *buf;
}

/* Populates the array for the I2C format */
int buildI2CDataArray(struct EEPROM* eeprom, int address, int* buf){
	int i = 0;

	// Set the Address
    while(i < eeprom->addressSize){
        buf[i++] = address & 0xFF;
        address >>= (i*8);	// i is 1 at first so we shift 8 bits each pass for each byte of the address
    }
    if(address != 0){
        ulog(ERROR,"Address out of Range for EEPROM: %i", address);
        return -1;
    }

	return i;
}

/* Reads bytes from an EEPROM via I2C */
int* getBytesI2C(struct EEPROM* eeprom, int addressToRead, int numBytesToRead, int* buf){
    int bufSize = buildI2CDataArray(eeprom,addressToRead, buf);
	return readI2C(eeprom->fd,buf,bufSize);
}

/* Read a single byte from and EEPROM via I2C */
int getByteI2C(struct EEPROM* eeprom, int address){
	int buf[eeprom->addressSize + 1];
	getBytesI2C(eeprom,address,1, buf);
	return *buf;
}

/* Writes bytes to an EEPROM via I2C */
int setBytesI2C(struct EEPROM* eeprom, int addressToWrite, char* data, int numBytesToWrite){
	char buf[eeprom->addressSize + numBytesToWrite];
	
	int bufSize = buildI2CDataArray(eeprom,addressToWrite, (int*)buf);
	// Set the Data
    while((bufSize - eeprom->addressSize) < numBytesToWrite){
        buf[bufSize] = data[bufSize - eeprom->addressSize];
        ++bufSize;
    }
	int numBytesWritten = writeI2C(eeprom->fd,buf,bufSize);
	finishWriteCycle(eeprom);
	return numBytesWritten;
}

/* Write a single byte to an EEPROM via I2C */
int setByteI2C(struct EEPROM* eeprom, int addressToWrite, char data){
	return setBytesI2C(eeprom, addressToWrite, &data, 1);
}

/* Fast forward to start value when reading file */
void seekFileToStartValue(FILE *romFile, int startValue){
	fseek(romFile, 0L, SEEK_END);
	unsigned long size = ftell(romFile);
	rewind(romFile);
	if(size > startValue){
		fseek(romFile,startValue,SEEK_SET);
	}
}

/* Get the next set of Data from a text file formatted rom */
int getNextFromTextFile(struct EEPROM *eeprom, FILE *romFile){
	int c;
	int addressLength = 0;
	char textFileAddress[(sizeof(int)*8)+1];
	
	while((c = fgetc(romFile)) != EOF && addressLength != sizeof(int)*8 && c != '\n' && c != ' ' && c != ':'){
		if(c == '1' || c == '0'){
			textFileAddress[addressLength++] = c;
		}
	}
	
	textFileAddress[addressLength++] = 0;
	if(c == EOF){
		return -1;
	} else {
		return binStr2num(textFileAddress);
	}
}

/* Sets all parameters for the EEPROM to be used */
void setEEPROMParameters(struct OPTIONS* sOptions, struct EEPROM* eeprom){
	eeprom->model = sOptions->eepromModel;
	eeprom->i2cId = sOptions->i2cId;
	eeprom->forceWrite = sOptions->force;
	eeprom->validateWrite = sOptions->validateWrite;
	eeprom->startValue = sOptions->startValue;
	eeprom->fileType = sOptions->fileType;

	eeprom->byteWriteCounter = 0;
	eeprom->byteReadCounter = 0;

	eeprom->useWriteCyclePolling = sOptions->useWriteCyclePolling;

	eeprom->size = EEPROM_MODEL_SIZE[sOptions->eepromModel];
	eeprom->maxAddressLength = EEPROM_ADDRESS_LENGTH[sOptions->eepromModel];
	eeprom->maxDataLength = (EEPROM_DATA_LENGTH[sOptions->eepromModel]);
	eeprom->pageSize = EEPROM_PAGE_SIZE[sOptions->eepromModel];
	eeprom->addressSize = EEPROM_ADDRESS_SIZE[sOptions->eepromModel];
	
	if( sOptions->writeCycleUSec == -1){
		eeprom->writeCycleTime = EEPROM_ADDRESS_SIZE[sOptions->eepromModel];
	}else{
    	eeprom->writeCycleTime = sOptions->writeCycleUSec;
	}

	if( sOptions->limit == -1){
		eeprom->limit = EEPROM_MODEL_SIZE[sOptions->eepromModel];
	}else{
    	eeprom->limit = sOptions->limit;
	}

	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C256){
		eeprom->type = I2C;
	} else {
		eeprom->type = PARALLEL;
	}

}

/* Sets all parameters to use GPIO */
void setGPIOConfigParameters(struct OPTIONS* sOptions, struct GPIO_CONFIG* gpioConfig){
	gpioConfig->gpioChip.chipname = sOptions->chipname;
	gpioConfig->gpioChip.numGPIOLines = sOptions->numGPIOLines;
	gpioConfig->gpioChip.consumer = sOptions->consumer;

	gpioConfig->chipname = sOptions->chipname;
	gpioConfig->numGPIOLines = sOptions->numGPIOLines;
	gpioConfig->consumer = sOptions->consumer;

	gpioConfig->gpioChip.isSetup = 0;
}

/******************************************************************************
*******************************************************************************
******************************************************************************/

/* Initialize Raspberry Pi to perform action on EEPROM */
int initHardware(struct OPTIONS *sOptions, struct EEPROM *eeprom, struct GPIO_CONFIG* gpioConfig){
	setEEPROMParameters(sOptions, eeprom);
	
	setGPIOConfigParameters(sOptions, gpioConfig);
	if(setupGPIO(&gpioConfig->gpioChip)){
		ulog(ERROR, "Failed to setup GPIO");
		return -1;
	}

	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C256){
		eeprom->fd = setupI2C(eeprom->i2cId);
								// 2; // 8 // 3 // I2C Pins 
								// 3; // 9 // 5 // I2C Pins

		eeprom->addressPins[0] =  13; // 23 // 33
		eeprom->addressPins[1] =  19; // 24 // 35
		eeprom->addressPins[2] =  26; // 25 // 37

		eeprom->vccPin = 		  12; // 26 // 32
		eeprom->writeProtectPin = 16; // 27 // 36


		for(int i=0;i<3;i++){
			setPinMode(gpioConfig,eeprom->addressPins[i], OUTPUT);
			setPinLevel(gpioConfig,eeprom->addressPins[i], LOW);
		}

		if (eeprom->i2cId != 0x50){
			setAddressPins(gpioConfig, eeprom, eeprom->i2cId - 0x50);
		}

		setPinMode(gpioConfig,eeprom->writeProtectPin, OUTPUT);
		setPinMode(gpioConfig,eeprom->vccPin, OUTPUT);
		setPinLevel(gpioConfig,eeprom->writeProtectPin, HIGH);
		setPinLevel(gpioConfig,eeprom->vccPin, HIGH);

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
		
		if (eeprom->model == AT28C64 || eeprom->model == AT28C256){
			eeprom->writeEnablePin =  14; // 15 // 8
			
			eeprom->addressPins[13] = 15; // 16 // 10
			eeprom->addressPins[11] = 24; // 5 // 18
		} else if (eeprom->model <= AT28C16){
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
				setPinMode(gpioConfig,eeprom->addressPins[i], INPUT);	
			} else {
				// ulog(DEBUG,"Setting Mode for Pin: %i",i);
				setPinMode(gpioConfig,eeprom->addressPins[i], OUTPUT);	
			}
		}
	
		for(int i=0;i<eeprom->maxDataLength;i++){
			setPinMode(gpioConfig,eeprom->dataPins[i], INPUT);
		}

		setPinMode(gpioConfig,eeprom->chipEnablePin, OUTPUT);
		setPinMode(gpioConfig,eeprom->outputEnablePin, OUTPUT);
		setPinMode(gpioConfig,eeprom->writeEnablePin, OUTPUT);
		setPinMode(gpioConfig,eeprom->vccPin, OUTPUT);
		setPinLevel(gpioConfig,eeprom->chipEnablePin, LOW);
		setPinLevel(gpioConfig,eeprom->outputEnablePin, HIGH);
		setPinLevel(gpioConfig,eeprom->writeEnablePin, HIGH);
		setPinLevel(gpioConfig,eeprom->vccPin, HIGH);
		
	}
	usleep(5000); //startup delay
	ulog(DEBUG,"Finished GPIO Initialization");
	return 0;
}

/* Read byte from specified Address */
int readByteFromAddress(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, unsigned int addressToRead){
	int byteVal = 0;
	if(addressToRead > eeprom->size){
		ulog(ERROR,"Address out of range of EEPROM: %i",addressToRead);
		return -1;
	}
	if (eeprom->type == I2C){
		setPinLevel(gpioConfig,eeprom->writeProtectPin,HIGH);
		byteVal = getByteI2C(eeprom,addressToRead);
	} else {
		byteVal = getByteParallel(gpioConfig,eeprom,addressToRead);
	}
	// return the number
	return byteVal;
}

/* Write specified byte to specified address */
int writeByteToAddress(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, int addressToWrite, char dataToWrite){
	int err = 0;
	if((unsigned)addressToWrite > eeprom->size){
		ulog(ERROR,"Address out of range of EEPROM: %i",addressToWrite);
		return -1;
	}
	if (eeprom->type == I2C){
		if (eeprom->forceWrite || dataToWrite != getByteI2C(eeprom,addressToWrite)){
			setPinLevel(gpioConfig,eeprom->writeProtectPin,LOW);
			if (setByteI2C(eeprom, addressToWrite, dataToWrite) != -1){
				ulog(INFO,"Wrote Byte %i at Address %i",dataToWrite,addressToWrite);
				eeprom->byteWriteCounter++;
			} else {
				ulog(WARNING,"Failed to Write Byte %i at Address %i",dataToWrite,addressToWrite);
				err = -1;
			}
			setPinLevel(gpioConfig,eeprom->writeProtectPin,HIGH);
		}
	} else {
		if (eeprom->forceWrite || dataToWrite != readByteFromAddress(gpioConfig, eeprom,addressToWrite)){
			setByteParallel(gpioConfig,eeprom,addressToWrite,dataToWrite);
			if (eeprom->validateWrite == 1){
				if (dataToWrite != readByteFromAddress(gpioConfig, eeprom,addressToWrite)){
					ulog(WARNING,"Failed to Write Byte %i at Address %i",dataToWrite,addressToWrite);
					err = -1;
				} else {
					ulog(INFO,"Wrote Byte %i at Address %i",dataToWrite,addressToWrite);
					eeprom->byteWriteCounter++;
				}
			}
		}
	}
	return err;
}

/* Open and write a text file to EEPROM */
int writeTextFileToEEPROM(struct GPIO_CONFIG* gpioConfig, struct EEPROM *eeprom, FILE *romFile){
	int err = 0;
	int address = 0;
	int data = 0;
	while(address < eeprom->limit && address >= eeprom->startValue && address != -1 && data != -1){
		address = getNextFromTextFile(eeprom, romFile);
		data = getNextFromTextFile(eeprom, romFile);
		if(address != -1 && data != -1){
			err |= writeByteToAddress(gpioConfig, eeprom, address, data);
		}
	}
	ulog(INFO,"Wrote %i bytes",eeprom->byteWriteCounter);
	return err;
}

/* Compare a text file to EEPROM */
int compareTextFileToEEPROM(struct GPIO_CONFIG* gpioConfig, struct EEPROM *eeprom, FILE *romFile){
	int address = 0;
	int data = 0;
	int bytesNotMatched = 0;
	while(address < eeprom->limit && address >= eeprom->startValue && address != -1 && data != -1){
		address = getNextFromTextFile(eeprom, romFile);
		data = getNextFromTextFile(eeprom, romFile);
		if(address != -1 && data != -1){
			int byte = readByteFromAddress(gpioConfig, eeprom, address);
			if (byte != data){
				ulog(INFO,"Byte at Address 0x%02x does not match. EEPROM: %i File: %i",address,byte,data);
				bytesNotMatched++;
			}
		}
	}

	if(bytesNotMatched == 0) {
		ulog(INFO,"All bytes match.");
	} else {
		ulog(INFO,"%i bytes do not match",bytesNotMatched);
	}
	return bytesNotMatched;
}

/* Open and write a binary file to the EEPROM */
int writeBinaryFileToEEPROM(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, FILE *romFile){
	char dataToWrite;
	int addressToWrite = eeprom->startValue;
	int err = 0;

	seekFileToStartValue(romFile,eeprom->startValue);
	
	while((addressToWrite < eeprom->limit && (dataToWrite = fgetc(romFile)) != EOF)) {
		err |= writeByteToAddress(gpioConfig, eeprom, addressToWrite++, dataToWrite);
	}
	ulog(INFO,"Wrote %i bytes",eeprom->byteWriteCounter);
	return err;
}

/* Compare a binary file to EEPROM */
int compareBinaryFileToEEPROM(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, FILE *romFile){
	char dataToCompare;
	int addressToCompare = eeprom->startValue;
	int bytesNotMatched = 0;

	seekFileToStartValue(romFile,eeprom->startValue);

	while(((dataToCompare = fgetc(romFile)) != EOF) && addressToCompare < eeprom->limit) {
		char byte = readByteFromAddress(gpioConfig, eeprom,addressToCompare);
		if (byte != dataToCompare){
			ulog(INFO,"Byte at Address 0x%02x does not match. EEPROM: %i File: %i",addressToCompare,byte,dataToCompare);
			bytesNotMatched++;
		}
		addressToCompare++;
	}
	if(bytesNotMatched == 0) {
		ulog(INFO,"All bytes match.");
	} else {
		ulog(INFO,"%i bytes do not match", bytesNotMatched);
	}
	return bytesNotMatched;
}

/* Compare a file to EEPROM */
int compareFileToEEPROM(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, FILE *romFile){
	if (eeprom->fileType == TEXT_FILE){
		return compareTextFileToEEPROM(gpioConfig, eeprom, romFile);
	} else {
		return compareBinaryFileToEEPROM(gpioConfig, eeprom, romFile);
	}
}

/* Open and write a file to EEPROM */
int writeFileToEEPROM(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, FILE *romFile){
	if (eeprom->fileType == TEXT_FILE){
		return writeTextFileToEEPROM(gpioConfig, eeprom, romFile);
	} else {
		return writeBinaryFileToEEPROM(gpioConfig, eeprom, romFile);
	}
}

/* Prints the EEPROM's Contents to the specified limit */
void printEEPROMContents(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom, int format){
	if (eeprom->limit == -1 || eeprom->limit > EEPROM_MODEL_SIZE[eeprom->model]){
		eeprom->limit = EEPROM_MODEL_SIZE[eeprom->model];
	}
	
	switch (format) {
	case 0:
		for (int i=eeprom->startValue;i<eeprom->limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(gpioConfig, eeprom,i));
		}
		break;
	case 1:	// binary
		setLoggingLevel(OFF);
		for (int i=0;i<eeprom->startValue;i++) {
			putc(0xFF,stdout);
		}
		for (int i=eeprom->startValue;i<eeprom->limit;i++) {
			putc(readByteFromAddress(gpioConfig, eeprom,i),stdout);
		}
		break;
	case 2: // text
		for (int i=eeprom->startValue;i<eeprom->limit;i++) {
			char addressBinStr[eeprom->maxAddressLength+2];
			char dataBinStr[eeprom->maxDataLength+1];

			num2binStr(addressBinStr,i,eeprom->maxAddressLength+1);
			num2binStr(dataBinStr,readByteFromAddress(gpioConfig, eeprom,i),eeprom->maxDataLength);
			if ( eeprom->startValue < 0x100 && eeprom->limit < 0x100){
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
		if ((eeprom->startValue % 16) != 0){
			eeprom->startValue = eeprom->startValue - ((eeprom->startValue % 16));
		}

		printf("       00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\n");
		printf("Device ===============================================================\n");
		for (int i=eeprom->startValue;i<eeprom->limit;i++){
			printf("%04x | ",i);
			int j=0;
			while(j<16 && i<eeprom->limit){
				printf("%02x  ",readByteFromAddress(gpioConfig, eeprom,i++));
				j++;
			}
			i--;
			printf("\n");
		}

		break;
	}
}

/* Free and release hardware */
void cleanupHardware(struct GPIO_CONFIG* gpioConfig, struct EEPROM* eeprom){
	cleanupGPIO(&gpioConfig->gpioChip);
	cleanupI2C(eeprom->fd);
}

/******************************************************************************
*******************************************************************************
******************************************************************************/

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
	sOptions->useWriteCyclePolling = 1;
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
					int tmpLog = getLoggingLevel();
					setLoggingLevel(OFF);
					int format = str2num(argv[i+1]);
					setLoggingLevel(tmpLog);
					if(format == -1 || format > 3){
						ulog(INFO,"No dump format specified, invalid dump format, or number out of range. Using default.");
						sOptions->dumpFormat = 3;
					} else {
						sOptions->dumpFormat = str2num(argv[i+1]);
					}
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
