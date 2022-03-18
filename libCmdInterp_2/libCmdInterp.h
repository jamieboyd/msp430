/*
 * libCmdInterp.h
 * Command Interpreter receives strings of characters from serial port UART, parses each string into commands and data, and then
 * runs the command with the associated data. To use the Command Interpreter, you must first define some commands
 * a command has a command name that will be sent over the uart, a function that will run when it is called
 *  Created on: Mar. 6, 2022, from previous versions without function references
 *      Author: jamie
 */
#ifndef LIBCMDINTERP_H_
#define LIBCMDINTERP_H_

#define     MAX_ARGS        6       // max number of numeric arguments for a function
#define     MAX_STR_ARGS    3       // max number of string arguments for a function
#define     MAX_STR_LEN     12      // max length of a string argument
#define     INIT_SIZE       16      // initial size of arrays of commands and of errors I know about
#define     STR_SIZE        40      // max size for command strings and error message strings
#define     BUFF_SIZE       6      // size of buffers for command strings and errors
// some static strings for first error messages, those that are independent of actual command
#define     ERR0        "success\0"
#define     ERR1        "CMD too long\0"
#define     ERR2        "CMD name does not exist\0"
#define     ERR3        "not enough arguments\0"
#define     ERR4        "argument not a number\0"
#define     ERR5        "not enough string args\0"
#define     ERR6        "too many args\0"

// structure that will hold the data parsed from the command.
typedef struct CMDdata{
    signed int args[MAX_ARGS];                  // all signed ints, will be parsed and filled out by me
    char strArgs [MAX_STR_ARGS] [MAX_STR_LEN];  // strings.
} CMDdata, * CMDdataPtr;

// type def for a function that takes a pointer to a CMDdata structure and returns an unsigned char error code
typedef unsigned char (*command)(CMDdataPtr commandData);  // command is a function that takes a pointer to a CMDdata struct and returns an error code

// a structure that describes a command. Each command you add gets one of these. It does NOT hold the data your function needs to run
typedef struct CMD {                   // defines a single command
    char * name;                       // pointer for command name, will point to a string literal #defined by you
    command theCommand;                // pointer to the function that runs when command name is sent by UART, defined by you
    unsigned char nArgs;               // number of input parameters for the command, as defined by you
    unsigned char nStrArgs;            // number of input string parameters for the command, as defined by you
}CMD, * CMDptr;


/************************************************************************************
* Function: libCMD_addCmd
* - Adds a command to the list of commands I recognise
* Arguments: 4
* cmdNameP - name of the command to be sent by UART
* nArgsP - number of numeric arguments, always first in parameter list
* nStrArgsP - number of string arguments, always last in parameter list
* theCommandP - function pointer to function that takes a CMDdataPtr argument and returns an unsigned error code
* returns: 0 for success, 1 for error (if needs to allocate memory and can not)
* Author: Jamie Boyd
* Date: 2022/03/16 */
unsigned char libCMD_addCmd (char * cmdNameP, unsigned char nArgsP, unsigned char nStrAgsP, command theCommandP);
unsigned char libCMD_addErr (char * errStr);
signed char libCMD_parseCmd(unsigned char gOutcmd);
signed char libCMD_validateCmd(CMD * cmdList ,char * cmdName);

signed char libCMD_parseArg (char * aToken, signed char * err);

void libCMD_PushErr (unsigned char theError);

char libCMD_TxInterrupt (void);
void libCMD_RxInterrupt (char  RXBUF);

char * libCMD_strTok (char * stringWithSeps, char** contextP);
void libCMD_strCpy (char * str1, char * str2);



#endif /* LIBCMDINTERP_H_ */
