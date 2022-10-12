#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "piepro.h"

int loggingLevel = WARNING;

int main(int argc, char *argv[]){
	enum FILE_TYPE {TEXT_FILE,BINARY_FILE};
	enum APP_FUNCTIONS {WRITE_FILE_TO_ROM,COMPARE_ROM_TO_FILE,DUMP_ROM,WRITE_SINGLE_BYTE_TO_ROM};

	FILE *romFile;
	char *filename;
	struct Eeprom eeprom;

	// defaults
	long limit = -1;
	long startValue = 0;
	int dumpFormat = 0;
	int validateWrite = 1;
	int force = 0;
	int action = WRITE_FILE_TO_ROM;
	int fileType = TEXT_FILE;
	int eepromModel = AT28C16;
	int writeCycleUSec = -1;
	char i2cAddress = 0x50;

	int addressParam = 0;
	int dataParam = 0;

	// TODO: Add serial device support -p --parallel -s --serial
	// TODO: Support start value for text files
	// TODO: Add write cycle time adjustment from command line
	// TODO: Add non-cmos support

	if (argc == 1){
		printHelp();
		return 1;
	} else {
		filename = argv[argc-1];
		for(int i=argc-1;i>0;i--){
			// -h --help
			if (!(strcmp(argv[i],"-h")) || (!strcmp(argv[i],"--help"))){
				printHelp();
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
				if (verbosity >= 0 && verbosity <= 5){
					loggingLevel = verbosity;
					ulog(INFO,"Setting verbosity to [%i] %s",verbosity,LOGLEVELSTRINGS[verbosity]);
				} else {
					ulog(FATAL,"Unsupported verbosity value");
					return 1;
				}
			}
		}

		for(int i=argc-1;i>0;i--){
			// -i --initial
			if (!strcmp(argv[i],"-s") || !strcmp(argv[i],"--start")){
				ulog(INFO,"Setting starting value to %i",str2num(argv[i+1]));
				startValue = str2num(argv[i+1]);
				if ( startValue == -1){
					ulog(FATAL,"Unsupported starting value");
					return 1;
				}
			}

			// -l --limit
			if (!strcmp(argv[i],"-l") || !strcmp(argv[i],"--limit")){
				ulog(INFO,"Setting limit to %i",str2num(argv[i+1]));
				limit = str2num(argv[i+1]);
				if ( limit== -1){
					ulog(FATAL,"Unsupported limit value");
					return 1;
				}
			}

			// -b --binary 
			if (!strcmp(argv[i],"-b") || !strcmp(argv[i],"--binary")){
				ulog(INFO,"Setting filetype to binary");
				fileType = BINARY_FILE;
			}

			// -wd --write-delay
			if (!strcmp(argv[i],"-wd") || !strcmp(argv[i],"--write-delay")){
				ulog(INFO,"Setting write cycle delay time to %i",str2num(argv[i+1]));
				writeCycleUSec = str2num(argv[i+1]);
				if ( writeCycleUSec == -1){
					ulog(FATAL,"Unsupported delay value");
					return 1;
				}
			}

			// -id --i2c-device-id
			if (!strcmp(argv[i],"-id") || !strcmp(argv[i],"--i2c-device-id")){
				ulog(INFO,"Setting I2C id to %i",str2num(argv[i+1]));
				i2cAddress = str2num(argv[i+1]);
				if ( i2cAddress == -1){
					ulog(FATAL,"Unsupported I2C id value");
					return 1;
				}
			}

			// -d --dump
			if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dump")){
				if (action == COMPARE_ROM_TO_FILE || action == WRITE_SINGLE_BYTE_TO_ROM){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Dumping ROM to standard out");
					dumpFormat = str2num(argv[i+1]);
					action = DUMP_ROM;
				}
			}

			// -w --write
			if (!strcmp(argv[i],"-w") || !strcmp(argv[i],"--write")){
				if (action == COMPARE_ROM_TO_FILE || action == DUMP_ROM){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					
					addressParam = str2num(argv[i+1]);
					dataParam = str2num(argv[i+2]);
					if ( addressParam == -1 || dataParam == -1) {
						ulog(ERROR,"Unsupported number format. Exiting.");
						return 1;
					}
					if (dataParam > 0xFF){
						ulog(ERROR,"Data byte too large. Please specify a value less than 256.");
						return 1;
					}
					ulog(INFO,"Writing Byte %s to Address %s",argv[i+2],argv[i+1]);
					action = WRITE_SINGLE_BYTE_TO_ROM;
				}
			}

			// -c --compare
			if (!strcmp(argv[i],"-c") || !strcmp(argv[i],"--compare")){
				if (action == DUMP_ROM || action == WRITE_SINGLE_BYTE_TO_ROM){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Comparing ROM to File");
					action = COMPARE_ROM_TO_FILE;
				}
			}

			// --no-validate-write
			if (!strcmp(argv[i],"--no-validate-write")){
				ulog(WARNING,"Disabling write verification");
				validateWrite = 0;
			}

			// -f --force
			if (!strcmp(argv[i],"-f") || !strcmp(argv[i],"--force")){
					ulog(INFO,"Forcing all writes even if value is already present.");
					force = 1;
			}

			// -m --model
			if (!strcmp(argv[i],"-m") || !strcmp(argv[i],"--model")){
				if (!strcmp(argv[i+1],"at28c16")){
					eepromModel = AT28C16;
				} else if (!strcmp(argv[i+1],"at28c64")) {
					eepromModel = AT28C64;
				} else if (!strcmp(argv[i+1],"at28c256")) {
					eepromModel = AT28C256;
				} else if (!strcmp(argv[i+1],"at24c01")) {
					eepromModel = AT24C01;
				} else if (!strcmp(argv[i+1],"at24c02")) {
					eepromModel = AT24C02;
				} else {
					ulog(FATAL,"Unsupported ROM Model");
					return 1;
				}
				ulog(INFO,"Setting rom model to %s",argv[i+1]);
			}
		}
	}

	// init
	if (eepromModel >= AT24C01 && eepromModel <= AT24C512){
		eeprom.fd = wiringPiI2CSetup(i2cAddress);		
	} else {
		if (-1 == wiringPiSetup()) {
			ulog(FATAL,"Failed to setup Wiring Pi!");
			return 1;
		}
	}	

	eeprom.writeCycleWait = writeCycleUSec;
	if ( 0 != init(&eeprom, eepromModel)){
		return 1;
	}

	switch(action){
		case WRITE_FILE_TO_ROM: case COMPARE_ROM_TO_FILE:
			// open file to read
			romFile = fopen(filename, "r");
			if(romFile == NULL){
				ulog(FATAL,"Error Opening File");
				return 1;
			}

			switch(fileType | (action<<8)){
				case TEXT_FILE| (WRITE_FILE_TO_ROM<<8):
					writeTextFileToEEPROM(&eeprom,romFile,validateWrite, force, limit);
					break;
				case BINARY_FILE | (WRITE_FILE_TO_ROM<<8):
					writeBinaryFileToEEPROM(&eeprom,romFile,validateWrite,force,startValue,limit);
					break;
				case TEXT_FILE | (COMPARE_ROM_TO_FILE<<8):
					if(compareTextFileToEEPROM(&eeprom,romFile,limit)){
						ulog(ERROR,"EEPROM does not match file");
					}
					break;
				case BINARY_FILE | (COMPARE_ROM_TO_FILE<<8):
					if(compareBinaryFileToEEPROM(&eeprom,romFile,startValue,limit)){
						ulog(ERROR,"EEPROM does not match file");
					}
					break;
			}
			
			fclose(romFile);
			break;
		case DUMP_ROM:
			printROMContents(&eeprom,startValue,limit,dumpFormat);
			break;
		case WRITE_SINGLE_BYTE_TO_ROM:
			writeByteToAddress(&eeprom,addressParam,dataParam,validateWrite,force,NULL);
			break;
	}
	return 0;
}

