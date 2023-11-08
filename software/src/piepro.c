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
	for (char pin = 0;pin<=eeprom->maxAddressLength;pin++){
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
		// Dummy write to wait for Ack from write
		if (eeprom->type == I2C){
			while(writeI2C(eeprom->fd,NULL,0) == -1){
				;;
			}
		} else {
			// TODO: Implement Parallel EEPROM polling
			usleep(eeprom->writeCycleTime);
		}
		return 0;
	} else {
		// Wait Write Cycle time as per datasheet
		usleep(eeprom->writeCycleTime);
	}
	return 0;
}

/* Writes bytes to an EEPROM via Parallel GPIO */
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
		setDataPins(gpioConfig, eeprom, data[j]);
		
		// perform the write
		setPinLevel(gpioConfig,eeprom->writeEnablePin,LOW);
		usleep(1);
		setPinLevel(gpioConfig,eeprom->writeEnablePin,HIGH);
		finishWriteCycle(eeprom);
	}
	return 0;
}

/* Write a single byte to an EEPROM via Parallel GPIO */
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
void setEEPROMParameters(struct OPTIONS* options, struct EEPROM* eeprom){
	eeprom->model = options->eepromModel;
	eeprom->i2cId = options->i2cId;
	eeprom->forceWrite = options->force;
	eeprom->validateWrite = options->validateWrite;
	eeprom->startValue = options->startValue;
	eeprom->fileType = options->fileType;

	eeprom->byteWriteCounter = 0;
	eeprom->byteReadCounter = 0;

	eeprom->useWriteCyclePolling = options->useWriteCyclePolling;
	

	eeprom->size = EEPROM_MODEL_SIZE[options->eepromModel];
	eeprom->maxAddressLength = EEPROM_ADDRESS_LENGTH[options->eepromModel];
	eeprom->maxDataLength = (EEPROM_DATA_LENGTH[options->eepromModel]);
	eeprom->pageSize = EEPROM_PAGE_SIZE[options->eepromModel];
	eeprom->addressSize = EEPROM_ADDRESS_SIZE[options->eepromModel];
	
	if( options->writeCycleUSec == -1){
		eeprom->writeCycleTime = EEPROM_WRITE_CYCLE_USEC[options->eepromModel];
	}else{
    	eeprom->writeCycleTime = options->writeCycleUSec;
	}

	if( options->limit == -1){
		eeprom->limit = EEPROM_MODEL_SIZE[options->eepromModel];
	}else{
    	eeprom->limit = options->limit;
	}

	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C256){
		eeprom->type = I2C;
	} else {
		eeprom->type = PARALLEL;
	}

}

/* Sets all parameters to use GPIO */
void setGPIOConfigParameters(struct OPTIONS* options, struct GPIO_CONFIG* gpioConfig){
	gpioConfig->gpioChip.chipname = options->chipname;
	gpioConfig->gpioChip.numGPIOLines = options->numGPIOLines;
	gpioConfig->gpioChip.consumer = options->consumer;

	gpioConfig->chipname = options->chipname;
	gpioConfig->numGPIOLines = options->numGPIOLines;
	gpioConfig->consumer = options->consumer;

	gpioConfig->gpioChip.isSetup = 0;
}

/******************************************************************************
*******************************************************************************
******************************************************************************/

