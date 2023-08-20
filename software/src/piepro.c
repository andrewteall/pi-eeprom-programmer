#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "piepro.h"
#include "gpio.h"

	static char *chipname = "gpiochip0";
    static struct gpiod_chip *chip = NULL;
    static struct gpiod_line* gpioLines[40];

	const char* EEPROM_MODEL_STRINGS[] = {"xl2816","xl28c16", \
                                        "at28c16","at28c64","at28c256", \
                                        "at24c01","at24c02","at24c256","at24c512"};
    const int EEPROM_MODEL_SIZES[] = {2048,2048, \
                                        2048,8192,32768, \
                                        1024,2048,32768,65536};
    const int EEPROM_NUM_ADDRESS_PINS[] = {11,11, \
                                            11,13,15, \
                                            10,11,1,1};
    const int EEPROM_NUM_DATA_PINS[] = {8,8, \
                                        8,8,8, \
                                        8,8,8,8};
    const int EEPROM_WRITE_CYCLE_USEC[] = {10000,10000, \
                                            5000,10000,1000, \
                                            5000,5000,5000,5000};
	
int compareBinaryFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);
int compareTextFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);
int writeTextFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);
int writeBinaryFileToEEPROM(struct EEPROM*, FILE*, struct OPTIONS*);

/* Local function wrapper to writeGPIO  */
void setPinLevel(struct gpiod_line **gpioLines, int pin, int level){
	writeGPIO(gpioLines,pin, level);
}

/* Local function wrapper to readGPIO  */
int getPinLevel(struct gpiod_line **gpioLines, int pin){
	return readGPIO(gpioLines,pin);
}

/* Local function wrapper to setPinModeGPIO  */
void setPinMode(struct gpiod_line **gpioLines, int pin, int mode){
	setPinModeGPIO(chip,gpioLines,pin, mode);
}

/* Local function wrapper to readByteI2C  */
int getByteI2C(int fd, int address){
	return readByteI2C(fd,address);
}

int setByteI2C(int fd, int address, char data){
	return writeByteI2C(fd,address,data);
}

/* Set Address eeprom to value to read from or write to */
void setAddressPins(struct EEPROM* eeprom,unsigned int addressToSet){
	for (char pin = 0;pin<eeprom->numAddressPins;pin++){
		if (!((eeprom->model == AT28C64) && ((pin == 13) || (pin == 14)))){
			setPinLevel(gpioLines,eeprom->addressPins[(int)pin],(addressToSet & 1));
			addressToSet >>= 1;
		}
	}
}

/* Set Data eeprom to value to write */
void setDataPins(struct EEPROM* eeprom,char dataToSet){
	for (char pin = 0;pin<eeprom->numDataPins;pin++){
		setPinLevel(gpioLines,eeprom->dataPins[(int)pin],(dataToSet & 1));
		dataToSet >>= 1;
	}
}

/* More descriptive wrapper around usleep to show the wait time for writes in microseconds */
void waitWriteCycle(int usec){
	usleep(usec);
}

