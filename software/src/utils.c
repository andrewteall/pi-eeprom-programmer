#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include "utils.h"
#include "piepro.h"

/* Strings that correlate to LOGLEVEL enum so that LOGLEVEL can be printed */
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};

/* Static global Logging Level to track verbosity across the program */
static int loggingLevel = WARNING;

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

/* Prints help message */
void printHelp(){
	printf("Usage: piepro [options] [file]\n");
	printf("Options:\n");
	printf(" -b,   --binary			Interpret file as a binary. Default: text\n");
	printf("					Text File format:\n");
	printf("					[00000000]00000000 00000000\n");
	printf(" -c,   	--compare		Compare file and EEPROM and print differences.\n");
	printf(" -d N, 	--dump N		Dump the contents of the EEPROM, 0=DEFAULT, 1=BINARY, 2=TEXT, 3=PRETTY.\n");
	printf(" -f,   	--force			Force writing of every byte instead of checking for existing value first.\n");
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

void setDefaultOptions(struct OPTIONS* sOptions){
    sOptions->limit = -1;
    sOptions->startValue = 0;
    sOptions->dumpFormat = 0;
    sOptions->validateWrite = 1;
    sOptions->force = 0;
    sOptions->action = WRITE_FILE_TO_ROM;
    sOptions->fileType = TEXT_FILE;
    sOptions->eepromModel = AT28C16;
    sOptions->writeCycleUSec = -1;
    sOptions->i2cId = 0x50;

    sOptions->addressParam = 0;
    sOptions->dataParam = 0;
}

int  parseCommandLineOptions(struct OPTIONS* sOptions,int argc, char* argv[]){
    setDefaultOptions(sOptions);

    sOptions->filename = argv[argc-1];
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
				if (setLoggingLevel(verbosity)){
					return 1;
				}
			}
		}

		for(int i=argc-1;i>0;i--){
			// -i --initial
			if (!strcmp(argv[i],"-s") || !strcmp(argv[i],"--start")){
				ulog(INFO,"Setting starting value to %i",str2num(argv[i+1]));
				sOptions->startValue = str2num(argv[i+1]);
				if ( sOptions->startValue == -1){
					ulog(FATAL,"Unsupported starting value");
					return 1;
				}
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

			// -b --binary 
			if (!strcmp(argv[i],"-b") || !strcmp(argv[i],"--binary")){
				ulog(INFO,"Setting filetype to binary");
				sOptions->fileType = BINARY_FILE;
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

			// -d --dump
			if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dump")){
				if (sOptions->action == COMPARE_ROM_TO_FILE || sOptions->action == WRITE_SINGLE_BYTE_TO_ROM){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Dumping EEPROM to standard out");
					sOptions->dumpFormat = str2num(argv[i+1]);
					sOptions->action = DUMP_ROM;
				}
			}

			// -w --write
			if (!strcmp(argv[i],"-w") || !strcmp(argv[i],"--write")){
				if (sOptions->action == COMPARE_ROM_TO_FILE || sOptions->action == DUMP_ROM){
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

			// -c --compare
			if (!strcmp(argv[i],"-c") || !strcmp(argv[i],"--compare")){
				if (sOptions->action == DUMP_ROM || sOptions->action == WRITE_SINGLE_BYTE_TO_ROM){
					ulog(WARNING, \
						"%s flag specified but another action has already be set. Ignoring %s flag.",argv[i],argv[i]);
				} else {
					ulog(INFO,"Comparing EEPROM to File");
					sOptions->action = COMPARE_ROM_TO_FILE;
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
	

}




/* Logging method to supprot filtering out logs by verbosity */
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

/* Sets the LoggingLevel to the specified newLogLevel. Fails and returns 1 if 
   an invalid newLogLevel is passed. Otherwise, returns 0. */
int setLoggingLevel(int newLogLevel){
	if( newLogLevel < DEBUG || newLogLevel > OFF){
		loggingLevel = newLogLevel;
		ulog(INFO,"Setting Logging Level to [%i] %s",newLogLevel,LOGLEVELSTRINGS[newLogLevel]);
		return 0;
	} else {
		ulog(ERROR,"Invalid Logging Level. Log Level could not be set.");
		return 1;
	}
}

/* Returns the currently set LoggingLevel */
int getLoggingLevel(){
	return loggingLevel;
}