#include <string.h>

#include "utils.h"
#include "ulog.h"

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