/* Initialize Raspberry Pi to perform action on EEPROM */
int init(struct EEPROM *eeprom,int eepromModel,struct OPTIONS *sOptions){
	setupGPIO(chip,chipname,gpioLines,"Pi EEPROM Programmer");
	
	eeprom->i2cId = sOptions->i2cId;
    eeprom->writeCycleWait = sOptions->writeCycleUSec;

	eeprom->model = eepromModel;
	eeprom->size = EEPROM_MODEL_SIZES[eepromModel];
	eeprom->numAddressPins = (EEPROM_NUM_ADDRESS_PINS[eepromModel]);
	eeprom->numDataPins = (EEPROM_NUM_DATA_PINS[eepromModel]);
	if(eeprom->writeCycleWait == -1){
		eeprom->writeCycleWait = EEPROM_WRITE_CYCLE_USEC[eepromModel];
	}
	
	if (eepromModel >= AT24C01 && eepromModel <= AT24C256){
		eeprom->fd = setupI2C();
								// 8; // 2 // 3 // I2C Pins 
								// 9; // 3 // 5 // I2C Pins

		eeprom->addressPins[0] =  13; // 23 // 33
		eeprom->addressPins[1] =  19; // 24 // 35
		eeprom->addressPins[2] =  26; // 25 // 37

		eeprom->vccPin = 		  12; // 26 // 32
		eeprom->writeProtectPin = 16; // 27 // 36


		for(int i=0;i<3;i++){
			setPinMode(gpioLines,eeprom->addressPins[i], OUTPUT);
			setPinLevel(gpioLines,eeprom->addressPins[i], LOW);
		}

		if (eeprom->i2cId != 0x50){
			setAddressPins(eeprom,eeprom->i2cId - 0x50);
		}

		setPinMode(gpioLines,eeprom->writeProtectPin, OUTPUT);
		setPinMode(gpioLines,eeprom->vccPin, OUTPUT);
		setPinLevel(gpioLines,eeprom->writeProtectPin, HIGH);
		setPinLevel(gpioLines,eeprom->vccPin, HIGH);

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
		
		if (eepromModel == AT28C64 || eepromModel == AT28C256){
			eeprom->writeEnablePin =  14; // 15 // 8
			
			eeprom->addressPins[13] = 15; // 16 // 10
			eeprom->addressPins[11] = 24; // 5 // 18
		} else if (eepromModel <= AT28C16){
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
		
		for(int i=0;i<eeprom->numAddressPins;i++){
			if ((eeprom->model == AT28C64) && ((i == 13) || (i == 14))){
				// handle NC pins
				setPinMode(gpioLines,eeprom->addressPins[i], INPUT);	
			} else {
				// ulog(DEBUG,"Setting Mode for Pin: %i",i);
				setPinMode(gpioLines,eeprom->addressPins[i], OUTPUT);	
			}
		}
	
		for(int i=0;i<eeprom->numDataPins;i++){
			setPinMode(gpioLines,eeprom->dataPins[i], INPUT);
		}

		setPinMode(gpioLines,eeprom->chipEnablePin, OUTPUT);
		setPinMode(gpioLines,eeprom->outputEnablePin, OUTPUT);
		setPinMode(gpioLines,eeprom->writeEnablePin, OUTPUT);
		setPinMode(gpioLines,eeprom->vccPin, OUTPUT);
		setPinLevel(gpioLines,eeprom->chipEnablePin, LOW);
		setPinLevel(gpioLines,eeprom->outputEnablePin, HIGH);
		setPinLevel(gpioLines,eeprom->writeEnablePin, HIGH);
		setPinLevel(gpioLines,eeprom->vccPin, HIGH);
		
	}
	usleep(5000); //startup delay
	ulog(DEBUG,"Finished GPIO Initialization");
	return 0;
}

/* Compare a file to EEPRom */
int compareFileToEEPROM(struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	if (sOptions->fileType == TEXT_FILE){
		return compareTextFileToEEPROM(eeprom,romFile,sOptions);
	} else {
		return compareBinaryFileToEEPROM(eeprom,romFile,sOptions);
	}
}

/* Open and write a file to EEPROM */
int writeFileToEEPROM(struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	if (sOptions->fileType == TEXT_FILE){
		return writeTextFileToEEPROM(eeprom,romFile,sOptions);
	} else {
		return writeBinaryFileToEEPROM(eeprom,romFile,sOptions);
	}
}

/* Open and write a text file to EEPROM */
int writeTextFileToEEPROM(struct EEPROM *eeprom, FILE *romFile, struct OPTIONS *sOptions){
	int counter = 0;
	
	char textFileAddress[eeprom->numAddressPins+1];
	char textFiledata[eeprom->numDataPins+1];
	int c;
	int err = 0;
	int haveNotReachedSeparator = 1;
	int addressLength = 0;
	int dataLength = 0;
	int byteWriteCounter = 0;

	textFileAddress[eeprom->numAddressPins] = 0;
	textFiledata[eeprom->numDataPins] = 0;

	while( ((c = fgetc(romFile)) != EOF ) && counter < sOptions->limit){
		if (c != '\n'){
			if ((addressLength < eeprom->numAddressPins) && (haveNotReachedSeparator) ){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				} else if (c == ' '){
					haveNotReachedSeparator = 0;
				}
			} else {
				if (dataLength < eeprom->numDataPins){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			if (addressLength < eeprom->numAddressPins){
				for(int i = addressLength-1,j=eeprom->numAddressPins-1;i>=0;i--,j--){
					textFileAddress[j]= textFileAddress[i];
					textFileAddress[i] = '0';
				}
			}
			// ulog(DEBUG,"Writing to File %s  %s",textFileAddress,textFiledata);
			// ulog(DEBUG,"Writing to File %i  %i",binStr2num(textFileAddress),binStr2num(textFiledata));
			if (binStr2num(textFileAddress) >= sOptions->startValue){
				err = writeByteToAddress( \
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
int compareTextFileToEEPROM(struct EEPROM *eeprom,FILE *romFile, struct OPTIONS *sOptions){
	int addressToCompare = 0;
	int dataToCompare = 0;
	int err = 0;
	int bytesNotMatched = 0;

	int counter = 0;
	
	char textFileAddress[eeprom->numAddressPins+1];
	char textFiledata[eeprom->numDataPins+1];
	int c;
	int haveNotReachedSeparator = 1;
	int addressLength = 0;
	int dataLength = 0;

	textFileAddress[eeprom->numAddressPins] = 0;
	textFiledata[eeprom->numDataPins] = 0;

	while( ((c = fgetc(romFile)) != EOF ) && (counter < sOptions->limit)){
		if (c != '\n'){
			if ((addressLength < eeprom->numAddressPins) && (haveNotReachedSeparator) ){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				} else if (c == ' '){
					haveNotReachedSeparator = 0;
				}
			} else {
				if (dataLength < eeprom->numDataPins){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			if (addressLength < eeprom->numAddressPins){
				for(int i = addressLength-1,j=eeprom->numAddressPins-1;i>=0;i--,j--){
					textFileAddress[j]= textFileAddress[i];
					textFileAddress[i] = '0';

				}
			}
			addressToCompare  = binStr2num(textFileAddress);
			dataToCompare = binStr2num(textFiledata);
			if (addressToCompare >= sOptions->startValue){
				if (readByteFromAddress(eeprom,addressToCompare) != dataToCompare){
					ulog(DEBUG,"Byte at Address 0x%02x does not match. EEPROM: %i File: %i", \
						addressToCompare,readByteFromAddress(eeprom,addressToCompare),dataToCompare);
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

void seekFileToStartValue(FILE *romFile, struct OPTIONS *sOptions){
	fseek(romFile, 0L, SEEK_END);
	unsigned long sz = ftell(romFile);
	rewind(romFile);
	if(sOptions->startValue < sOptions->limit && sz > sOptions->startValue){
		fseek(romFile,sOptions->startValue,SEEK_SET);
	}
}

/* Open and write a binary file to the EEPROM */
int writeBinaryFileToEEPROM(struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	char dataToWrite;
	int addressToWrite = 0;
	int err = 0;
	int byteWriteCounter = 0;

	seekFileToStartValue(romFile,sOptions);
	
	while(((dataToWrite = fgetc(romFile)) != EOF) && addressToWrite < sOptions->limit) {
		err = writeByteToAddress(eeprom,addressToWrite++,dataToWrite,sOptions,&byteWriteCounter) || err;
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return err;
}

/* Compare a binary file to EEPRom */
int compareBinaryFileToEEPROM(struct EEPROM* eeprom,FILE *romFile, struct OPTIONS *sOptions){
	char dataToCompare;
	int err = 0;
	int addressToCompare = 0;
	int bytesNotMatched = 0;

	seekFileToStartValue(romFile,sOptions);

	while(((dataToCompare = fgetc(romFile)) != EOF) && addressToCompare < sOptions->limit) {
		if (readByteFromAddress(eeprom,addressToCompare) != (char)dataToCompare){
			ulog(DEBUG,"Byte at Address 0x%02x does not match. EEPROM: %i File: %i", \
				addressToCompare,readByteFromAddress(eeprom,addressToCompare),dataToCompare);
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

/* Read byte from specified Address */
char readByteFromAddress(struct EEPROM* eeprom,unsigned int addressToRead){
	char byteVal = 0;
	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C512){
		setPinLevel(gpioLines,eeprom->writeProtectPin,HIGH);
		byteVal = getByteI2C(eeprom->fd,addressToRead);
	} else {
		// set the address
		setAddressPins(eeprom,addressToRead);
		// enable output from the chip
		setPinLevel(gpioLines,eeprom->outputEnablePin,LOW);
		// set the rpi to input on it's gpio data lines
		for(int i=0;i<eeprom->numDataPins;i++){
			setPinMode(gpioLines,eeprom->dataPins[i],INPUT);
		}
		// read the eeprom and store to string
		for(int i=eeprom->numDataPins-1;i>=0;i--){
			byteVal <<= 1;
			byteVal |= (getPinLevel(gpioLines,eeprom->dataPins[i]) & 1);		
		}
	}
	// return the number
	return byteVal;
}

/* Write specified byte to specified address */
int writeByteToAddress(struct EEPROM* eeprom,unsigned int addressToWrite, \
						char dataToWrite, struct OPTIONS *sOptions, int* byteWriteCounter){
	int err = 0;
	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C512){
		setPinLevel(gpioLines,eeprom->writeProtectPin,HIGH);
		if ( sOptions->force || dataToWrite != getByteI2C(eeprom->fd,addressToWrite )){
			setPinLevel(gpioLines,eeprom->writeProtectPin,LOW);
			waitWriteCycle(eeprom->writeCycleWait);
			if (-1 != setByteI2C(eeprom->fd,addressToWrite, dataToWrite )){
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
		if ( sOptions->force || dataToWrite != readByteFromAddress(eeprom,addressToWrite)){
			// set the address
			setAddressPins(eeprom,addressToWrite);
			// disable output from the chip
			setPinLevel(gpioLines,eeprom->outputEnablePin,HIGH);
			// set the rpi to output on it's gpio data lines
			for(int i=0;i<eeprom->numDataPins;i++){
					setPinMode(gpioLines,eeprom->dataPins[i], OUTPUT);
			}
			// Set the data eeprom to the data to be written
			setDataPins(eeprom,dataToWrite);
			
			// perform the write
			setPinLevel(gpioLines,eeprom->writeEnablePin,LOW);
			usleep(1);
			setPinLevel(gpioLines,eeprom->writeEnablePin,HIGH);
			waitWriteCycle(eeprom->writeCycleWait);
			if (sOptions->validateWrite == 1){
				if ( dataToWrite != readByteFromAddress(eeprom,addressToWrite)){
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

/* Prints the EEPRom's Contents to the specified limit */
void printEEPROMContents(struct EEPROM* eeprom, struct OPTIONS *sOptions){
	if (sOptions->limit == -1 || sOptions->limit > EEPROM_MODEL_SIZES[eeprom->model]){
		sOptions->limit = EEPROM_MODEL_SIZES[eeprom->model];
	}
	
	switch (sOptions->dumpFormat) {
	case 0:
		for (int i=sOptions->startValue;i<sOptions->limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(eeprom,i));
		}
		break;
	case 1:	// binary
		setLoggingLevel(OFF);
		for (int i=0;i<sOptions->startValue;i++) {
			putc(0xFF,stdout);
		}
		for (int i=sOptions->startValue;i<sOptions->limit;i++) {
			putc(readByteFromAddress(eeprom,i),stdout);
		}
		break;
	case 2: // text
		for (int i=sOptions->startValue;i<sOptions->limit;i++) {
			char addressBinStr[eeprom->numAddressPins+2];
			char dataBinStr[eeprom->numDataPins+1];

			num2binStr(addressBinStr,i,eeprom->numAddressPins+1);
			num2binStr(dataBinStr,readByteFromAddress(eeprom,i),eeprom->numDataPins);
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
				printf("%02x  ",readByteFromAddress(eeprom,i++));
				j++;
			}
			i--;
			printf("\n");
		}

		break;
	}
}
