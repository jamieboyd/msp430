/*
 * BinaryCmdInter.h
 *
 *  Created on: Apr. 1, 2022
 *      Author: Jamie Boyd/Matthew Wonneberg
 */

#ifndef BINARYCMDINTERP_H_
#define BINARYCMDINTERP_H_

#define     CMD_LIST_SIZE   32      // arrays of command structures that we know about, room for  CMD_LIST_SIZE commands from Host.
#define     DATA_SIZE       20      // this many bytes available for each input command and output results. should be lots
#define     BUFF_SIZE       6      // size of buffers for commands and results. Make these bigger

#define     BUFF_EMPTY      0       // Buffer is empty
#define     BUFF_AVAIL      1       // space is available for adding and items are available for removing
#define     BUFF_FULL       2       // buffer is full

// type def for a function that takes a pointer to an input buffer, and to an output buffer. Here, each command does its own parsing of binary data
typedef unsigned char (*command)(unsigned char * inputData, unsigned char * outputResults);  // command is a function that takes a pointer to a CMDdata struct and returns an error code

// a structure that describes a command. Each command you add gets one of these, stored in an array of [CMD_LIST_SIZE] [DATA_SIZE]
typedef struct CMD {                   // defines a single command
    command theCommand;                // pointer to the function that runs when command name is sent by UART, defined by you
    unsigned char nCharsIn;               // number of bytes in input parameters for the command, as defined by Python
}CMD, * CMDptr;

void binInterp_init (void);
unsigned char binInterp_addCmd (unsigned char nCharsIn, command commandFuncPtr);
void binInterp_doNextCommand (void);
void binInterp_run (void);
unsigned char binInterp_RxInterupt (char RXBUF);
char binInterp_TxInterrupt (unsigned char* lpm);


#endif /* BINARYCMDINTERP_H_ */
