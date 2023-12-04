#ifndef TEST_EXPECT_H
#define TEST_EXPECT_H 1

/**
 * @brief Defines a Test.
 */
struct TEST_CONTAINER{
    int testNumber;
    char* description;
    int suiteNumber;
    void (*test)(void);
    
    int expected;
    int actual;
    int status;
    int inverted;
    unsigned int timeToRunSec;
    unsigned int timeToRunMSec;
};

/**
 * @brief Defines a Suite.
 */
struct SUITE_CONTAINER{
    int suiteNumber;
    char* description;
};

/**
 * @brief Compare expectation to actual and set the test status based on the comparison.
 * @param expectation The expected value.
 * @param actual The actual value.
 */
void expect(int expectation, int actual);

/**
 * @brief Compare expectationNot to actual and set the test status based on the comparison.
 * @param expectation The value not expected.
 * @param actual The actual value.
 */
void expectNot(int expectationNot, int actual);

/**
 * @brief Returns the current Functional Suite.
 * @return int The current Functional Suite.
 */
int getCurrentFuncSuite();

/**
 * @brief Returns the current Unit Suite.
 * @return int The current Unit Suite.
 */
int getCurrentUnitSuite();

/**
 * @brief Adds a new Functional Suite.
 * @param suiteDescription A point to the Suite Description.
 * @return int Just returns 0.
 */
int addFuncSuite(char* suiteDescription);

/**
 * @brief Adds a new Unit Suite.
 * @param suiteDescription A point to the Suite Description.
 * @return int Just returns 0.
 */
int addUnitSuite(char* suiteDescription);

/**
 * @brief Adds a new Functional Test.
 * @param description A pointer to the Test Description.
 * @param suiteNumber The Suite to add the test to.
 * @param testPtr Pointer to the test to add.
 * @return int Just returns 0.
 */
int addFuncTest(char* description, int suiteNumber, void (*testPtr)());

/**
 * @brief Adds a new Unit Test.
 * @param description A pointer to the Test Description.
 * @param suiteNumber The Suite to add the test to.
 * @param testPtr Pointer to the test to add.
 * @return int Just returns 0.
 */
int addUnitTest(char* description, int suiteNumber, void (*testPtr)());

/**
 * @brief Adds a initializer function to be run before every Functional Suite.
 * @param funcSuiteInitToAdd Pointer to the function to run before every Functional Suite.
 * @return int Just returns 0.
 */
int addFuncSuiteInit(void (*funcSuiteInitToAdd)(void));

/**
 * @brief Adds a initializer function to be run before every Functional Test.
 * @param funcTestInitToAdd Pointer to the function to run before every Functional Test.
 * @return int Just returns 0.
 */
int addFuncTestInit(void (*funcTestInitToAdd)(void));

/**
 * @brief Adds a initializer function to be run before every Unit Suite.
 * @param uintSuiteInitToAdd Pointer to the function to run before every Unit Suite.
 * @return int Just returns 0.
 */
int addunitSuiteInit(void (*unitSuiteInitToAdd)(void));

/**
 * @brief Adds a initializer function to be run before every Unit Test.
 * @param funcTestInitToAdd Pointer to the function to run before every Unit Test.
 * @return int Just returns 0.
 */
int addUnitTestInit(void (*unitTestInitToAdd)(void));

/**
 * @brief Runs Functional tests.
 * @param suiteNum The Functional Suite to run. -1 for all.
 * @param testNum The Functional Test to run. -1 for all.
 */
void runFuncTests(int suiteNum, int testNum);

/**
 * @brief Runs Unit tests.
 * @param suiteNum The Unit Suite to run. -1 for all.
 * @param testNum The Unit Test to run. -1 for all.
 */
void runUnitTests(int suiteNum, int testNum);

/**
 * @brief Prints The number of tests failed.
 */
void printResults();

/**
 * @brief Returns the number of tests failed globally.
 * @return int The number of tests failed globally.
 */
int getTestFailCounter();
#endif