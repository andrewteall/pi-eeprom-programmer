#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <wiringPi.h>

#include "piepro.h"

int loggingLevel = WARNING;

int main(int argc, char *argv[]){
	enum FILE_TYPE {TEXT_FILE,BINARY_FILE};
	enum APP_FUNCTIONS {WRITE_FILE_TO_ROM,COMPARE_ROM_TO_FILE,DUMP_ROM};

	FILE *romFile;
	char *filename;

	// defaults
	long limit = -1;
	long startValue = 0;
	int dumpFormat = 0;
	int validateWrite = 1;
	struct Eeprom eeprom;
	
	int action = WRITE_FILE_TO_ROM;
	int fileType = TEXT_FILE;
	int romType = AT28C16;

	// TODO: Add serial device support -p --parallel -s --serial
	// TODO: Support start value for text files


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
					ulog(DEBUG,"Setting verbosity to [%i] %s",verbosity,LOGLEVELSTRINGS[verbosity]);
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

			// -d --dump
			if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dump")){
				if (action == COMPARE_ROM_TO_FILE){
					ulog(WARNING,"-d or --dump flag specified but -c or --compare has already be set. Ignoring -d or --dump flag.");
				} else {
					ulog(INFO,"Dumping ROM to standard out");
					dumpFormat = str2num(argv[i+1]);
					action = DUMP_ROM;
				}
			}

			// --no-validate-write
			if (!strcmp(argv[i],"--no-validate-write")){
				ulog(WARNING,"Disabling write verification");
				validateWrite = 0;
			}

			// -c --compare
			if (!strcmp(argv[i],"-c") || !strcmp(argv[i],"--compare")){
				if (action == DUMP_ROM){
					ulog(WARNING,"-c or --compare flag specified but -d or --dump has already be set. Ignoring -c or --compare flag.");
				} else {
					ulog(INFO,"Comparing ROM to File");
					action = COMPARE_ROM_TO_FILE;
				}
			}

			// -rt --romtype
			if (!strcmp(argv[i],"-rt") || !strcmp(argv[i],"--romtype")){
				if (!strcmp(argv[i+1],"at28c16")){
					romType = AT28C16;
				} else if (!strcmp(argv[i+1],"at28c64")) {
					romType = AT28C64;
				} else if (!strcmp(argv[i+1],"at28c256")) {
					romType = AT28C256;
				} else {
					ulog(FATAL,"Unsupported ROM type");
					return 1;
				}
				ulog(INFO,"Setting rom type to %s",ROMTYPESTRINGS[romType]);
			}
		}
	}
	
	// init
	if (-1 == wiringPiSetup()) {
		ulog(FATAL,"Failed to setup Wiring Pi!");
		return 1;
	}
	init(romType,&eeprom);

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
					writeTextFileToEEPROM(&eeprom,romFile,validateWrite,limit);
					break;
				case BINARY_FILE | (WRITE_FILE_TO_ROM<<8):
					writeBinaryFileToEEPROM(&eeprom,romFile,validateWrite,startValue,limit);
					break;
				case TEXT_FILE | (COMPARE_ROM_TO_FILE<<8):
					compareTextFileToEEPROM(&eeprom,romFile,limit);
					break;
				case BINARY_FILE | (COMPARE_ROM_TO_FILE<<8):
					compareBinaryFileToEEPROM(&eeprom,romFile,startValue,limit);
					break;
			}
			
			fclose(romFile);
			break;
		case DUMP_ROM:
			printROMContents(&eeprom,startValue,limit,dumpFormat);
			break;
	}
	return 0;
}

