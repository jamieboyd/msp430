/*
 * libCmdInterp.h
 * Command Interpreter receives strings of characters from the serial port UART, parses each string into commands and data, and
 * runs the associated function with the supplied data. To use the Command Interpreter, you must first define some commands.
 * a command has a command name, a function to run when the command name is received, and a description of the funtion's number
 * of parameters and return type. Numeric parameters, up to MAX_ARGS, are signed integers, and must be listed before the
 * string parameters (up to MAX_STR_ARGS). String parameters can not include  spaces, commas, or tabs, as these are separator
 * characters, and must contain fewer characters than MAX_STR_LEN. A result is printed for each command, either the value
 * assigned by the function, or an error message. Some general error messages are provided, Error messages specific
 * to your functions can be added. These must be static strings.
 *  Created on: Mar. 6, 2022, from previous versions without function references
 *      Author: jamie
 */
#ifndef LIBCMDINTERP_H_
#define LIBCMDINTERP_H_

#include "libUART1A.h"
#include <stdlib.h>
#include <stdio.h>

#define     MAX_ARGS        6       // max number of numeric arguments for a function
#define     MAX_STR_ARGS    3       // max number of string arguments for a function
#define     MAX_STR_LEN     12      // max length of a string argument
#define     INIT_SIZE       16      // initial size of arrays of commands and of errors I know about
#define     STR_SIZE        40      // max size for command strings and error message strings
#define     BUFF_SIZE       6      // size of buffers for command strings and errors

#define     BUFF_EMPTY      0       // Buffer is empty
#define     BUFF_AVAIL      1       // room is available for adding and items are available for removing
#define     BUFF_FULL       2       // buffer is full

// some strings for general error messages, those that are independent of the actual command
#define     ERR0        "success\0"
#define     ERR1        "CMD name does not exist\0"
#define     ERR2        "not enough arguments\0"
#define     ERR3        "argument not a number\0"
#define     ERR4        "not enough string args\0"
#define     ERR5        "too many args\0"

// some mnemonic constants for these errors
#define     SUCCESS     0 `
#define     NOT_EXISTS  1
#define     FEW_ARGS    2
#define     ARG_NAN     3
#define     FEW_STRS    4
#define     MANY_ARGS   5


// structure that will hold the data parsed from the command. Only need one of these
typedef struct CMDdata{
    signed int args[MAX_ARGS];                  // all signed ints, will be parsed and filled out by me
    char strArgs [MAX_STR_ARGS] [MAX_STR_LEN];  // array of strings, copied from the command
    signed long result;                            // function result goes here. 4 bytes, so can be cast to other types as needed
} CMDdata, * CMDdataPtr;

// type def for a function that takes a pointer to a CMDdata structure and returns an unsigned char error code
typedef unsigned char (*command)(CMDdataPtr commandData);  // command is a function that takes a pointer to a CMDdata struct and returns an error code

// a structure that describes a command. Each command you add gets one of these, stored in an array that cna be resized
// It does NOT hold the data your function gets when it runs, that is what the CMDdata structure is for
typedef struct CMD {                   // defines a single command
    char * name;                       // pointer for command name, will point to a string literal #defined by you
    command theCommand;                // pointer to the function that runs when command name is sent by UART, defined by you
    unsigned char nArgs;               // number of input parameters for the command, as defined by you
    unsigned char nStrArgs;            // number of input string parameters for the command, as defined by you
    unsigned char resultType;           // see mnemonic codes below
}CMD, * CMDptr;

// mnemonic codes for result types
#define     R_NONE      0
#define     R_UCHAR     1
#define     R_SCHAR     2
#define     R_UINT      3
#define     R_SINT      4
#define     R_ULONG     5
#define     R_SLONG     6
#define     R_FLOAT     7
#define     R_STRING    8       // pointer to a static string

/******************************** libCMD_INIT ****************************************************
* Function: libCMD_INIT
* - Initializes UART and installs TX and RX interrupts, makes arrays for CMDs and results,
*   and adds first 5 error messages
* Arguments: 0
* returns:  1 for success, 0 if could not allocate memory for needed arrays
* Author: Jamie Boyd
* Date: 2022/03/16 */
unsigned char libCMD_init (void);

/************************************* libCMD_addCmd ***********************************************
* Function: libCMD_addCmd
* - Adds a command to the list of commands I recognize
* Arguments: 5
* cmdNameP - name of the command to be sent by UART
* nArgsP - number of numeric arguments, always first in parameter list
* nStrArgsP - number of string arguments, always last in parameter list
* resultType - code for type of result
* theCommandP - function pointer to function that takes a CMDdataPtr argument and returns an unsigned error code
* returns: 0 for success, 1 for error (if needs to allocate memory and can not)
* Author: Jamie Boyd
* Date: 2022/03/16 */
unsigned char libCMD_addCmd (char * cmdNameP, unsigned char nArgsP, unsigned char nStrAgsP, unsigned char resultType, command theCommandP);

/*************************************** libCMD_addErr *********************************************
* Function: libCMD_addErr
* - Adds an error to the list of errors I know about.
* - ErrStr can not be string literal - it must have an address and keep existing after function returns
* - because characters are not copied, just the address
* Arguments: 1
* errStr - pointer to the character string that is your error message
* returns: offset to this error in the error string array, or 0 if needed to allocate and could not
* Author: Jamie Boyd
* Date: 2022/03/16 */
unsigned char libCMD_addErr (char * errStr);

/*********************************** libCMD_run *************************************************
* Function: libCMD_run
* - main loop. Goes into low power mode. When woken, processes commands till command buffer is empty
* Arguments: None
* returns: Nothing
* Author: Jamie Boyd
* Date: 2022/03/10 */
void libCMD_run (void);
/*********************************** libCMD_doNextCommand *************************************************
* Function: libCMD_doNextCommand
* - parses next command line from user, filling out data in the CMDdata structure
*  using info from the command's entry in CMD list, then runs command function with parsed arguments,
*  and adds result to the print buffer
* Arguments: None
* returns: Nothing - lotsa side effects obviously
* Author: Jamie Boyd
* Date: 2022/02/10 */
void libCMD_doNextCommand (void);


signed char libCMD_validateCmd(CMD * cmdList ,char * cmdName);
signed int libCMD_parseArg (char * aToken, unsigned char * err);

char libCMD_TxInterrupt (unsigned char* lpm);
unsigned char libCMD_RxInterrupt (char  RXBUF);

char * libCMD_strTok (char * stringWithSeps, char** contextP);
void libCMD_strCpy (char * str1, char * str2);
unsigned char libCMD_strCmp (char * str1, char * str2);



#endif /* LIBCMDINTERP_H_ */
