/*************************************************************************************************
 * USCIcmdInterpreter.h
 * - C interface file for MSP430 serial command interpreter
 *
 *  Author: Jamie Boyd
 *  Created on: 2022/01/13
 *  Modified: 2022/01/15 by Jamie Boyd
 **************************************************************************************************/
#ifndef USCICMDINTERPRETER_H_
#define USCICMDINTERPRETER_H_

#include <msp430.h>
#include <stdio.h>// used ONLY for sprintf function
#include "USCIA1UART.h"

//#define ASYNCH
#undef      ASYNCH                    // if defined, use the interrupts, else process commands as they are received

#define     NULL            0
#define     MAX_CMDS        3       // many more to come, I'm sure
#define     CMD_LEN         16      // long enough for longest command with spaces and arguments
#define     MAX_ARGS        3       // pDir and pOut have 3 arguments, that's max so far

#define     CMD0            "pDir"  // set up bits of a port for input or output
#define     CMD0_NARGS      3       // pNum (port number), mask (mask for setting bits), dir (0 for input, 1 for output)
#define     CMD1            "pOut"  // sets or clears bits in a port
#define     CMD1_NARGS      3       // pNum (port number), mask (for affecting bits), state (0=clear, 1=set, 2=toggle)
#define     CMD2            "p3Out" // writes a byte to P3out, assumes port is set up as output. Cause it is the contiguous port
#define     CMD2_NARGS      1       // byte to write to the port

typedef struct CMD {               // defines a command
    const char *name;             // pointer will point to string literals #defined above
    int nArgs;          // number of input arguments for a command
    int args[MAX_ARGS]; // all interpreted as unsigned though
}CMD;


extern volatile unsigned char gError;
extern volatile unsigned char gCmdCnt; //
extern const char gErrStrs [8][25];
extern char msgStr [50];

// needed string functions
char * strTok (char * stringWithSeps, char** contextP);
char strCmp (char * str1, char * str2);
int parseArg (char * aToken, char * err);

void initCmds (CMD * cmdList);
int parseCmd(CMD * cmdList, char * cmdLine);
int validateCmd(CMD * cmdList, char * cmdName);
int executeCmd(CMD * cmdList, char cmdIndex);
void printErr (void);



#ifdef ASYNCH

typedef struct LINE{
    unsigned char lineNum;    // line numbers as entered, for tracking which lines generated errors
    char line[CMD_LEN];
}LINE;

typedef struct ERR{         // error printing needs to know which line generated which error
    unsigned char cmdNum;
    unsigned char errCode;
}ERR;

#define     LINE_BUF_SZ      6       // size of a circular buffer of command lines to process
#define     ERR_BUF_SZ      6       // size of circular buffer of error strings to process

#endif

#endif /* USCICMDINTERPRETER_H_ */
