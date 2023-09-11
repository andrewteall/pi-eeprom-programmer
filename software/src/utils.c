#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "utils.h"

/* Strings that correlate to LOGLEVEL enum so that LOGLEVEL can be printed */
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG",};

/* Static global Logging Level to track verbosity across the program */
static int loggingLevel = WARNING;

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

/* Converts a number to a binary String */
char* num2binStr(char* binStrBuf, int num, int strBufLen){
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
	int num = 0, numSize = 0;
	for (int i=strlen(binStr)-1,j=0;i>=0;i--,j++){
		if (binStr[i] == '1' || binStr[i] == '0'){
			if (numSize < strlen(binStr) && numSize < 32){
				num += (binStr[i]-48)*(1 << j);
				numSize++;
			} else {
				ulog(ERROR,"Number out of Range");
				num = -1;
				i = -1;
			}
		}
	}
	return num;
}

/* Converts a number string to a number */
int str2num(char *numStr){
	int num = 0;
	int numSize = 0;
	for (int i=strlen(numStr)-1,j=0;i>=0;i--){
		if ((numStr[0] == '0' && (numStr[1] == 'x' || numStr[1] == 'X')) || numStr[0] == '$'){ 
			// convert hexidecimal number
			int limit = 2;
			if(numStr[0] == '$'){
				limit = 1;
			}
			if(!(i < limit) ){
				if ((numStr[i] >= '0' && numStr[i] <= '9') || (numStr[i] >= 'A' && numStr[i] <= 'F') \
					|| (numStr[i] >= 'a' && numStr[i] <= 'f') ){
					if (j < 9){
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
		} else if ((numStr[0] == '0' && (numStr[1] == 'b' || numStr[1] == 'B')) || numStr[0] == '%'){
			// convert binary number
			int limit = 2;
			if(numStr[0] == '%'){
				limit = 1;
			}

			if(!(i < limit) ){
				if (numSize < strlen(numStr) && numSize < 32){
					num += (numStr[i]-48)*(1 << j++);
					numSize++;
				} else {
					ulog(ERROR,"Number out of Range");
					num = -1;
					i = -1;
				}
			}
		} else { 
			// convert decimal number
			if (numStr[i] >= '0' && numStr[i] <= '9'){
				if (j < 11){ // This could be better
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

/* @brief Rounds a float up to the nearest whole number */
int roundUp(float floatNumber){
	int intNumber = (int)floatNumber;
	if(intNumber < floatNumber){
		intNumber++;
	}
	return intNumber;
}