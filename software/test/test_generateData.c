#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/utils.h"

/* Generate a Random Number */
int getRandomData(int limit){
    return (rand() % (limit + 1));
}

/* Get the next set of Data from a text file formatted rom */
int getAddressOrDataFromTextFile(FILE *inputFile){
	int c;
	int strLen = 0;
	char textFileString[(sizeof(int)*8)+1];
	
	while((c = fgetc(inputFile)) != EOF && strLen != sizeof(int)*8 && c != '\n' && c != ' ' && c != ':'){
		// Will skip characters that are not '1' or '0'
		if(c == '1' || c == '0'){
			textFileString[strLen++] = c;
		}
	}
	
	textFileString[strLen++] = 0;
	if(c == EOF){
		return -1;
	} else {
		return binStr2num(textFileString);
	}
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

int generateBinaryFile(char* filename, int size){
    FILE* outputFile = fopen(filename,"w");
    if(outputFile == NULL){
        fprintf(stderr,"Error Opening File\n");
        return 1;
    }

    printf("Using output file name: %s\n", filename);
    for(int i=0; i < size; i++){
        fputc(getRandomData(256),outputFile);
    }

    fclose(outputFile);

    return 0;
}

int generateTextFile(char* filename, int size){
    FILE* outputFile = fopen(filename,"w");
    if(outputFile == NULL){
            fprintf(stderr,"Error Opening File\n");
            return 1;
        }

    printf("Using output file name: %s\n", filename);
    char addressString[17];
    char dataString[9];
    for(int i=0; i < size; i++){
        num2binStr(i,addressString,17);
        fputs(addressString,outputFile);
        fputc(' ',outputFile);
        num2binStr((char)getRandomData(256),dataString,9);
        fputs(dataString,outputFile);
        fputc('\n',outputFile);
    }

    fclose(outputFile);

    return 0;
}

int convertBinaryFileToTextFile(char* inputFilename){
    int inputFilenameLen = strlen(inputFilename);
    char outputFilename[inputFilenameLen+15];

    int found = 0;
    int extension = inputFilenameLen;
    while(!found && extension > 0){
        if(inputFilename[extension] == '.'){
            found = 1;
        } else {
            --extension;
        }
    }

    if(extension != 0){
        strncpy(outputFilename,inputFilename,extension+1);
        outputFilename[extension] = '\0';

        // Decide filename
        strcat(outputFilename,".txt\0");

        printf("Using output file name: %s\n", outputFilename);
        
        
        FILE* inputFile = fopen(inputFilename,"r");
        if(inputFile == NULL){
            fprintf(stderr,"Error Opening File\n");
            return 1;
        }
        FILE* outputFile = fopen(outputFilename,"w");
        if(outputFile == NULL){
            fprintf(stderr,"Error Opening File\n");
            return 1;
        }

        int dataByte;
        int address = 0;
        char addressString[17];
        char dataString[9];
        while((dataByte = fgetc(inputFile)) != EOF){
            num2binStr(address,addressString,17);
            fputs(addressString,outputFile);
            fputc(' ',outputFile);
            num2binStr((char)dataByte,dataString,9);
            fputs(dataString,outputFile);
            fputc('\n',outputFile);

            address++;
        }

        fclose(inputFile);
        fclose(outputFile);
    } else {
        return 1;
    }
    return 0;
}

int convertBinaryFileToUnmatchedBinaryFile(char* inputFilename){
    int inputFilenameLen = strlen(inputFilename);
    char outputFilename[inputFilenameLen+15];

    int found = 0;
    int extension = inputFilenameLen;
    while(!found && extension > 0){
        if(inputFilename[extension] == '.'){
            found = 1;
        } else {
            --extension;
        }
    }

    if(extension != 0){
        strncpy(outputFilename,inputFilename,extension+1);
        outputFilename[extension] = '\0';

        // Decide filename
        strcat(outputFilename,"-unmatched.bin\0");

        printf("Using output file name: %s\n", outputFilename);
        
        
        FILE* inputFile = fopen(inputFilename,"r");
        if(inputFile == NULL){
            fprintf(stderr,"Error Opening File\n");
            return 1;
        }
        FILE* outputFile = fopen(outputFilename,"w");
        if(outputFile == NULL){
            fprintf(stderr,"Error Opening File\n");
            return 1;
        }

        int dataByte;
        while((dataByte = fgetc(inputFile)) != EOF){
            fputc(((char)dataByte)+7,outputFile);
        }

        fclose(inputFile);
        fclose(outputFile);
    } else {
        return 1;
    }
    return 0;
}

int convertTextFileToUnmatchedTextFile(char* inputFilename){
    int inputFilenameLen = strlen(inputFilename);
    char outputFilename[inputFilenameLen+15];

    int found = 0;
    int extension = inputFilenameLen;
    while(!found && extension > 0){
        if(inputFilename[extension] == '.'){
            found = 1;
        } else {
            --extension;
        }
    }

    if(extension != 0){
        strncpy(outputFilename,inputFilename,extension+1);
        outputFilename[extension] = '\0';

        // Decide filename
        strcat(outputFilename,"-unmatched.txt\0");

        printf("Using output file name: %s\n", outputFilename);
        
        FILE* inputFile = fopen(inputFilename,"r");
        if(inputFile == NULL){
            fprintf(stderr,"Error Opening File\n");
            return 1;
        }
        FILE* outputFile = fopen(outputFilename,"w");
        if(outputFile == NULL){
            fprintf(stderr,"Error Opening File\n");
            return 1;
        }

        int address = 0;
        int dataByte;
        char addressString[17];
        char dataString[9];
        while(address != -1 && dataByte != -1){
            address = getAddressOrDataFromTextFile(inputFile);
            dataByte = getAddressOrDataFromTextFile(inputFile);
            if( address != -1 && dataByte != -1){
                num2binStr(address,addressString,17);
                fputs(addressString,outputFile);
                fputc(' ',outputFile);

                
                num2binStr((char)dataByte+7,dataString,9);
                fputs(dataString,outputFile);
                fputc('\n',outputFile);
            }
        }

        fclose(inputFile);
        fclose(outputFile);
    } else {
        return 1;
    }
    return 0;
}

int generateTestData(char* inputFilename, int size){
    printf("Generating Test Data...\n");
    int error = 0;
    error |= generateBinaryFile(inputFilename, size);
    error |= convertBinaryFileToTextFile(inputFilename);
    error |= convertBinaryFileToUnmatchedBinaryFile(inputFilename);

    int inputFilenameLen = strlen(inputFilename);
    char outputFilename[inputFilenameLen+15];

    int found = 0;
    int extension = inputFilenameLen;
    while(!found && extension > 0){
        if(inputFilename[extension] == '.'){
            found = 1;
        } else {
            --extension;
        }
    }

    if(extension != 0){
        strncpy(outputFilename,inputFilename,extension+1);
        outputFilename[extension] = '\0';

        strcat(outputFilename,".txt\0");
        error |= convertTextFileToUnmatchedTextFile(outputFilename);
    } else {
        return 1;
    }

    {
    int inputFilenameLen = strlen(inputFilename);
    char outputFilename[inputFilenameLen+15];

    int found = 0;
    int extension = inputFilenameLen;
    while(!found && extension > 0){
        if(inputFilename[extension] == '.'){
            found = 1;
        } else {
            --extension;
        }
    }

    if(extension != 0){
        strncpy(outputFilename,inputFilename,extension+1);
        outputFilename[extension] = '\0';

        strcat(outputFilename,"-oversized.bin\0");
        error |= generateBinaryFile(outputFilename,size*2);
    } else {
        return 1;
    }
    }

    {
    int inputFilenameLen = strlen(inputFilename);
    char outputFilename[inputFilenameLen+15];

    int found = 0;
    int extension = inputFilenameLen;
    while(!found && extension > 0){
        if(inputFilename[extension] == '.'){
            found = 1;
        } else {
            --extension;
        }
    }

    if(extension != 0){
        strncpy(outputFilename,inputFilename,extension+1);
        outputFilename[extension] = '\0';

        strcat(outputFilename,"-oversized.txt\0");
        error |= generateTextFile(outputFilename,size*2);
    } else {
        return 1;
    }
    }

    return error;
}
