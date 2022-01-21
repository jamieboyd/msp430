/************************************************************************************************
 * CommandInterpreter.c
 *- top level file with main{} for Command Interpreter
 *-  *
 *  Author: Jamie Boyd
 *  Created on: 2022/01/15
 *  Modified:
 ************************************************************************************************/

#include <msp430.h>
#include "USCIcmdInterpreter.h"

/******************** global strings, structs, and variables, perhaps shared with interrupts ********************/
#ifndef ASYNCH
volatile unsigned char gCmdCnt;                // number of command being processed = number of command just entered
volatile unsigned char gError = 0;             // flag to be set to an error condition, there are 8, including 0-no error
char msgStr [50];
const char gErrStrs [8][25] = {
                                "success\0",
                                "too long\0",
                                "name does not exist\0",
                                "not enough arguments\0",
                                "too many arguments\0",
                                "argument not a number\0",
                                "Port num out of range\0",
                                "argument out of range\0"
                               };

// here, commands are entered and processed synchronously, i.e., no interrupts
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watch-dog timer
    usciA1UartInit(19200);                  // initialize UART for 19200 Baud communication
    CMD cmdList [MAX_CMDS];                 // array of commands that we know how to process
    int cmdIndex;                            // position of current command in command list, -1 for error
    gCmdCnt = 1;                            // humans like to start at 1 not 0
    initCmds (cmdList);                  // initialize command list from #defines

    char gCmdLineBuffer [CMD_LEN];                      // a simple garden-variety string
    while (1){                              // get command, process it, run it.
        sprintf ((char *)msgStr,"CMD %d:\0", gCmdCnt);  // display a prompt for user
        usciA1UartTxString (msgStr);        // prompt user for a command
        if (usciA1UartGets (gCmdLineBuffer) == NULL){    //user entered too many characters
            gError = 1;
            printErr ();
            continue;
        }
        cmdIndex = parseCmd(cmdList, gCmdLineBuffer);
        if (cmdIndex ==-1){
           printErr ();
           continue;
        }
        cmdIndex = executeCmd(cmdList, cmdIndex);
        printErr ();  // error from executeCmd, or no error
        if (cmdIndex != -1){
            gCmdCnt +=1;            // we only count commands that succeed in this version
        }
    }
    return 0;
}
#endif


/* Here was a start on asynchronous processing with interrupts. Not finished */
#ifdef ASYNCH
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;                   // Stop watch-dog timer
    usciA1UartInit(19200);                      // initialize UART for 19200 Baud communication
    usciA1UartInstallRxInt (&cmdRxIntFunc);     // install RX interrupt but DONT start it
    usciA1UartEnableRxInt (0);
    usciA1UartInstallTxInt (&msgTxIntFunc);     // install TX interrupt but DONT start it
    usciA1UartEnableTxInt (0);
    __enable_interrupt();                       // enable interrupts by setting global interrupt enable bit

    unsigned char gBufState = 0;                   // cmdBuffer 0 is empty, 1 is part filled, 2 is full
    // now main will process commands in an endless loop as Rx interrupt fills them
    while (1){
        if (gBufState > 0){
           // if parseCmd (cmdList, gCmdLineBuffer[gCmdBufOut]);

            gCmdBufOut +=1;
            if (gCmdBufOut == CMD_BUF_SZ){
                gCmdBufOut = 0;
            }
            if (gCmdBufOut == gCmdBufIn){  // buffer is empty
                gBufState =0
            }
        }
    }
    return 0;
}


#ifdef ASYNCH
LINE lineList [LINE_BUF_SZ];                 // a circular buffer of command lines from user, unprocessed
volatile unsigned char gLineBufIn =0;       // which command line buffer is being filled
volatile unsigned char gLineBufOut = 0;     // which command line buffer is being processed
volatile unsigned char gBufState =0;        // 0 when empty, 2 when full, 1 when partly full

ERR errList [ERR_BUF_SZ];                       // array of errors that have occurred
volatile unsigned char gErrBufIn=0;             // position in buffer of errors
volatile unsigned char gErrBufOut=0;

volatile unsigned char lineInProgress;      // flag set to 1 when user is entering a command
                                            // flag set to 2 when printing error msg
volatile unsigned char rxLinePos=0;

volatile unsigned char gLineCnt =1;         // start at 1
#endif



void cmdRxIntFunc(char RXBUFF){
    if (!((lineInProgress == 2) || (gBufState == 2))){  // clear buffer but ignore user input when other code is printing error msg
        lineInProgress = 1;
        if (RXBUFF == '\r'){ // end of this command
            lineList[gLineBufIn].cmdLine[rxLinePos] = '\0';
            lineList[gLineBufIn].lineNum = gLineCnt;
            gLineCnt +=1;
            lineInProgress = 0;
            rxLinePos =0;
            gLineBufIn += 1;
            if (gCmdBufIn == LINE_BUF_SZ){
                gCmdBufIn = 0;
            }
            if (gCmdBufIn == gCmdBufOut){
                gBufState = 2;
            }else{
                gBufState = 1; // buffer is not empty and not full
            }
        } else{  // middle of a command
            if ((rxLinePos + 1) == CMD_LEN){    // line too long error 1st on the list

            }
            lineList[gLineBufIn].cmdLine[rxLinePos] = RXBUFF;
            rxLinePos +=1;
        }
        while (!(UCA1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send


    UCA1TXBUF = RXBUFF;           // always echo char to user till they screw up

        gCmdLineBuffer[cmdBufIn] [cmdLineCnt] = '\0';
        gCmdLineCnt =0;
        gCmdBufIn +=1;
        if (gCmdBufIn == CMD_BUF_SZ){
            gCmdBufIn = 0;
        }
        if (gCmdBufIn == gCmdBufOut){
            usciA1UartEnableRxInt (0);  // buffer is full
            gBufState = 2;
        }else{
            gBufState = 1; // buffer is not empty and not full
        }
    } else{ // in the middle of this command
        gCmdLineBuffer[gCmdBufIn] [gCmdLineCnt] = RXBUFF;
        gCmdLineCnt +=1;
        if (gCmdLineCnt == CMD_LEN){ // end of line, so no room for terminator
            gCmdLineCnt =0;  // to redo this command
            usciA1UartEnableRxInt (0);  // STOP receiving more command data
            gTxStrPtr = gErrStr1;
            gError = 101;
            usciA1UartEnableTxInt (1);  // warn user
        }
    }
}

unsigned char msgTxIntFunc(){
    unsigned char rChar;
    if ( *(gTxStrPtr +1) == '\0'){ // we are done with this error string
        usciA1UartEnableTxInt (0);  // turn off tx interrupt
        gError = 0;                 // error is dealt with.
    }
    gTxStrPtr +=1;
    return * (gTxStrPtr -1);
}
#endif