/* Initialize Raspberry Pi to perform action on EEPROM */
int initHardware(struct OPTIONS *options, struct EEPROM *eeprom, struct GPIO_CONFIG* gpioConfig){
	setEEPROMParameters(options, eeprom);
	
	setGPIOConfigParameters(options, gpioConfig);
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
	if(addressToRead > eeprom->size-1){
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
	if((unsigned)addressToWrite > eeprom->size-1){
		ulog(ERROR,"Address out of range of EEPROM: %i",addressToWrite);
		return -1;
	}
	if (eeprom->type == I2C){
		if (eeprom->forceWrite || dataToWrite != getByteI2C(eeprom,addressToWrite)){
			setPinLevel(gpioConfig,eeprom->writeProtectPin,LOW);
			if (setByteI2C(eeprom, addressToWrite, dataToWrite) != -1){
				ulog(DEBUG,"Wrote Byte %i at Address %i",dataToWrite,addressToWrite);
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
					ulog(DEBUG,"Wrote Byte %i at Address %i",dataToWrite,addressToWrite);
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
			fprintf(stdout,"Address: %i     Data: %i \n",i,readByteFromAddress(gpioConfig, eeprom,i));
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
				fprintf(stdout,"%s %s \n", shortAddressBinStr,dataBinStr);
			} else {
				fprintf(stdout,"%s %s \n",addressBinStr,dataBinStr);
			}
		}
		break;

	case 3: // pretty
	default:
		if ((eeprom->startValue % 16) != 0){
			eeprom->startValue = eeprom->startValue - ((eeprom->startValue % 16));
		}

		fprintf(stdout,"       00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\n");
		fprintf(stdout,"Device ===============================================================\n");
		for (int i=eeprom->startValue;i<eeprom->limit;i++){
			fprintf(stdout,"%04x | ",i);
			int j=0;
			while(j<16 && i<eeprom->limit){
				fprintf(stdout,"%02x  ",readByteFromAddress(gpioConfig, eeprom,i++));
				j++;
			}
			i--;
			fprintf(stdout,"\n");
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
	fprintf(stdout,"piepro v%s\n",VERSION);
	fprintf(stdout,"Usage: piepro [options]\n");
	fprintf(stdout,"Options:\n");
	fprintf(stdout," -c FILE,   --compare FILE  Compare FILE and EEPROM and print number of differences.\n");
	fprintf(stdout,"            --chipname      Specify the chipname to use. Default: gpiochip0\n");
	fprintf(stdout," -d N,      --dump N        Dump the contents of the EEPROM, 0=LABELED, 1=BINARY, 2=TEXT, 3=PRETTY. Default: PRETTY\n");
	fprintf(stdout," -f,        --force         Force writing of every byte instead of checking for existing value first.\n");
	fprintf(stdout," -id,       --i2c-device-id The address id of the I2C device.\n");
	fprintf(stdout," -h,        --help          Print this message and exit.\n");
	fprintf(stdout," -l N,      --limit N       Specify the maximum address to operate.\n");
	fprintf(stdout," -m MODEL,  --model MODEL   Specify EERPOM device model. Default: AT28C16.\n");
	fprintf(stdout,"            --no-validate-write \n");
	fprintf(stdout,"                            Do not perform a read directly after writing to verify the data was written.\n");
	fprintf(stdout," -r,        --read          Read the contents of the EEPROM, 0=LABELED, 1=BINARY, 2=TEXT, 3=PRETTY. Default: PRETTY\n");
	fprintf(stdout," -rb N,     --read-byte ADDRESS \n");
	fprintf(stdout,"                            Read From specified ADDRESS.\n");
	fprintf(stdout," -s N,      --start N       Specify the minimum address to operate.\n");
	fprintf(stdout," -t,        --text          Interpret file as a text. Default: binary\n");
	fprintf(stdout,"                            Text File format:\n");
	fprintf(stdout,"                            [00000000]00000000 00000000\n");
	fprintf(stdout," -v N,      --v[vvvv]       Set the log verbosity to N, 0=OFF, 1=FATAL, 2=ERROR, 3=WARNING, 4=INFO, 5=DEBUG. Default: WARNING\n");
	fprintf(stdout,"            --version       Print the piepro version and exit.\n");
	fprintf(stdout," -w FILE,   --write FILE    Write EEPROM with specified file.\n");
	fprintf(stdout," -wb ADDRESS DATA, --write-byte ADDRESS DATA \n");
	fprintf(stdout,"                            Write specified DATA to ADDRESS.\n");
	fprintf(stdout," -wd [N],   --write-delay N Enable write delay. N Number of microseconds to delay between writes.\n");
	fprintf(stdout,"\n");
}

/* Prints version number */
void printVersion(){
	fprintf(stdout,"piepro v%s\n",VERSION);
}

/* Sets default options */
void setDefaultOptions(struct OPTIONS* options){
	// Options
    options->filename = NULL;
	options->limit = -1;
    options->startValue = 0;
    options->dumpFormat = 3;
    options->validateWrite = 1;
    options->force = 0;
    options->action = NOTHING;
    options->fileType = BINARY_FILE;
    options->eepromModel = AT28C16;
    options->writeCycleUSec = -1;
	options->useWriteCyclePolling = 1;
	options->boardType = RPI4;
	// Single Read/Write Parameters
    options->addressParam = 0;
    options->dataParam = 0;
	// I2C Specific
	options->i2cId = 0x50;
	options->consumer = consumer;
    options->chipname = chipname;
    options->numGPIOLines = 28;
}

/* Parses and processes all command line arguments */
int  parseCommandLineOptions(struct OPTIONS* options,int argc, char* argv[]){
    setDefaultOptions(options);

    options->filename = argv[argc-1];
		for(int i=argc-1;i>0;i--){
			// -h --help
			if (!(strcmp(argv[i],"-h")) || (!strcmp(argv[i],"--help"))){
				printHelp();
				options->action = HELP;
				return 0;
			}

			// --version
			if (!strcmp(argv[i],"--version")){
				printVersion();
				options->action = VER;
				return 0;
			}

			//-v -vvvv
			if (!strcmp(argv[i],"-v") || !strcmp(argv[i],"--v") || !strcmp(argv[i],"--vv") \
				|| !strcmp(argv[i],"--vvv") || !strcmp(argv[i],"--vvvv")|| !strcmp(argv[i],"--vvvvv")){
				int verbosity = 0;
				if (!strcmp(argv[i],"-v")){
					if (i != argc-1) {
						verbosity = str2num(argv[i+1]);
					} else {
						ulog(ERROR,"%s Flag must have a verbosity level specified",argv[i]);
					return -1;
					}
				} else {
					verbosity = strlen(argv[i])-2;
				}
				if (setLoggingLevel(verbosity)){
					return -1;
				}
			}
		}

		for(int i=1;i<argc;i++){
			// -c --compare
			if (!strcmp(argv[i],"-c") || !strcmp(argv[i],"--compare")){
				if (options->action != COMPARE_FILE_TO_ROM && options->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else if (i != argc-1) {
					ulog(INFO,"Comparing EEPROM to File: %s",argv[i+1]);
					options->action = COMPARE_FILE_TO_ROM;
					options->filename = argv[i+1];
					i++;
				} else {
					ulog(ERROR,"%s Flag must have a filename specified",argv[i]);
					return -1;
				}
			}

			// --chipname
			if (!strcmp(argv[i],"--chipname")){
				if (i != argc-1) {
					ulog(INFO,"Setting Chipname to %s",argv[i+1]);
					options->chipname = argv[i+1];
					i++;
				} else {
					ulog(ERROR,"%s Flag must have a chipname specified",argv[i]);
					return -1;
				}
			}

			// -d --dump || -r --read
			if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dump") || !strcmp(argv[i],"-r") || !strcmp(argv[i],"--read")){
				if (options->action != DUMP_ROM && options->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Dumping EEPROM to standard out");
					int format = 3;
					if (i != argc-1) {
						format = str2num(argv[i+1]);
						i++;
					}
					if(format == -1 || format > 3){
						options->dumpFormat = 3;
					} else {
						options->dumpFormat = format;
					}
					options->action = DUMP_ROM;
				}
			}

			// -f --force
			if (!strcmp(argv[i],"-f") || !strcmp(argv[i],"--force")){
					ulog(INFO,"Forcing all writes even if value is already present.");
					options->force = 1;
			}

			// -id --i2c-device-id
			if (!strcmp(argv[i],"-id") || !strcmp(argv[i],"--i2c-device-id")){
				if (i != argc-1) {
					options->i2cId = str2num(argv[i+1]);		
					if ( options->i2cId == (char)-1){
						ulog(ERROR,"Unsupported I2C id value");
						return -1;
					} else {
						ulog(INFO,"Setting I2C id to %i",options->i2cId);
					}
					i++;
				} else {
					ulog(ERROR,"%s Flag must have an id specified",argv[i]);
					return -1;
				}
				
			}

			// -l --limit
			if (!strcmp(argv[i],"-l") || !strcmp(argv[i],"--limit")){
				if (i != argc-1) {
					options->limit = str2num(argv[i+1]);
					ulog(INFO,"Setting limit to %i",options->limit);
					if ( options->limit== -1){
						ulog(ERROR,"Unsupported limit value");
						return -1;
					}
				} else {
					ulog(ERROR,"%s Flag must have a value specified",argv[i]);
					return -1;
				}
			}
			
			// -m --model
			if (!strcmp(argv[i],"-m") || !strcmp(argv[i],"--model")){
				if (i != argc-1) {
					options->eepromModel = 0;
					while(options->eepromModel < END && strcmp(argv[i+1],EEPROM_MODEL_STRINGS[options->eepromModel])){
						options->eepromModel++;
					}
					if(options->eepromModel == END){
						ulog(INFO,"Supported EEPROM Models:");
						for(int i=0; i < END-1;i++){
							ulog(INFO,"\t%s",EEPROM_MODEL_STRINGS[i]);
						}
						ulog(FATAL,"Unsupported EEPROM Model");
						return -1;
					}
					ulog(INFO,"Setting EEPROM model to %s",EEPROM_MODEL_STRINGS[options->eepromModel]);
				} else {
					ulog(ERROR,"%s Flag must have a model specified",argv[i]);
					return -1;
				}
			}

			// --no-validate-write
			if (!strcmp(argv[i],"--no-validate-write")){
				ulog(WARNING,"Disabling write verification");
				options->validateWrite = 0;
			}

			// -rb --read-byte
			if (!strcmp(argv[i],"-rb") || !strcmp(argv[i],"--read-byte")){
				if (options->action != READ_SINGLE_BYTE_FROM_ROM && options->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else if (i != argc-1) {
					options->addressParam = str2num(argv[i+1]);
					if ( options->addressParam == -1) {
						ulog(ERROR,"Invalid number. Exiting.");
						return -1;
					}
					ulog(INFO,"Reading single byte from Address %s",argv[i+1]);
					options->action = READ_SINGLE_BYTE_FROM_ROM;
				}else {
					ulog(ERROR,"%s Flag must have a value specified",argv[i]);
					return -1;
				}
			}

			// -s --start
			if (!strcmp(argv[i],"-s") || !strcmp(argv[i],"--start")){
				if (i != argc-1) {
					options->startValue = str2num(argv[i+1]);
					ulog(INFO,"Setting starting value to %i",options->startValue);
					if ( options->startValue == -1){
						ulog(FATAL,"Unsupported starting value");
						return -1;
					}
				} else {
					ulog(ERROR,"%s Flag must have a value specified",argv[i]);
					return -1;
				}
			}

			// -t --text 
			if (!strcmp(argv[i],"-t") || !strcmp(argv[i],"--text")){
				ulog(INFO,"Setting filetype to text");
				options->fileType = TEXT_FILE;
			}

			// -w --write
			if (!strcmp(argv[i],"-w") || !strcmp(argv[i],"--write")){
				if (options->action != WRITE_FILE_TO_ROM && options->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else if (i != argc-1) {
					ulog(INFO,"Writing File to EEPROM");
					options->action = WRITE_FILE_TO_ROM;
					options->filename = argv[i+1];
					ulog(INFO,"Setting filename to %s",options->filename);
				} else {
					ulog(ERROR,"%s Flag must have a filename specified",argv[i]);
					return -1;
				}
			}

			// -wd --write-delay
			if (!strcmp(argv[i],"-wd") || !strcmp(argv[i],"--write-delay")){
				ulog(INFO,"Using Write Cycle Delay instead of Polling");
				options->useWriteCyclePolling = 0;
				if (i != argc-1) {
					options->writeCycleUSec = str2num(argv[i+1]);
					if ( options->writeCycleUSec != -1){
						ulog(INFO,"Setting write cycle delay time to %i",options->writeCycleUSec);
					}
				}
			}

			// -wb --write-byte
			if (!strcmp(argv[i],"-wb") || !strcmp(argv[i],"--write-byte")){
				if (options->action != WRITE_SINGLE_BYTE_TO_ROM && options->action != NOTHING){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else if (i < argc-2) {
					options->addressParam = str2num(argv[i+1]);
					options->dataParam = str2num(argv[i+2]);
					if ( options->addressParam == -1 || options->dataParam == -1) {
						ulog(ERROR,"Invalid number. Exiting.");
						return -1;
					}
					if (options->dataParam > 0xFF){
						ulog(ERROR,"Data byte too large. Please specify a value less than 256.");
						return -1;
					}
					ulog(INFO,"Writing Byte %s to Address %s",argv[i+2],argv[i+1]);
					options->action = WRITE_SINGLE_BYTE_TO_ROM;
				} else  {
					ulog(ERROR,"%s Flag must have an address and data specified",argv[i]);
					return -1;
				}
			}

		}
	return 0;
}
