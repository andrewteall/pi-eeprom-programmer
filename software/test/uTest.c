#include <stdio.h>
#include <time.h>

#include "uTest.h"

#ifdef WITH_COLOR
    #define RED     "\x1b[31m"
    #define GREEN   "\x1b[32m"
    #define RESET   "\x1b[0m"
#else
    #define RED     ""
    #define GREEN   ""
    #define RESET   ""
#endif

enum TEST_STATUS {FAIL=0, PASS};

struct timespec start;
struct timespec stop;

int currentTestNumber = 0;
int testFailCounter = 0;
struct TEST_CONTAINER* testListPtr;
struct SUITE_CONTAINER* suiteListPtr;

int functionalSuiteCounter = 0;
int functionalTestCounter = 0;
struct TEST_CONTAINER functionalTestList[1000];
struct SUITE_CONTAINER functionalSuiteList[100];

int unitSuiteCounter = 0;
int unitTestCounter = 0;
struct TEST_CONTAINER unitTestList[1000];
struct SUITE_CONTAINER unitSuiteList[100];

void (*funcTestInit)(void) = NULL;
void (*funcSuiteInit)(void) = NULL;
void (*unitTestInit)(void) = NULL;
void (*unitSuiteInit)(void) = NULL;

void expect(int expectation, int actual){
    testListPtr[currentTestNumber].expected = expectation;
    testListPtr[currentTestNumber].actual = actual;
    testListPtr[currentTestNumber].status = (expectation == actual);
    testListPtr[currentTestNumber].inverted = 0;
}

void expectNot(int expectationNot, int actual){
    testListPtr[currentTestNumber].expected = expectationNot;
    testListPtr[currentTestNumber].actual = actual;
    testListPtr[currentTestNumber].status = (expectationNot != actual);
    testListPtr[currentTestNumber].inverted = 1;
}

int getCurrentFuncSuite(){
    return functionalSuiteCounter-1;
}

int getCurrentUnitSuite(){
    return unitSuiteCounter-1;
}

int addFuncSuite(char* suiteDescription){
    functionalSuiteList[functionalSuiteCounter].suiteNumber = functionalSuiteCounter;
    functionalSuiteList[functionalSuiteCounter].description = suiteDescription;
    functionalSuiteCounter++;
    return 0;
}

int addUnitSuite(char* suiteDescription){
    unitSuiteList[unitSuiteCounter].suiteNumber = unitSuiteCounter;
    unitSuiteList[unitSuiteCounter].description = suiteDescription;
    unitSuiteCounter++;
    return 0;
}

int addFuncTest(char* description, int suiteNumber, void (*testPtr)()){
    functionalTestList[functionalTestCounter].description = description;
    functionalTestList[functionalTestCounter].test = testPtr;
    functionalTestList[functionalTestCounter].testNumber = functionalTestCounter;
    functionalTestList[functionalTestCounter].suiteNumber = suiteNumber;
    functionalTestCounter++;
    return 0;
}

int addUnitTest(char* description, int suiteNumber, void (*testPtr)()){
    unitTestList[unitTestCounter].description = description;
    unitTestList[unitTestCounter].test = testPtr;
    unitTestList[unitTestCounter].testNumber = unitTestCounter;
    unitTestList[unitTestCounter].suiteNumber = suiteNumber;
    unitTestCounter++;
    return 0;
}

int addFuncSuiteInit(void (*funcSuiteInitToAdd)(void)){
    funcSuiteInit = funcSuiteInitToAdd;
    return 0;
}

int addFuncTestInit(void (*funcTestInitToAdd)(void)){
    funcTestInit = funcTestInitToAdd;
    return 0;
}

int addunitSuiteInit(void (*unitSuiteInitToAdd)(void)){
    unitSuiteInit = unitSuiteInitToAdd;
    return 0;
}

int addUnitTestInit(void (*unitTestInitToAdd)(void)){
    unitTestInit = unitTestInitToAdd;
    return 0;
}

void printFuncTestStatus(int testNumber){
    if(functionalTestList[testNumber].status == PASS){
        fprintf(stdout, GREEN "passed in %u.%03us" RESET "\n",functionalTestList[testNumber].timeToRunSec,functionalTestList[testNumber].timeToRunMSec);
    } else if(functionalTestList[currentTestNumber].inverted){
        fprintf(stdout, RED "failed in %u.%03us  Not Expected: %i" RESET "\n", \
                        functionalTestList[testNumber].timeToRunSec,functionalTestList[testNumber].timeToRunMSec, \
                        functionalTestList[testNumber].expected);
        testFailCounter++;
    } else {
        fprintf(stdout, RED "failed in %u.%03us Expected: %i   Actual: %i " RESET "\n", \
                        functionalTestList[testNumber].timeToRunSec,functionalTestList[testNumber].timeToRunMSec, \
                        functionalTestList[testNumber].expected, functionalTestList[testNumber].actual);
        testFailCounter++;
    }
}