/* Open and write a text file to Memory */
int writeTextFileToEEPROM(struct Eeprom *eeprom, FILE *memoryFile,int validate, char force, unsigned long limit){
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

	while( ((c = fgetc(memoryFile)) != EOF ) && counter < limit){
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
			err = writeByteToAddress( \
							eeprom,binStr2num(textFileAddress),binStr2num(textFiledata),validate,force,&byteWriteCounter);
			addressLength = 0;
			dataLength = 0;
			haveNotReachedSeparator = 1;
			counter++;
		}
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return 0;
}

int compareTextFileToEEPROM(struct Eeprom *eeprom,FILE *memoryFile, unsigned long limit){
	int addressToCompare = 0, dataToCompare = 0, err = 0;

	int counter = 0;
	
	char textFileAddress[eeprom->numAddressPins+1];
	char textFiledata[eeprom->numDataPins+1];
	int c;
	int haveNotReachedSeparator = 1;
	int addressLength = 0;
	int dataLength = 0;

	textFileAddress[eeprom->numAddressPins] = 0;
	textFiledata[eeprom->numDataPins] = 0;

	while( ((c = fgetc(memoryFile)) != EOF ) && (counter < limit)){
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
			if (readByteFromAddress(eeprom,addressToCompare) != dataToCompare){
				ulog(INFO,"Byte at Address 0x%02x does not match. Rom: %i File: %i", \
					addressToCompare,readByteFromAddress(eeprom,addressToCompare),dataToCompare);
				err = 1;
			}
			addressLength = 0;
			dataLength = 0;
			haveNotReachedSeparator = 1;
			counter++;
		}
	}
	return err;
}

