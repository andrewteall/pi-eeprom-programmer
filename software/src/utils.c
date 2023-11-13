#include <stdlib.h>

/* Converts a number to a binary String */
char* num2binStr(int num, char* binStrBuf, int strBufLen){
    for (int i=strBufLen-2; i >= 0; i--){
		binStrBuf[i] = (char)((num%2)+0x30);
		num >>= 1;
    }
	if(num != 0){
		return (char*) -1;
	}
    binStrBuf[strBufLen-1] = 0;
    return binStrBuf;
}

/* Converts a binary string to a number. */
int binStr2num(char *binStr){
	char* indexPtr;
	long num = strtol(binStr, &indexPtr, 2);
	if (*indexPtr != '\0' || num < 0){
		num = -1;
	}
	return (int)num;
}

/* Converts a number string to a number */
int str2num(char *numStr){
	char* indexPtr;
	int index = 0;
	int base = 10;

	if ((numStr[0] == '0' && (numStr[1] == 'x' || numStr[1] == 'X')) || numStr[0] == '$'){ 
		// convert hexidecimal number
		index = 2;
		if(numStr[0] == '$'){
			index = 1;
		}
		base = 16;
	} else if ((numStr[0] == '0' && (numStr[1] == 'b' || numStr[1] == 'B')) || numStr[0] == '%'){
		// convert binary number
		index = 2;
		if(numStr[0] == '%'){
			index = 1;
		}
		base = 2;
	}

	long num = strtol(numStr+index, &indexPtr, base);
	if (*indexPtr != '\0' || num < 0){
		num = -1;
	}
	return (int)num;
}
