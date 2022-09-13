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
	struct Pins pins;
	
	int action = WRITE_FILE_TO_ROM;
	int fileType = TEXT_FILE;
	int romType = AT28C16;

	// TODO: Handle device type -t --type
	// TODO: Add serial device support -p --parallel -s --serial
	// TODO: Support start value for text files


	if (argc == 1){
		printHelp();
		return 0;
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
	init(romType,&pins);

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
					writeTextFileToEEPROM(&pins,romFile,validateWrite,limit);
					break;
				case BINARY_FILE | (WRITE_FILE_TO_ROM<<8):
					writeBinaryFileToEEPROM(&pins,romFile,validateWrite,startValue,limit);
					break;
				case TEXT_FILE | (COMPARE_ROM_TO_FILE<<8):
					compareTextFileToEEPROM(&pins,romFile,limit);
					break;
				case BINARY_FILE | (COMPARE_ROM_TO_FILE<<8):
					compareBinaryFileToEEPROM(&pins,romFile,startValue,limit);
					break;
			}
			
			fclose(romFile);
			break;
		case DUMP_ROM:
			printROMContents(&pins,startValue,limit,dumpFormat);
			break;
	}
	return 0;
}

/* Open and write a text file to Memory */
int writeTextFileToEEPROM(struct Pins *pins, FILE *memoryFile,int validate, unsigned long limit){

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
			err = writeByteToAddress(pins,binStr2num(textFileAddress),binStr2num(textFiledata),validate,&byteWriteCounter);
			addressLength = 0;
			dataLength = 0;
			haveNotReachedSeparator = 1;
			counter++;
		}
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return 0;
}