/* Open and write a binary file to Memory */
int writeBinaryFileToEEPROM(struct Eeprom* eeprom,FILE *memoryFile,int validate,char force,long begin, unsigned long limit){
	int c,addressToWrite = 0,err=0;
	int byteWriteCounter = 0;

	while (addressToWrite < begin && (fgetc(memoryFile) != EOF)){
		addressToWrite++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToWrite < limit) {
		err = writeByteToAddress(eeprom,addressToWrite++,c,validate,force,&byteWriteCounter);
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return err;
}

/* Compare a binary file to Rom */
int compareBinaryFileToEEPROM(struct Eeprom* eeprom,FILE *memoryFile, long begin, unsigned long limit){
	int c,addressToCompare = 0, err = 0;

	while (addressToCompare < begin && (fgetc(memoryFile) != EOF)){
		addressToCompare++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToCompare < limit) {
		if (readByteFromAddress(eeprom,addressToCompare) != (char)c){
			ulog(INFO,"Byte at Address 0x%02x does not match. Rom: %i File: %i", \
				addressToCompare,readByteFromAddress(eeprom,addressToCompare),c);
			err = 1;
		}
		addressToCompare++;
	}
	return err;
}

/* Read byte from specified Address */
char readByteFromAddress(struct Eeprom* eeprom,unsigned int addressToRead){
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
int writeByteToAddress(struct Eeprom* eeprom,unsigned int addressToWrite, \
						char dataToWrite,char verify,char force, int* byteWriteCounter){
	int err = 0;
	if (eeprom->model >= AT24C01 && eeprom->model <= AT24C512){
		if ( force || dataToWrite != wiringPiI2CReadReg8(eeprom->fd, addressToWrite )){
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
		if ( force || dataToWrite != readByteFromAddress(eeprom,addressToWrite)){
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
			if (verify == 1){
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
void setAddressPins(struct Eeprom* eeprom,unsigned int addressToSet){
	for (char pin = 0;pin<eeprom->numAddressPins;pin++){
		if (!((eeprom->model == AT28C64) && ((pin == 13) || (pin == 14)))){
			digitalWrite(eeprom->addressPins[pin],(addressToSet & 1));
			addressToSet >>= 1;
		}
	}
}

/* Set Data eeprom to value to write */
void setDataPins(struct Eeprom* eeprom,char dataToSet){
	for (char pin = 0;pin<eeprom->numDataPins;pin++){
		digitalWrite(eeprom->dataPins[pin],(dataToSet & 1));
		dataToSet >>= 1;
	}
}

/* Converts a number to a binary String */
char *num2binStr(char *binStrBuf, int num, int strBufLen){
    for (int i=strBufLen-1;i>=0;i--){
		binStrBuf[i] = (char)((num%2)+0x30);
		num >>= 1;
    }
	if(num !=0){
		ulog(WARNING,"Error: String Buffer not large enough to hold number: %i",num);
	}
    binStrBuf[strBufLen] = 0;
    return binStrBuf;
}

/* Converts a binary string to a number. Will skip characters that are not '1' or '0' */
int binStr2num(const char *binStr){
	int num = 0, num_size = 0;
	for (int i=strlen(binStr)-1,j=0;i>=0;i--,j++){
		if (binStr[i] == '1' || binStr[i] == '0'){
			if (num_size < strlen(binStr) && num_size < 32){
				num += (binStr[i]-48)*(1 << j);
				num_size++;
			} else {
				ulog(ERROR,"Number out of Range");
				num = -1;
				i = -1;
			}
		}
	}
	return num;
}

/* Converts a number string to a number.*/
int str2num(char *numStr){
	int num = 0;
	for (int i=strlen(numStr)-1,j=0;i>=0;i--){
		if (numStr[0] == '0' && numStr[1] == 'x'){ 
			// convert hexidecimal number
			if(!(i < 2) ){
				if ((numStr[i] >= '0' && numStr[i] <= '9') || (numStr[i] >= 'A' && numStr[i] <= 'F') \
					|| (numStr[i] >= 'a' && numStr[i] <= 'f') ){
					if (j < 8){
						if (numStr[i] >= 'A' && numStr[i] <= 'F') {
							num += (numStr[i]-0x37)*(expo(16 , j++));
						} else if (numStr[i] >= 'a' && numStr[i] <= 'f'){
							num += (numStr[i]-0x57)*(expo(16 , j++));
						} else {
							num += (numStr[i]-0x30)*(expo(16 , j++));
						}
					} else {
						ulog(ERROR,"Number out of Range");
						num = -1;
						i = -1;
					}
				} else {
					ulog(ERROR,"Not a valid hexidecimal number");
					num = -1;
					i = -1;
				}
			}
		} else { 
			// convert decimal number
			if (numStr[i] >= '0' && numStr[i] <= '9'){
				if (j < 11){
					num += (numStr[i]-0x30)*(expo(10 , j++));
				} else {
					ulog(ERROR,"Number out of Range");
					num = -1;
					i = -1;
				}
			} else {
				ulog(ERROR,"Not a valid decimal number");
				num = -1;
				i = -1;
			}
		}
	}
	return num;
}

/* Exponent function */
long expo(int base, int power){
	int result = base;
	if (power == 0){
		result = 1;
	} else {
		for (int i = 1; i < power; i++){
			result *= base;
		}
	}
    return result;
}

/* Initialize rpi to write */
int init(struct Eeprom *eeprom,int eepromModel){
	eeprom->model = eepromModel;
	eeprom->size = EEPROM_MODEL_SIZES[eepromModel];
	eeprom->numAddressPins = (EEPROM_NUM_ADDRESS_PINS[eepromModel]);
	eeprom->numDataPins = (EEPROM_NUM_DATA_PINS[eepromModel]);
	if(eeprom->writeCycleWait == -1){
		eeprom->writeCycleWait = EEPROM_WRITE_CYCLE_USEC[eepromModel];
	}
	
	if (eepromModel >= AT24C01 && eepromModel <= AT24C256){
		// eeprom->addressPins[0] = 23; // 13 // 33
		// eeprom->addressPins[1] = 24; // 19 // 35
		// eeprom->addressPins[2] = 25; // 26 // 37

		// eeprom->vccPin = 26; // 12 // 32
		// eeprom->writeProtectPin = 27; // 16 // 36


		// for(int i=0;i<3;i++){
		// 	pinMode(eeprom->addressPins[i], OUTPUT);
		// 	digitalWrite(eeprom->addressPins[i], LOW);
		// }

		// pinMode(eeprom->writeProtectPin, OUTPUT);
		// pinMode(eeprom->vccPin, OUTPUT);
		// digitalWrite(eeprom->writeProtectPin, LOW);
		// digitalWrite(eeprom->vccPin, HIGH);

	} else {
				/*   WiPi // GPIO // Pin   */ 
		eeprom->addressPins[14] = 8; // 2 // 3 !Not Used on AT28C16
		eeprom->addressPins[12] = 9; // 3 // 5 !Not Used on AT28C16
		
		eeprom->addressPins[7] = 7; // 4 // 7
		eeprom->addressPins[6] = 0; // 17 // 11
		eeprom->addressPins[5] = 2; // 27 // 13
		eeprom->addressPins[4] = 3; // 22 // 15
		eeprom->addressPins[3] = 12; // 10 // 19
		eeprom->addressPins[2] = 13; // 9 // 21
		eeprom->addressPins[1] = 14; // 11 // 23
		eeprom->addressPins[0] = 30; // 0 // 27

		eeprom->dataPins[0] = 21; // 5 // 29
		eeprom->dataPins[1] = 22; // 6 // 31
		eeprom->dataPins[2] = 23; // 13 // 33
		// 24; // 19 // 35
		// 25; // 26 // 37


		if (eepromModel == AT28C64 || eepromModel == AT28C256){
			eeprom->writeEnablePin =  15; // 14 // 8
			
			eeprom->addressPins[13] = 16; // 15 // 10
			eeprom->addressPins[11] = 5; // 24 // 18
		} else if (eepromModel == AT28C16){
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

void waitWriteCycle(int usec){
	usleep(usec);
}

/* Prints help message */
void printHelp(){
	printf("Usage: piepro [options] [file]\n");
	printf("Options:\n");
	printf(" -b,   --binary			Interpret file as a binary. Default: text\n");
	printf("					Text File format:\n");
	printf("					00000000 00000000\n");
	printf(" -c,   	--compare		Compare file and EEPROM and print differences.\n");
	printf(" -d N, 	--dump N		Dump the contents of the EEPROM, 0=DEFAULT, 1=BINARY, 2=TEXT, 3=PRETTY.\n");
	printf(" -f,   	--force			Force writing of every bite instead of checking for existing value first.\n");
	printf(" -id,   --i2c-device-id	The Address id of the I2C device.\n");
	printf(" -h,   	--help			Print this message and exit.\n");
	printf(" -l N, 	--limit N		Specify the maximum address to operate.\n");
	printf("       	--no-validate-write	Do not perform a read directly after writing to verify the data was written.\n");
	printf(" -m MODEL, --model MODEL	Specify EERPOM device model. Default: AT28C16.\n");
	printf(" -s N, 	--start N		Specify the minimum address to operate.\n");
	printf(" -v N, 	--v[vvvv]		Set the log verbosity to N, 0=OFF, 1=FATAL, 2=ERROR, 3=WARNING, 4=INFO, 5=DEBUG.\n");
	printf(" -w ADDRESS DATA, \n");
	printf("\t--write ADDRESS DATA	Write specified DATA to ADDRESS.\n");
	printf(" -wd N, --write-delay N		Number of microseconds to delay between writes.\n");
	printf("\n");
}

/* Prints the Rom's Contents to the specified limit */
void printROMContents(struct Eeprom* eeprom, long begin,long limit,int format){
	if (limit == -1 || limit > EEPROM_MODEL_SIZES[eeprom->model]){
		limit = EEPROM_MODEL_SIZES[eeprom->model];
	}
	
	switch (format) {
	case 0:
		for (int i=begin;i<limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(eeprom,i));
		}
		break;
	case 1:	// binary
		loggingLevel=OFF;
		for (int i=0;i<begin;i++) {
			putc(0xFF,stdout);
		}
		for (int i=begin;i<limit;i++) {
			putc(readByteFromAddress(eeprom,i),stdout);
		}
		break;
	case 2: // text
		for (int i=begin;i<limit;i++) {
			char addressBinStr[eeprom->numAddressPins+2];
			char dataBinStr[eeprom->numDataPins+1];

			num2binStr(addressBinStr,i,eeprom->numAddressPins+1);
			num2binStr(dataBinStr,readByteFromAddress(eeprom,i),eeprom->numDataPins);
			if ( begin < 0x100 && limit < 0x100){
				char shortAddressBinStr[9];
				strncpy(shortAddressBinStr,&addressBinStr[8],8);
				printf("%s %s \n", shortAddressBinStr,dataBinStr);
			} else {
				printf("%s %s \n",addressBinStr,dataBinStr);
			}
		}
		break;
	case 3: // pretty
		if ((begin % 16) != 0){
			begin = begin - ((begin % 16));
		}
		printf("       00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\n");
		printf("       ===============================================================\n");
		for (int i=begin;i<limit;i++){
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
		for (int i=begin;i<limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(eeprom,i));
		}
		break;
	}
}

void ulog(int verbosity, const char* logMessage,...) {
	if (verbosity <= loggingLevel){
		char logBuf[120];
		va_list args;
		va_start(args, logMessage);
		vsnprintf(logBuf,sizeof(logBuf),logMessage, args);
		va_end(args);
    	printf("%s:\t%s\n", LOGLEVELSTRINGS[verbosity],logBuf);
	}
}