void printUnitTestStatus(int testNumber){
    if(unitTestList[testNumber].status == PASS){
        fprintf(stdout, GREEN "passed in %u.%03us" RESET "\n",unitTestList[testNumber].timeToRunSec,unitTestList[testNumber].timeToRunMSec);
    } else if(unitTestList[currentTestNumber].inverted){
        fprintf(stdout, RED "failed in %u.%03us  Not Expected: %i" RESET "\n", \
                        unitTestList[testNumber].timeToRunSec,unitTestList[testNumber].timeToRunMSec, \
                        unitTestList[testNumber].expected);
        testFailCounter++;
    } else {
        fprintf(stdout, RED "failed in %u.%03us  Expected: %i   Actual: %i " RESET "\n", \
                        unitTestList[testNumber].timeToRunSec,unitTestList[testNumber].timeToRunMSec, \
                        unitTestList[testNumber].expected, unitTestList[testNumber].actual);
        testFailCounter++;
    }
}

void runFuncTests(int suiteNum, int testNum){
    setbuf(stdout, NULL); // disable buffering
    testListPtr = functionalTestList;
    fprintf(stdout, "\nStarting Functional Tests:\n");
    fprintf(stdout, "--------------------------\n\n");

    for(int j = 0; j < functionalSuiteCounter; j++){
        if(j == suiteNum || suiteNum == -1 || testNum != -1){
            if(testNum == -1){
                if(funcSuiteInit != NULL){
                    funcSuiteInit();
                }
            fprintf(stdout, "Suite Number: %04i - %s\n",functionalSuiteList[j].suiteNumber,functionalSuiteList[j].description);
            fprintf(stdout, "------------------------------------------------------\n");
            }
            for(int i = 0; i < functionalTestCounter; i++){
                if(functionalTestList[i].suiteNumber == j && (i == testNum || testNum == -1)){
                    if(funcTestInit != NULL){
                        funcTestInit();
                    }
                    currentTestNumber = i;
                    fprintf(stdout, "Test %04i - %-100s: ",functionalTestList[i].testNumber,functionalTestList[i].description);

                    clock_gettime(CLOCK_REALTIME, &start);
                    (*functionalTestList[i].test)();
                    clock_gettime(CLOCK_REALTIME, &stop);
                    functionalTestList[i].timeToRunSec = stop.tv_sec - start.tv_sec;
                    if(stop.tv_nsec > start.tv_nsec){
                        functionalTestList[i].timeToRunMSec = (int)(stop.tv_nsec - start.tv_nsec)/1000000;
                    } else {
                        functionalTestList[i].timeToRunMSec = (int)((stop.tv_nsec+1000000000) - start.tv_nsec)/1000000;
                        --functionalTestList[i].timeToRunSec;
                    }
                    
                    printFuncTestStatus(i);
                }
            }
            if(testNum == -1){
                fprintf(stdout, "\n");
            }
        }
    }
}

void runUnitTests(int suiteNum, int testNum){
    setbuf(stdout, NULL); // disable buffering
    testListPtr = unitTestList;
    fprintf(stdout, "\nStarting Unit Tests:\n");
    fprintf(stdout, "--------------------------\n\n");

    for(int j = 0; j < unitSuiteCounter; j++){
        if(j == suiteNum || suiteNum == -1 || testNum != -1){
            if(testNum == -1){
                if(unitSuiteInit != NULL){
                    unitSuiteInit();
                }
                fprintf(stdout, "Suite Number: %04i - %s\n",unitSuiteList[j].suiteNumber,unitSuiteList[j].description);
                fprintf(stdout, "------------------------------------------------------\n");
            }
            for(int i = 0; i < unitTestCounter; i++){
                if(unitTestList[i].suiteNumber == j && (i == testNum || testNum == -1)){
                    if(unitTestInit != NULL){
                        unitTestInit();
                    }
                    currentTestNumber = i;
                    fprintf(stdout, "Test %04i - %-100s: ",unitTestList[i].testNumber,unitTestList[i].description);
                    
                    clock_gettime(CLOCK_REALTIME, &start);
                    (*unitTestList[i].test)();
                    clock_gettime(CLOCK_REALTIME, &stop);
                    unitTestList[i].timeToRunSec = stop.tv_sec - start.tv_sec;
                    if(stop.tv_nsec > start.tv_nsec){
                        unitTestList[i].timeToRunMSec = (int)(stop.tv_nsec - start.tv_nsec)/1000000;
                    } else {
                        unitTestList[i].timeToRunMSec = (int)((stop.tv_nsec+1000000000) - start.tv_nsec)/1000000;
                        --unitTestList[i].timeToRunSec;
                    }
                    
                    printUnitTestStatus(i);
                }
            }
            if(testNum == -1){
                fprintf(stdout, "\n");
            }        
        }
    }
}

void printResults(){
    if(testFailCounter == 0){
        printf("\nAll Tests Passed\n\n");
    } else {
        printf("\n%i Tests Failed\n\n", testFailCounter);
    }
}

int getTestFailCounter(){
    return testFailCounter;
}
