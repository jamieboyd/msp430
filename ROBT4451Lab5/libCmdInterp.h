/*************************************************************************************************
 * USCIcmdInterpreter.h
 * - C interface file for MSP430 serial command interpreter
 *
 *  Author: Jamie Boyd
 *  Created on: 2022/01/13
 *  Modified: 2022/01/15 by Jamie Boyd
 **************************************************************************************************/
#ifndef LIBCMDINTERP_H_
#define LIBCMDINTERP_H_

#include <msp430.h>
#include <stdio.h>// used ONLY for sprintf function
#include <libUART1A.h>  // library to configure and use serial port

#ifndef     NULL
#define     NULL            0
#endif
#define     MAX_CMDS        8       // many more to come, I'm sure
#define     CMD_LEN         16      // long enough for longest command with spaces and arguments
#define     MAX_ARGS        4       // nokLine has 4

#define     CMD0            "pDir\0"  // set up bits of a port for input or output
#define     CMD0_NARGS      3       // pNum (port number), mask (mask for setting bits), dir (0 for input, 1 for output)
#define     CMD1            "pOut\0"  // sets or clears bits in a port
#define     CMD1_NARGS      3       // pNum (port number), mask (for affecting bits), state (0=clear, 1=set, 2=toggle)
#define     CMD2            "p3Out\0" // writes a byte to P3out, assumes port is set up as output. Cause it is the contiguous port
#define     CMD2_NARGS      1       // byte to write to the port

#define     CMD3            "nokClear\0"  // Clear Nokia screen
#define     CMD3_NARGS      0           // just clear it
#define     CMD4            "nokSetPix\0" // set a single pixel
#define     CMD4_NARGS      2           //(unsigned char xPos, unsigned char yPos)
#define     CMD5            "nokScrnLine\0"
#define     CMD5_NARGS      2           // (unsigned char linePos, unsigned char isVnotH)
#define     CMD6            "nokLine\0"   // draw a line from anywhere
#define     CMD6_NARGS      4           //  (unsigned char xStart, unsigned char yStart, unsigned char xEnd, unsigned char yEnd)
#define     CMD7            "nokClearPix\0"
#define     CMD7_NARGS      2


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
signed int parseArg (char * aToken, char * err);

void initCmds (CMD * cmdList);
signed char parseCmd(CMD * cmdList, char * cmdLine);
signed char validateCmd(CMD * cmdList, char * cmdName);
signed char executeCmd(CMD * cmdList, char cmdIndex);
void printErr (void);


#endif /* LIBCMDINTERP_H_ */