int compareTextFileToEEPROM(struct Pins *pins,FILE *memoryFile, unsigned long limit){
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
			if (readByteFromAddress(pins,addressToCompare) != dataToCompare){
				printf("Byte at Address 0x%02x does not match. Rom: %i File: %i\n", \
					addressToCompare,readByteFromAddress(pins,addressToCompare),dataToCompare);
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
int writeBinaryFileToEEPROM(struct Pins* pins,FILE *memoryFile,int validate,long begin, unsigned long limit){
	int c,addressToWrite = 0,err=0;
	int byteWriteCounter = 0;
	int counter = 0;

	while (addressToWrite < begin && (fgetc(memoryFile) != EOF)){
		addressToWrite++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToWrite < limit) {
		err = writeByteToAddress(pins,addressToWrite++,c,validate,&byteWriteCounter);
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return err;
}

/* Compare a binary file to Rom */
int compareBinaryFileToEEPROM(struct Pins* pins,FILE *memoryFile, long begin, unsigned long limit){
	int c,addressToCompare = 0, err = 0;

	while (addressToCompare < begin && (fgetc(memoryFile) != EOF)){
		addressToCompare++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToCompare < limit) {
		if (readByteFromAddress(pins,addressToCompare) != (char)c){
			printf("Byte at Address 0x%02x does not match. Rom: %i File: %i\n", \
				addressToCompare,readByteFromAddress(pins,addressToCompare),c);
			err = 1;
		}
		addressToCompare++;
	}
	return err;
}

/* Read byte from specified Address */
char readByteFromAddress(struct Pins* pins,unsigned short addressToRead){
	char binAdrStr[NUM_DATA_PINS+1];
	binAdrStr[NUM_DATA_PINS] = 0;
	// set the address
	setAddressPins(pins,addressToRead);
	// enable output from the chip
	digitalWrite(pins->outputEnable,LOW);
	// set the rpi to input on it's gpio data lines
	for(int i=0;i<NUM_DATA_PINS;i++){
		pinMode(pins->dataPins[i],INPUT);
	}
	// read the pins and store to string
	for(int i=0,j=NUM_DATA_PINS-1;i<NUM_DATA_PINS;i++,j--){
		binAdrStr[i] = (char)(digitalRead(pins->dataPins[j])+0x30);
	}

	// convert the string to a number and return the number
	return binStr2num(binAdrStr);
}

/* Write specified byte to specified address */
int writeByteToAddress(struct Pins* pins,unsigned short addressToWrite, char dataToWrite,char verify,int* byteWriteCounter){
	char binAdrStr[NUM_DATA_PINS+1];
	binAdrStr[NUM_DATA_PINS] = 0;
	int err = 0;

	// set the address
	setAddressPins(pins,addressToWrite);
	// disable output from the chip
	digitalWrite(pins->outputEnable,HIGH);
	// set the rpi to output on it's gpio data lines
	for(int i=0;i<NUM_DATA_PINS;i++){
		pinMode(pins->dataPins[i],OUTPUT);
	}
	// Set the data pins to the data to be written
	setDataPins(pins,dataToWrite);

	// perform the write
	digitalWrite(pins->writeEnable,HIGH);
	usleep(200);
	digitalWrite(pins->writeEnable,LOW);
	usleep(200);
	if (verify == 1){
		// printf("Verifying Byte %i at Address %i\n",dataToWrite,addressToWrite);
		if ( dataToWrite != readByteFromAddress(pins,addressToWrite)){
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

/* Set Address pins to value to read from or write to */
void setAddressPins(struct Pins* pins,unsigned short addressToSet){
	char binStr[NUM_ADDRESS_PINS];
	num2binStr(binStr,addressToSet,sizeof(binStr)/sizeof(binStr[0]));
	char pin = NUM_ADDRESS_PINS-1;
	for (char c = 0;c<NUM_ADDRESS_PINS;c++){
		digitalWrite(pins->addressPins[pin],binStr[c]-0x30);
		pin--;
	}
}

/* Set Data pins to value to write */
void setDataPins(struct Pins* pins,char dataToSet){
	char binStr[NUM_DATA_PINS];
	num2binStr(binStr,dataToSet,sizeof(binStr)/sizeof(binStr[0]));

	char pin = NUM_DATA_PINS-1;
	for (char c = 0;c<NUM_DATA_PINS;c++){
		digitalWrite(pins->dataPins[pin],binStr[c]-0x30);
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
int init(int romType,struct Pins *pins){
			/*   WiPi // GPIO // Pin   */ 
	pins->addressPins[14] = 8; // 2 // 3
	pins->addressPins[12] = 9; // 3 // 5
	
	pins->addressPins[7] = 7; // 4 // 7
	pins->addressPins[6] = 0; // 17 // 11
	pins->addressPins[5] = 2; // 27 // 13
	pins->addressPins[4] = 3; // 22 // 15
	pins->addressPins[3] = 12; // 10 // 19
	pins->addressPins[2] = 13; // 9 // 21
	pins->addressPins[1] = 14; // 11 // 23
	pins->addressPins[0] = 30; // 0 // 27

	pins->dataPins[0] = 21; // 5 // 29
	pins->dataPins[1] = 22; // 6 // 31
	pins->dataPins[2] = 23; // 13 // 33

	// 24; // 19 // 35
	// 25; // 26 // 37

	if (romType == AT28C64 || romType == AT28C256){
		pins->writeEnable =		  15; // 14 // 8
		pins->addressPins[13] = 16; // 15 // 10
		pins->addressPins[8] = 1; // 18 // 12
		pins->addressPins[9] = 4; // 23 // 16
		pins->addressPins[11] = 5; // 24 // 18
	} else if (romType == AT28C16){
		pins->addressPins[8] = 1; // 18 // 12
		pins->addressPins[9] = 4; // 23 // 16
		pins->writeEnable =  5; // 24 // 18
	}

	pins->outputEnable =	   6; // 25 // 22
	pins->addressPins[10] = 10; // 8 // 24
	pins->chipEnable =	  11; // 7 // 26
	pins->dataPins[7] = 31; // 1 // 28
	pins->dataPins[6] = 26; // 12 // 32
	pins->dataPins[5] = 27; // 16 // 36
	pins->dataPins[4] = 28; // 20 // 38
	pins->dataPins[3] = 29; // 21 // 40
	
	for(int i=0;i<NUM_ADDRESS_PINS;i++){
		pinMode(pins->addressPins[i], OUTPUT);
		digitalWrite(pins->addressPins[i], LOW);
	}

	for(int i=0;i<NUM_DATA_PINS;i++){
		pinMode(pins->dataPins[i], OUTPUT);
		digitalWrite(pins->dataPins[i], LOW);
	}

	pinMode(pins->chipEnable, OUTPUT);
	pinMode(pins->outputEnable, OUTPUT);
	pinMode(pins->writeEnable, OUTPUT);
	digitalWrite(pins->chipEnable, LOW);
	digitalWrite(pins->outputEnable, HIGH);
	digitalWrite(pins->writeEnable, LOW);

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
void printROMContents(struct Pins* pins, long begin,long limit,int format){
	if (limit == -1){
		limit = 256;
	}

	switch (format) {
	case 0:
		for (int i=begin;i<limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(pins,i));
		}
		break;
	case 1:	// binary
		loggingLevel=OFF;
		for (int i=0;i<begin;i++) {
			putc(0xFF,stdout);
		}
		for (int i=begin;i<limit;i++) {
			putc(readByteFromAddress(pins,i),stdout);
		}
		break;
	case 2: // text
		for (int i=begin;i<limit;i++) {
			char addressBinStr[NUM_ADDRESS_PINS+1];
			char dataBinStr[NUM_DATA_PINS+1];

			num2binStr(addressBinStr,i,NUM_ADDRESS_PINS);
			num2binStr(dataBinStr,readByteFromAddress(pins,i),NUM_DATA_PINS);
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
			readByteFromAddress(pins,i), readByteFromAddress(pins,i+1), readByteFromAddress(pins,i+2), \
			readByteFromAddress(pins,i+3), readByteFromAddress(pins,i+4), readByteFromAddress(pins,i+5), \
			readByteFromAddress(pins,i+6), readByteFromAddress(pins,i+7), readByteFromAddress(pins,i+8), \
			readByteFromAddress(pins,i+9), readByteFromAddress(pins,i+10), readByteFromAddress(pins,i+11), \
			readByteFromAddress(pins,i+12), readByteFromAddress(pins,i+13), readByteFromAddress(pins,i+14), \
			readByteFromAddress(pins,i+15));
			i = i+15;
		}
		break;
	default:
		for (int i=begin;i<limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(pins,i));
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