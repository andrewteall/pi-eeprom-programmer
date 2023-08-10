#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "piepro.h"


/* Open and write a text file to Memory */
int writeTextFileToEEPROM(struct EEPROM *eeprom, FILE *memoryFile, struct OPTIONS *sOptions){
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

	while( ((c = fgetc(memoryFile)) != EOF ) && counter < sOptions->limit){
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
							eeprom,binStr2num(textFileAddress),binStr2num(textFiledata),sOptions,&byteWriteCounter);
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
int compareTextFileToEEPROM(struct EEPROM *eeprom,FILE *memoryFile, struct OPTIONS *sOptions){
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

	while( ((c = fgetc(memoryFile)) != EOF ) && (counter < sOptions->limit)){
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

/* Open and write a binary file to the EEPROM */
int writeBinaryFileToEEPROM(struct EEPROM* eeprom,FILE *memoryFile, struct OPTIONS *sOptions){
	int c;
	int addressToWrite = 0;
	int err=0;
	int byteWriteCounter = 0;

	while (addressToWrite < sOptions->startValue && (fgetc(memoryFile) != EOF)){
		addressToWrite++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToWrite < sOptions->limit) {
		err = writeByteToAddress(eeprom,addressToWrite++,c,sOptions,&byteWriteCounter);
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return err;
}

/* Compare a binary file to EEPRom */
int compareBinaryFileToEEPROM(struct EEPROM* eeprom,FILE *memoryFile, struct OPTIONS *sOptions){
	char c;
	int err = 0;
	int addressToCompare = 0;
	int bytesNotMatched = 0;

	while (addressToCompare < sOptions->startValue && (fgetc(memoryFile) != EOF)){
		addressToCompare++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToCompare < sOptions->limit) {
		if (readByteFromAddress(eeprom,addressToCompare) != (char)c){
			ulog(DEBUG,"Byte at Address 0x%02x does not match. EEPROM: %i File: %i", \
				addressToCompare,readByteFromAddress(eeprom,addressToCompare),c);
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
		byteVal = wiringPiI2CReadReg8(eeprom->fd, addressToRead);
	} else {
		// set the address
		setAddressPins(eeprom,addressToRead);
		// enable output from the chip
		digitalWrite(eeprom->outputEnablePin,LOW);
		// set the rpi to input on it's gpio data lines
		for(int i=0;i<eeprom->numDataPins;i++){
			pinMode(eeprom->dataPins[i],INPUT);
			pullUpDnControl(eeprom->dataPins[i],PUD_DOWN);
		}
		// read the eeprom and store to string
		for(int i=eeprom->numDataPins-1;i>=0;i--){
			byteVal <<= 1;
			byteVal |= (digitalRead(eeprom->dataPins[i]) & 1);		
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
		if ( sOptions->force || dataToWrite != wiringPiI2CReadReg8(eeprom->fd, addressToWrite )){
			waitWriteCycle(eeprom->writeCycleWait);
			if (-1 != wiringPiI2CWriteReg8(eeprom->fd, addressToWrite, dataToWrite )){
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
			digitalWrite(eeprom->outputEnablePin,HIGH);
			// set the rpi to output on it's gpio data lines
			for(int i=0;i<eeprom->numDataPins;i++){
					pinMode(eeprom->dataPins[i], OUTPUT);
			}
			// Set the data eeprom to the data to be written
			setDataPins(eeprom,dataToWrite);

			// perform the write
			digitalWrite(eeprom->writeEnablePin,LOW);
			usleep(1);
			digitalWrite(eeprom->writeEnablePin,HIGH);
			// usleep(10000); //Non CMOS version <-
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

/* Set Address eeprom to value to read from or write to */
void setAddressPins(struct EEPROM* eeprom,unsigned int addressToSet){
	for (char pin = 0;pin<eeprom->numAddressPins;pin++){
		if (!((eeprom->model == AT28C64) && ((pin == 13) || (pin == 14)))){
			digitalWrite(eeprom->addressPins[pin],(addressToSet & 1));
			addressToSet >>= 1;
		}
	}
}

/* Set Data eeprom to value to write */
void setDataPins(struct EEPROM* eeprom,char dataToSet){
	for (char pin = 0;pin<eeprom->numDataPins;pin++){
		digitalWrite(eeprom->dataPins[pin],(dataToSet & 1));
		dataToSet >>= 1;
	}
}

/* Initialize Raspberry Pi to perform action on EEPROM */
int init(struct EEPROM *eeprom,int eepromModel){
	eeprom->model = eepromModel;
	eeprom->size = EEPROM_MODEL_SIZES[eepromModel];
	eeprom->numAddressPins = (EEPROM_NUM_ADDRESS_PINS[eepromModel]);
	eeprom->numDataPins = (EEPROM_NUM_DATA_PINS[eepromModel]);
	if(eeprom->writeCycleWait == -1){
		eeprom->writeCycleWait = EEPROM_WRITE_CYCLE_USEC[eepromModel];
	}
	
	if (eepromModel >= AT24C01 && eepromModel <= AT24C256){
		// 8; // 2 // 3 // I2C Pins 
		// 9; // 3 // 5 // I2C Pins

		eeprom->addressPins[0] = 23; // 13 // 33
		eeprom->addressPins[1] = 24; // 19 // 35
		eeprom->addressPins[2] = 25; // 26 // 37

		eeprom->vccPin = 26; // 12 // 32
		eeprom->writeProtectPin = 27; // 16 // 36


		for(int i=0;i<3;i++){
			pinMode(eeprom->addressPins[i], OUTPUT);
			digitalWrite(eeprom->addressPins[i], LOW);
		}

		if (eeprom->i2cId != 0x50){
			setAddressPins(eeprom,eeprom->i2cId - 0x50);
		}

		pinMode(eeprom->writeProtectPin, OUTPUT);
		pinMode(eeprom->vccPin, OUTPUT);
		digitalWrite(eeprom->writeProtectPin, LOW);
		digitalWrite(eeprom->vccPin, HIGH);

	} else {
				/*   WiPi // GPIO // Pin   */ 
		eeprom->addressPins[14] = 7; // 4 // 7   !Not Used on AT28C16
		eeprom->addressPins[12] = 0; // 17 // 11 !Not Used on AT28C16
		
		eeprom->addressPins[7] = 2; // 27 // 13
		eeprom->addressPins[6] = 3; // 22 // 15
		eeprom->addressPins[5] = 12; // 10 // 19
		eeprom->addressPins[4] = 13; // 9 // 21
		eeprom->addressPins[3] = 14; // 11 // 23
		eeprom->addressPins[2] = 30; // 0 // 27
		eeprom->addressPins[1] = 21; // 5 // 29
		eeprom->addressPins[0] = 22; // 6 // 31

		eeprom->dataPins[0] = 23; // 13 // 33
		eeprom->dataPins[1] = 24; // 19 // 35
		eeprom->dataPins[2] = 25; // 26 // 37
		
		if (eepromModel == AT28C64 || eepromModel == AT28C256){
			eeprom->writeEnablePin =  15; // 14 // 8
			
			eeprom->addressPins[13] = 16; // 15 // 10
			eeprom->addressPins[11] = 5; // 24 // 18
		} else if (eepromModel <= AT28C16){
			eeprom->writeEnablePin =  5; // 24 // 18
			eeprom->vccPin = 16; // 15 // 10

		}

		eeprom->addressPins[8] = 1; // 18 // 12
		eeprom->addressPins[9] = 4; // 23 // 16

		eeprom->outputEnablePin =	   6; // 25 // 22
		eeprom->addressPins[10] = 10; // 8 // 24
		eeprom->chipEnablePin =	  11; // 7 // 26
		eeprom->dataPins[7] = 31; // 1 // 28
		eeprom->dataPins[6] = 26; // 12 // 32
		eeprom->dataPins[5] = 27; // 16 // 36
		eeprom->dataPins[4] = 28; // 20 // 38
		eeprom->dataPins[3] = 29; // 21 // 40
		
		for(int i=0;i<eeprom->numAddressPins;i++){
			if ((eeprom->model == AT28C64) && ((i == 13) || (i == 14))){
				// handle NC pins
				pinMode(eeprom->addressPins[i], INPUT);	
				pullUpDnControl(eeprom->dataPins[i],PUD_OFF);
			} else {
				pinMode(eeprom->addressPins[i], OUTPUT);
				digitalWrite(eeprom->addressPins[i], LOW);
			}
		}

		for(int i=0;i<eeprom->numDataPins;i++){
			pinMode(eeprom->dataPins[i], OUTPUT);
			digitalWrite(eeprom->dataPins[i], LOW);
		}

		pinMode(eeprom->chipEnablePin, OUTPUT);
		pinMode(eeprom->outputEnablePin, OUTPUT);
		pinMode(eeprom->writeEnablePin, OUTPUT);
		pinMode(eeprom->vccPin, OUTPUT);
		digitalWrite(eeprom->chipEnablePin, LOW);
		digitalWrite(eeprom->outputEnablePin, HIGH);
		digitalWrite(eeprom->writeEnablePin, HIGH);
		digitalWrite(eeprom->vccPin, HIGH);
	}
	usleep(5000); //startup delay
	return 0;
}

/* More descriptive wrapper around usleep to show the wait time for writes in microseconds */
void waitWriteCycle(int usec){
	usleep(usec);
}

/* Prints the EEPRom's Contents to the specified limit */
void printROMContents(struct EEPROM* eeprom, struct OPTIONS *sOptions){
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
		if ((sOptions->startValue % 16) != 0){
			sOptions->startValue = sOptions->startValue - ((sOptions->startValue % 16));
		}
		printf("       00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\n");
		printf("       ===============================================================\n");
		for (int i=sOptions->startValue;i<sOptions->limit;i++){
			printf( \
			"%04x | %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x\n", \
			i, \
			readByteFromAddress(eeprom,i), readByteFromAddress(eeprom,i+1), readByteFromAddress(eeprom,i+2), \
			readByteFromAddress(eeprom,i+3), readByteFromAddress(eeprom,i+4), readByteFromAddress(eeprom,i+5), \
			readByteFromAddress(eeprom,i+6), readByteFromAddress(eeprom,i+7), readByteFromAddress(eeprom,i+8), \
			readByteFromAddress(eeprom,i+9), readByteFromAddress(eeprom,i+10), readByteFromAddress(eeprom,i+11), \
			readByteFromAddress(eeprom,i+12), readByteFromAddress(eeprom,i+13), readByteFromAddress(eeprom,i+14), \
			readByteFromAddress(eeprom,i+15));
			i = i+15;
		}
		break;
	default:
		for (int i=sOptions->startValue;i<sOptions->limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(eeprom,i));
		}
		break;
	}
}
