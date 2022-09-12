#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <wiringPi.h>

#include "piepro.h"

     /*   WiPi // GPIO // Pin   */ 
// AT28C16
#define A0 14  //  11  //  23
#define A1 12  //  10  //  19
#define A2 3   //  22  //  15
#define A3 2   //  27  //  13
#define A4 0   //  17  //  11
#define A5 7   //  4   //  7
#define A6 9   //  3   //  5
#define A7 8   //  2   //  3

#define A8  6  //  25  //  22
#define A9  10 //  8   //  24
#define A10 11 //  7   //  26

// AT28C64
#define A11  1  // 18  //  12
#define A12  1 //  18  //  12

// AT28C256
#define A13  1  // 18  //  12
#define A14  1 //  18  //  12

#define D0 21  //  5   //  29
#define D1 22  //  6   //  31
#define D2 23  //  13  //  33
#define D3 29  //  21  //  40
#define D4 28  //  20  //  38
#define D5 27  //  16  //  36
#define D6 26  //  12  //  32
#define D7 31  //  1   //  28

#define CE 17
#define OE 5   //  24  //  18
#define WE 4   //  23  //  16

#define NUM_ADDRESS_PINS 13
#define NUM_DATA_PINS 8

int address[NUM_ADDRESS_PINS];
int data[NUM_DATA_PINS];
int loggingLevel = WARNING;

int main(int argc, char *argv[]){
	FILE *romFile;
	char *filename;
	long limit = -1;
	long startValue = 0;
	int dumpFormat = 0;
	int validateWrite = 1;
	int action = WRITE_FILE_TO_ROM;
	int fileType = TEXT_FILE;

	// TODO: Handle device type -t --type
	// TODO: Add serial device support -p --parallel -s --serial
	// TODO: Support start value for text files
	// TODO: Allow 2 byte addresses in text file

	// BUG: Make flags idempotent

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

			// -b --binary -t --text
			if (!strcmp(argv[i],"-t") || !strcmp(argv[i],"--text")){
				ulog(INFO,"Setting filetype to text");
				// action = WRITE_FILE_TO_ROM;
				fileType = TEXT_FILE;
			} else if (!strcmp(argv[i],"-b") || !strcmp(argv[i],"--binary")){
				ulog(INFO,"Setting filetype to binary");
				// action = WRITE_FILE_TO_ROM;
				fileType = BINARY_FILE;
			}

			// -d --dump
			if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dump")){
				ulog(INFO,"Dumping ROM to standard out");
				dumpFormat = str2num(argv[i+1]);
				action = DUMP_ROM;
			}

			// --no-validate-write
			if (!strcmp(argv[i],"--no-validate-write")){
				ulog(WARNING,"Disabling write verification");
				validateWrite = 0;
			}

			// -c --compare
			if (!strcmp(argv[i],"-c") || !strcmp(argv[i],"--compare")){
				ulog(INFO,"Comparing ROM to File");
				action = COMPARE_ROM_TO_FILE;
			}
		}
	}
	
	// init
	if (-1 == wiringPiSetup()) {
		ulog(FATAL,"Failed to setup Wiring Pi!");
		return 1;
	}
	init();

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
					writeTextFileToEEPROM(romFile,validateWrite,limit);
					break;
				case BINARY_FILE | (WRITE_FILE_TO_ROM<<8):
					writeBinaryFileToEEPROM(romFile,validateWrite,startValue,limit);
					break;
				case TEXT_FILE | (COMPARE_ROM_TO_FILE<<8):
					compareTextFileToEEPROM(romFile,limit);
					break;
				case BINARY_FILE | (COMPARE_ROM_TO_FILE<<8):
					compareBinaryFileToEEPROM(romFile,startValue,limit);
					break;
			}
			
			fclose(romFile);
			break;
		case DUMP_ROM:
			printROMContents(startValue,limit,dumpFormat);
			break;
	}
	return 0;
}