/* Open and write a text file to Memory */
int writeTextFileToEEPROM(struct Eeprom *eeprom, FILE *memoryFile,int validate, unsigned long limit){

	int counter = 0;
	
	char textFileAddress[NUM_ADDRESS_PINS+1];
	char textFiledata[NUM_DATA_PINS+1];
	int c;
	int err = 0;
	int haveNotReachedSeparator = 1;
	int addressLength = 0;
	int dataLength = 0;
	int byteWriteCounter = 0;

	textFileAddress[NUM_ADDRESS_PINS] = 0;
	textFiledata[NUM_DATA_PINS] = 0;

	while( ((c = fgetc(memoryFile)) != EOF ) && counter < limit){
		if (c != '\n'){
			if ((addressLength < NUM_ADDRESS_PINS) && (haveNotReachedSeparator) ){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				} else if (c == ' '){
					haveNotReachedSeparator = 0;
				}
			} else {
				if (dataLength < NUM_DATA_PINS){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			if (addressLength < NUM_ADDRESS_PINS){
				for(int i = addressLength-1,j=NUM_ADDRESS_PINS-1;i>=0;i--,j--){
					textFileAddress[j]= textFileAddress[i];
					textFileAddress[i] = '0';
				}
			}
			// ulog(DEBUG,"Writing to File %s  %s",textFileAddress,textFiledata);
			// ulog(DEBUG,"Writing to File %i  %i",binStr2num(textFileAddress),binStr2num(textFiledata));
			err = writeByteToAddress(eeprom,binStr2num(textFileAddress),binStr2num(textFiledata),validate,&byteWriteCounter);
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
	
	char textFileAddress[NUM_ADDRESS_PINS+1];
	char textFiledata[NUM_DATA_PINS+1];
	int c;
	int haveNotReachedSeparator = 1;
	int addressLength = 0;
	int dataLength = 0;

	textFileAddress[NUM_ADDRESS_PINS] = 0;
	textFiledata[NUM_DATA_PINS] = 0;

	while( ((c = fgetc(memoryFile)) != EOF ) && (counter < limit)){
		if (c != '\n'){
			if ((addressLength < NUM_ADDRESS_PINS) && (haveNotReachedSeparator) ){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				} else if (c == ' '){
					haveNotReachedSeparator = 0;
				}
			} else {
				if (dataLength < NUM_DATA_PINS){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			if (addressLength < NUM_ADDRESS_PINS){
				for(int i = addressLength-1,j=NUM_ADDRESS_PINS-1;i>=0;i--,j--){
					textFileAddress[j]= textFileAddress[i];
					textFileAddress[i] = '0';

				}
			}
			addressToCompare  = binStr2num(textFileAddress);
			dataToCompare = binStr2num(textFiledata);
			if (readByteFromAddress(eeprom,addressToCompare) != dataToCompare){
				printf("Byte at Address 0x%02x does not match. Rom: %i File: %i\n", \
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
int writeBinaryFileToEEPROM(struct Eeprom* eeprom,FILE *memoryFile,int validate,long begin, unsigned long limit){
	int c,addressToWrite = 0,err=0;
	int byteWriteCounter = 0;
	int counter = 0;

	while (addressToWrite < begin && (fgetc(memoryFile) != EOF)){
		addressToWrite++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToWrite < limit) {
		err = writeByteToAddress(eeprom,addressToWrite++,c,validate,&byteWriteCounter);
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
			printf("Byte at Address 0x%02x does not match. Rom: %i File: %i\n", \
				addressToCompare,readByteFromAddress(eeprom,addressToCompare),c);
			err = 1;
		}
		addressToCompare++;
	}
	return err;
}

/* Read byte from specified Address */
char readByteFromAddress(struct Eeprom* eeprom,unsigned short addressToRead){
	char binAdrStr[NUM_DATA_PINS+1];
	binAdrStr[NUM_DATA_PINS] = 0;
	// set the address
	setAddressPins(eeprom,addressToRead);
	// enable output from the chip
	digitalWrite(eeprom->outputEnablePin,LOW);
	// set the rpi to input on it's gpio data lines
	for(int i=0;i<NUM_DATA_PINS;i++){
		pinMode(eeprom->dataPins[i],INPUT);
	}
	// read the eeprom and store to string
	for(int i=0,j=NUM_DATA_PINS-1;i<NUM_DATA_PINS;i++,j--){
		binAdrStr[i] = (char)(digitalRead(eeprom->dataPins[j])+0x30);
	}

	// convert the string to a number and return the number
	return binStr2num(binAdrStr);
}

/* Write specified byte to specified address */
int writeByteToAddress(struct Eeprom* eeprom,unsigned short addressToWrite, char dataToWrite,char verify,int* byteWriteCounter){
	char binAdrStr[NUM_DATA_PINS+1];
	binAdrStr[NUM_DATA_PINS] = 0;
	int err = 0;

	// set the address
	setAddressPins(eeprom,addressToWrite);
	// disable output from the chip
	digitalWrite(eeprom->outputEnablePin,HIGH);
	// set the rpi to output on it's gpio data lines
	for(int i=0;i<NUM_DATA_PINS;i++){
		if ((eeprom->type == AT28C64) && ((i == 13) || (i == 14))){
			// handle NC pins
			pinMode(eeprom->dataPins[i], INPUT);	
		} else {
			pinMode(eeprom->dataPins[i], OUTPUT);
		}
	}
	// Set the data eeprom to the data to be written
	setDataPins(eeprom,dataToWrite);

	// perform the write
	digitalWrite(eeprom->writeEnablePin,HIGH);
	usleep(200);
	digitalWrite(eeprom->writeEnablePin,LOW);
	usleep(200);
	if (verify == 1){
		// printf("Verifying Byte %i at Address %i\n",dataToWrite,addressToWrite);
		if ( dataToWrite != readByteFromAddress(eeprom,addressToWrite)){
			ulog(WARNING,"Failed to Write Byte %i at Address %i",dataToWrite,addressToWrite);
			(*byteWriteCounter)--;
			err = -1;
		} else {
			ulog(DEBUG,"Wrote Byte %i at Address %i",dataToWrite,addressToWrite);
		}
	}
	(*byteWriteCounter)++;
	return err;
}

/* Set Address eeprom to value to read from or write to */
void setAddressPins(struct Eeprom* eeprom,unsigned short addressToSet){
	char binStr[NUM_ADDRESS_PINS];
	num2binStr(binStr,addressToSet,sizeof(binStr)/sizeof(binStr[0]));
	char pin = NUM_ADDRESS_PINS-1;
	for (char c = 0;c<NUM_ADDRESS_PINS;c++){
		digitalWrite(eeprom->addressPins[pin],binStr[c]-0x30);
		pin--;
	}
}

/* Set Data eeprom to value to write */
void setDataPins(struct Eeprom* eeprom,char dataToSet){
	char binStr[NUM_DATA_PINS];
	num2binStr(binStr,dataToSet,sizeof(binStr)/sizeof(binStr[0]));

	char pin = NUM_DATA_PINS-1;
	for (char c = 0;c<NUM_DATA_PINS;c++){
		digitalWrite(eeprom->dataPins[pin],binStr[c]-0x30);
		pin--;
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
int init(int romType,struct Eeprom *eeprom){

	eeprom->type = romType;

			/*   WiPi // GPIO // Pin   */ 
	eeprom->addressPins[14] = 8; // 2 // 3
	eeprom->addressPins[12] = 9; // 3 // 5
	
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


	if (romType == AT28C64 || romType == AT28C256){
		eeprom->writeEnablePin =		  15; // 14 // 8
		eeprom->addressPins[13] = 16; // 15 // 10
		eeprom->addressPins[8] = 1; // 18 // 12
		eeprom->addressPins[9] = 4; // 23 // 16
		eeprom->addressPins[11] = 5; // 24 // 18
	} else if (romType == AT28C16){
		eeprom->addressPins[8] = 1; // 18 // 12
		eeprom->addressPins[9] = 4; // 23 // 16
		eeprom->writeEnablePin =  5; // 24 // 18
	}

	eeprom->outputEnablePin =	   6; // 25 // 22
	eeprom->addressPins[10] = 10; // 8 // 24
	eeprom->chipEnablePin =	  11; // 7 // 26
	eeprom->dataPins[7] = 31; // 1 // 28
	eeprom->dataPins[6] = 26; // 12 // 32
	eeprom->dataPins[5] = 27; // 16 // 36
	eeprom->dataPins[4] = 28; // 20 // 38
	eeprom->dataPins[3] = 29; // 21 // 40
	
	for(int i=0;i<NUM_ADDRESS_PINS;i++){
		pinMode(eeprom->addressPins[i], OUTPUT);
		digitalWrite(eeprom->addressPins[i], LOW);
	}

	for(int i=0;i<NUM_DATA_PINS;i++){
		if ((eeprom->type == AT28C64) && ((i == 13) || (i == 14))){
			// handle NC pins
			pinMode(eeprom->dataPins[i], INPUT);	
		} else {
			pinMode(eeprom->dataPins[i], OUTPUT);
			digitalWrite(eeprom->dataPins[i], LOW);
		}
	}

	pinMode(eeprom->chipEnablePin, OUTPUT);
	pinMode(eeprom->outputEnablePin, OUTPUT);
	pinMode(eeprom->writeEnablePin, OUTPUT);
	digitalWrite(eeprom->chipEnablePin, LOW);
	digitalWrite(eeprom->outputEnablePin, HIGH);
	digitalWrite(eeprom->writeEnablePin, LOW);

	return 0;
}

/* Prints help message */
void printHelp(){
	printf("Usage: piepro [options] [file]\n");
	printf("Options:\n");
	printf(" -b,   --binary			Interpret file as a binary. Default: text\n");
	printf(" -c,   --compare		Compare file and EEPROM and print differences.\n");
	printf(" -d N,   --dump N		Dump the contents of the EEPROM, 0=DEFAULT, 1=BINARY, 2=TEXT, 3=PRETTY.\n");
	printf(" -h,   --help			Print this message and exit.\n");
	printf(" -l N, --limit N		Specify the maximum address to operate.\n");
	printf("       --no-validate-write	Do not perform a read directly after writing to verify the data was written.\n");
	printf(" -rt TYPE, --romtype TYPE	Specify EERPOM device type. Default: AT28C16.\n");
	printf(" -s N, --start N		Specify the minimum address to operate.\n");
	printf("					Text File format:\n");
	printf("					00000000 00000000\n");
	printf(" -v N, --v[vvvv]		Set the log verbosity to N, 0=OFF, 1=FATAL, 2=ERROR, 3=WARNING, 4=INFO, 5=DEBUG.\n");
	printf("\n");
}

/* Prints the Rom's Contents to the specified limit */
void printROMContents(struct Eeprom* eeprom, long begin,long limit,int format){
	if (limit == -1){
		limit = 256;
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
			char addressBinStr[NUM_ADDRESS_PINS+2];
			char dataBinStr[NUM_DATA_PINS+1];

			num2binStr(addressBinStr,i,NUM_ADDRESS_PINS+1);
			num2binStr(dataBinStr,readByteFromAddress(eeprom,i),NUM_DATA_PINS);
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
			printf("%04x | %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x  %02x\n", \
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

void ulog(int verbosity, char* logMessage,...) {
	if (verbosity <= loggingLevel){
		char logBuf[120];
		va_list args;
		va_start(args, logMessage);
		vsnprintf(logBuf,sizeof(logBuf),logMessage, args);
		va_end(args);
    	printf("%s:\t%s\n", LOGLEVELSTRINGS[verbosity],logBuf);
	}
}