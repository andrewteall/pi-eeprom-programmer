#include <stdarg.h>
#include <stdio.h>

#include "ulog.h"

/* Strings that correlate to LOGLEVEL enum so that LOGLEVEL can be printed */
const char *LOGLEVELSTRINGS[] = {"OFF","FATAL", "ERROR", "WARNING", "INFO", "DEBUG", "TRACE",};

/* Static global Logging Level to track verbosity across the program */
static int loggingLevel = WARNING;

/* Logging method to support filtering out logs by verbosity */
void ulog(int verbosity, const char* logMessage,...) {
	if (verbosity <= loggingLevel){
		char logBuf[120];
		va_list args;
		va_start(args, logMessage);
		vsnprintf(logBuf,sizeof(logBuf),logMessage, args);
		va_end(args);
    	fprintf(stderr,"%8s:  %s\n", LOGLEVELSTRINGS[verbosity],logBuf);
	}
}

/* Sets the LoggingLevel to the specified newLogLevel. Fails and returns -1 if 
   an invalid newLogLevel is passed. Otherwise, returns 0. */
int setLoggingLevel(int newLogLevel){
	if( newLogLevel <= TRACE && newLogLevel >= OFF){
		loggingLevel = newLogLevel;
		ulog(INFO,"Setting Logging Level to [%i] %s",newLogLevel,LOGLEVELSTRINGS[newLogLevel]);
		return 0;
	} else {
		ulog(ERROR,"Invalid Logging Level. Log Level could not be set.");
		return -1;
	}
}

/* Returns the currently set LoggingLevel */
int getLoggingLevel(){
	return loggingLevel;
}