/* Open and write a text file to Memory */
int writeTextFileToEEPROM(FILE *memoryFile,int validate, unsigned long limit){
	int addressMaxLenth = 8;
	int dataMaxLenth = 8;
	int counter = 0;
	
	char textFileAddress[addressMaxLenth+1];
	char textFiledata[dataMaxLenth+1];
	int c;
	int err = 0;
	int addressLength = 0;
	int dataLength = 0;
	int byteWriteCounter = 0;

	textFileAddress[addressMaxLenth] = 0;
	textFiledata[dataMaxLenth] = 0;

	while( ((c = fgetc(memoryFile)) != EOF ) && counter < limit){
		if (c != '\n'){
			if (addressLength < addressMaxLenth){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				}
			} else {
				if (dataLength < dataMaxLenth){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			// ulog(DEBUG,"Writing to File %s  %s",textFileAddress,textFiledata);
			// ulog(DEBUG,"Writing to File %i  %i",binStr2num(textFileAddress),binStr2num(textFiledata));
			err = writeByteToAddress(binStr2num(textFileAddress),binStr2num(textFiledata),validate,&byteWriteCounter);
			addressLength = 0;
			dataLength = 0;
			counter++;
		}
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return 0;
}

int compareTextFileToEEPROM(FILE *memoryFile, unsigned long limit){
	int addressToCompare = 0, dataToCompare = 0, err = 0;

	int addressMaxLenth = 8;
	int dataMaxLenth = 8;
	int counter = 0;
	
	char textFileAddress[addressMaxLenth+1];
	char textFiledata[dataMaxLenth+1];
	int c;
	int addressLength = 0;
	int dataLength = 0;

	textFileAddress[addressMaxLenth] = 0;
	textFiledata[dataMaxLenth] = 0;

	while( ((c = fgetc(memoryFile)) != EOF ) && counter < limit){
		if (c != '\n'){
			if (addressLength < addressMaxLenth){
				if(c == '1' || c == '0'){
					textFileAddress[addressLength++] = c;
				}
			} else {
				if (dataLength < dataMaxLenth){
					if(c == '1' || c == '0'){
						textFiledata[dataLength++] = c;
					}
				}
			}
		} else {
			addressToCompare  = binStr2num(textFileAddress);
			dataToCompare = binStr2num(textFiledata);
			if (readByteFromAddress(addressToCompare) != dataToCompare){
				printf("Byte at Address 0x%02x does not match. Rom: %i File: %i\n", \
					addressToCompare,readByteFromAddress(addressToCompare),dataToCompare);
				err = 1;
			}
			addressLength = 0;
			dataLength = 0;
			counter++;
		}
	}
	return err;
}

/* Open and write a binary file to Memory */
int writeBinaryFileToEEPROM(FILE *memoryFile,int validate,long begin, unsigned long limit){
	int c,addressToWrite = 0,err=0;
	int byteWriteCounter = 0;
	int counter = 0;

	while (addressToWrite < begin && (fgetc(memoryFile) != EOF)){
		addressToWrite++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToWrite < limit) {
		err = writeByteToAddress(addressToWrite++,c,validate,&byteWriteCounter);
	}
	ulog(INFO,"Wrote %i bytes",byteWriteCounter);
	return err;
}

/* Compare a binary file to Rom */
int compareBinaryFileToEEPROM(FILE *memoryFile, long begin, unsigned long limit){
	int c,addressToCompare = 0, err = 0;

	while (addressToCompare < begin && (fgetc(memoryFile) != EOF)){
		addressToCompare++;
	}

	while(((c = fgetc(memoryFile)) != EOF) && addressToCompare < limit) {
		if (readByteFromAddress(addressToCompare) != (char)c){
			printf("Byte at Address 0x%02x does not match. Rom: %i File: %i\n", \
				addressToCompare,readByteFromAddress(addressToCompare),c);
			err = 1;
		}
		addressToCompare++;
	}
	return err;
}

/* Read byte from specified Address */
char readByteFromAddress(unsigned short addressToRead){
	char binAdrStr[NUM_DATA_PINS+1];
	binAdrStr[NUM_DATA_PINS] = 0;
	// set the address
	setAddressPins(addressToRead);
	// enable output from the chip
	digitalWrite(OE,LOW);
	// set the rpi to input on it's gpio data lines
	for(int i=0;i<NUM_DATA_PINS;i++){
		pinMode(data[i],INPUT);
	}
	// read the pins and store to string
	for(int i=0,j=NUM_DATA_PINS-1;i<NUM_DATA_PINS;i++,j--){
		binAdrStr[i] = (char)(digitalRead(data[j])+0x30);
	}

	// convert the string to a number and return the number
	return binStr2num(binAdrStr);
}

/* Write specified byte to specified address */
int writeByteToAddress(unsigned short addressToWrite, char dataToWrite,char verify,int* byteWriteCounter){
	char binAdrStr[NUM_DATA_PINS+1];
	binAdrStr[NUM_DATA_PINS] = 0;
	int err = 0;

	// set the address
	setAddressPins(addressToWrite);
	// disable output from the chip
	digitalWrite(OE,HIGH);
	// set the rpi to output on it's gpio data lines
	for(int i=0;i<NUM_DATA_PINS;i++){
		pinMode(data[i],OUTPUT);
	}
	// Set the data pins to the data to be written
	setDataPins(dataToWrite);

	// perform the write
	digitalWrite(WE,HIGH);
	usleep(200);
	digitalWrite(WE,LOW);
	usleep(200);
	if (verify == 1){
		// printf("Verifying Byte %i at Address %i\n",dataToWrite,addressToWrite);
		if ( dataToWrite != readByteFromAddress(addressToWrite)){
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
void setAddressPins(unsigned short addressToSet){
	char binStr[NUM_ADDRESS_PINS];
	num2binStr(binStr,addressToSet,sizeof(binStr)/sizeof(binStr[0]));
	char pin = NUM_ADDRESS_PINS-1;
	for (char c = 0;c<NUM_ADDRESS_PINS;c++){
		digitalWrite(address[pin],binStr[c]-0x30);
		pin--;
	}
}

/* Set Data pins to value to write */
void setDataPins(char dataToSet){
	char binStr[NUM_DATA_PINS];
	num2binStr(binStr,dataToSet,sizeof(binStr)/sizeof(binStr[0]));

	char pin = NUM_DATA_PINS-1;
	for (char c = 0;c<NUM_DATA_PINS;c++){
		digitalWrite(data[pin],binStr[c]-0x30);
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
int binStr2num(char *binStr){
	int num = 0, num_size = 0;
	for (int i=strlen(binStr)-1,j=0;i>=0;i--){
		if (binStr[i] == '1' || binStr[i] == '0'){
			if (num_size < strlen(binStr) && num_size < 32){
				num += (binStr[i]-48)*(1 << j++);
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
int init(void){
	address[0] = A0;
	address[1] = A1;
	address[2] = A2;
	address[3] = A3;
	address[4] = A4;
	address[5] = A5;
	address[6] = A6;
	address[7] = A7;

	address[8] = A8;
	address[9] = A9;
	address[10] = A10;

	address[11] = A11;
	address[12] = A12;

	address[13] = A13;
	address[14] = A14;

	data[0] = D0;
	data[1] = D1;
	data[2] = D2;
	data[3] = D3;
	data[4] = D4;
	data[5] = D5;
	data[6] = D6;
	data[7] = D7;

	for(int i=0;i<NUM_ADDRESS_PINS;i++){
		pinMode(address[i], OUTPUT);
		digitalWrite(address[i], LOW);
	}

	for(int i=0;i<NUM_DATA_PINS;i++){
		pinMode(data[i], OUTPUT);
		digitalWrite(data[i], LOW);
	}

	pinMode(CE, OUTPUT);
	pinMode(OE, OUTPUT);
	pinMode(WE, OUTPUT);
	digitalWrite(CE, LOW);
	digitalWrite(OE, HIGH);
	digitalWrite(WE, LOW);

	return 0;
}

/* Prints help message */
void printHelp(){
	printf("Usage: piepro [options] [file]\n");
	printf("Options:\n");
	printf(" -b,   --binary			Interpret file as a binary.\n");
	printf(" -c,   --compare		Compare file and EEPROM and print differences.\n");
	printf(" -d N,   --dump N		Dump the contents of the EEPROM, 0=DEFAULT, 1=BINARY, 2=TEXT, 3=PRETTY.\n");
	printf(" -h,   --help			Print this message and exit.\n");
	printf(" -l N, --limit N		Specify the maximum address to operate.\n");
	printf("       --no-validate-write	Do not perform a read directly after writing to verify the data was written.\n");
	printf(" -s N, --start N		Specify the minimum address to operate.\n");
	printf(" -t,   --text			Interpret file as text.\n");
	printf("					Text File format:\n");
	printf("					00000000 00000000\n");
	printf(" -v N, --v[vvvv]		Set the log verbosity to N, 0=OFF, 1=FATAL, 2=ERROR, 3=WARNING, 4=INFO, 5=DEBUG.\n");
	printf("\n");
}

/* Original way to write the ROM */
void backupWriter(char *filename){
	// open file to write
    FILE *memoryFile = fopen(filename, "r");
    char line[32];
	int counter = 0;
	
	while((fgets(line, sizeof line, memoryFile)) != NULL) {
		// write Data
		char c = 0;
		char pin = 7;
		while (c<8){
			digitalWrite(address[pin],line[c]-0x30);
			// printf("Address Pin %d: %d\n",address[pin],line[c]-0x30);
			c++;
			pin--;
		}
		pin = 7;
		c++;
		while (c<17){
			digitalWrite(data[pin],line[c]-0x30);
			// printf("Data Pin %d: %d\n",data[pin],line[c]-0x30);
			c++;
			pin--;
		}

		digitalWrite(WE,HIGH);
		usleep(500);
		digitalWrite(WE,LOW);
		counter++;
	}
	ulog(INFO,"Wrote %d bytes",counter);
	
	fclose(memoryFile);
}

/* Prints the Rom's Contents to the specified limit */
void printROMContents(long begin,long limit,int format){
	if (limit == -1){
		limit = 256;
	}

	switch (format) {
	case 0:
		for (int i=begin;i<limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(i));
		}
		break;
	case 1:	// binary
		loggingLevel=OFF;
		for (int i=0;i<begin;i++) {
			putc(0xFF,stdout);
		}
		for (int i=begin;i<limit;i++) {
			putc(readByteFromAddress(i),stdout);
		}
		break;
	case 2: // text
		for (int i=begin;i<limit;i++)
		{
			char addressBinStr[9];
			int  addressBinStrLen = sizeof(addressBinStr)/sizeof(addressBinStr[0])-1;
			char dataBinStr[9];
			int  dataBinStrLen = sizeof(dataBinStr)/sizeof(dataBinStr[0])-1;

			num2binStr(addressBinStr,i,addressBinStrLen);
			num2binStr(dataBinStr,readByteFromAddress(i),dataBinStrLen);
			printf("%s %s \n",addressBinStr,dataBinStr);
		}
		break;
	case 3: // pretty
		if ((begin % 16) != 0){
			begin = begin - ((begin % 16));
		}
		printf("       00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F\n");
		printf("       ===============================================================\n");
		for (int i=begin;i<limit;i++){
			printf("%04x | %x  %x  %x  %x  %x  %x  %x  %x  %x  %x  %x  %x  %x  %x  %x  %x\n", \
			i, \
			readByteFromAddress(i), readByteFromAddress(i+1), readByteFromAddress(i+2), \
			readByteFromAddress(i+3), readByteFromAddress(i+4), readByteFromAddress(i+5), \
			readByteFromAddress(i+6), readByteFromAddress(i+7), readByteFromAddress(i+8), \
			readByteFromAddress(i+9), readByteFromAddress(i+10), readByteFromAddress(i+11), \
			readByteFromAddress(i+12), readByteFromAddress(i+13), readByteFromAddress(i+14), \
			readByteFromAddress(i+15));
			i = i+15;
		}
		break;
	default:
		for (int i=begin;i<limit;i++){
			printf("Address: %i     Data: %i \n",i,readByteFromAddress(i));
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