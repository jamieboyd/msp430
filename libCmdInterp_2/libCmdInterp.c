/*************************************************************************************************
 *  libCMdInterp
 *  Library to interpret and run commands sent over UART. What commands are sent is up to you.
 *  Author: Jamie Boyd
 *  Created on: 2022/03/15
 *  Last Modified: 2022/03/22
 *  **************************************************************************************************/

#include <msp430.h> 
#include "libCmdInterp.h"

// for describing commands I know about
static CMDptr gCmdArrayPtr;                         // pointer to array of commands, will be initialized at start of program
static unsigned char gCmdArraySize = INIT_SIZE;     // size of the array of commands
static unsigned char gNumCommands = 0;              // number of commands, <= array size, or will be resized

// for describing errors I know about
static char ** gErrArrayPtr;                         // array of pointers to error strings, that's pointers to char pointers, will be initialized at start
static unsigned char gErrArraySize = INIT_SIZE;         // size of the array of errors
static unsigned char gNumErrs = 0;                           // number of distinct error messages, <= array size, or will be resized

// for getting user commands
volatile unsigned char gCmdCntIn=0;                    // number of command being processed, increments each time
char gCMDstrs[BUFF_SIZE * STR_SIZE];                // used to hold commands as entered by user, we do a single array and use pointer arithmetic to find start of string of interest
volatile unsigned char gInCmd = 0;                    // for circular buffer of commands we are processing
volatile unsigned char gOutCmd = 0;         // for circular buffer of commands we are processing
volatile unsigned char gCmdBufState = 0;      // 0 means empty, 1 means inProgress, 2 means full

// for parsing and executing user commands
CMDdata gCMDdata;                                  // structure used and re-used to pass data to each command, only need one

// for printing returned messages for each command
volatile unsigned char gCmdCntOut=0;
char gErrs [BUFF_SIZE * STR_SIZE];        // we do a single array and use pointer arithmetic to find start of string of interest
//volatile unsigned char gErrs [] = {5,5,5,5,5,5};     // used to hold indexes into our array of pre-made error message strings
volatile unsigned char gInErr =0;                   // for circular buffer of errors we are processing
volatile unsigned char gOutErr =0;         // for circular buffer of errors we are processing
volatile unsigned char gErrBufState = 0;      // 0 means empty, 1 means inProgress, 2 means full

volatile unsigned char gIsPrinting=0;                    // set when someone is printing to serial, or does not want anyone else to print right now
volatile unsigned char gStopPrinting=0;                   // set when someone would like the floor, asking others to stop printing

/******************************** libCMD_INIT ****************************************************
* Function: libCMD_INIT
* - Initializes UART and installs TX and RX interrupts, makes arrays for CMDs and results,
*   and adds first 5 error messages
* Arguments: 0
* returns:  1 for success, 0 if could not allocate memory for needed arrays
* Author: Jamie Boyd
* Date: 2022/03/16
************************************************************************************/
unsigned char libCMD_init (){
    unsigned char rVal = 1;     // successs
    usciA1UartInit(19200); // initialise UART for 19200 Baud communication
    gCmdArrayPtr = (CMD *)malloc (INIT_SIZE * sizeof (CMD));  // array of CMD structures, can be resized
    gErrArrayPtr = (char **)malloc (INIT_SIZE * sizeof (char *)); // array of pointers to static error strings
    if ((gCmdArrayPtr != NULL) && (gErrArrayPtr != NULL)){
        libCMD_addErr (ERR0);       // fill first 6 error messages
        libCMD_addErr (ERR1);
        libCMD_addErr (ERR2);
        libCMD_addErr (ERR3);
        libCMD_addErr (ERR4);
        libCMD_addErr (ERR5);
        usciA1UartInstallRxInt (&libCMD_RxInterrupt);   // install UART interrupts
        usciA1UartInstallTxInt (&libCMD_TxInterrupt);
        usciA1UartEnableRxInt (1);                      // enable Rx interrupt right away
        usciA1UartEnableTxInt (0);
        usciA1UartTxString ("Press any key and wait for prompt\r\0");           // always wanted to say "Press any key to continue"
        _enable_interrupts();                           // enable interrupts
    }else{
        usciA1UartTxString ("Command Interpreter failed to make buffers\r\0");
        rVal = 0;
    }
    return rVal;
}

/************************************************************************************
* Function: libCMD_addCmd
* - Adds a command to the list of commands I recognize
* Arguments: 4
* cmdNameP - name of the command to be sent by UART
* nArgsP - number of numeric arguments, always first in parameter list
* nStrArgsP - number of string arguments, always last in parameter list
* theCommandP - function pointer to function that takes a CMDdataPtr argument and returns an unsigned error code
* returns: 1 for success, 0 for error (if needs to allocate memory and can not)
* Author: Jamie Boyd
* Date: 2022/03/16
************************************************************************************/
unsigned char libCMD_addCmd (char * cmdNameP, unsigned char nArgsP, unsigned char nStrAgsP, unsigned char resultTypeP, command theCommandP){
    unsigned char rVall = 1;
    if (gNumCommands == gCmdArraySize){
        CMDptr newArray  = (CMDptr)malloc (gCmdArraySize * 2 * sizeof (CMD));
        if (newArray != NULL){
            unsigned char iMem;
            for (iMem=0;iMem < gCmdArraySize; iMem +=1){
                newArray [iMem] = gCmdArrayPtr [iMem];
            }
            gCmdArraySize *= 2;
            free (gCmdArrayPtr);
            gCmdArrayPtr = newArray;
        } else{
            rVall = 0;
        }
    }
    if (rVall == 1){
        gCmdArrayPtr[gNumCommands].name = cmdNameP;
        gCmdArrayPtr[gNumCommands].theCommand = theCommandP;
        gCmdArrayPtr[gNumCommands].nArgs = nArgsP;
        gCmdArrayPtr[gNumCommands].nStrArgs = nStrAgsP;
        gCmdArrayPtr[gNumCommands].resultType = resultTypeP;
        gNumCommands +=1;
    }
    return rVall;
}

/************************************************************************************
* Function: libCMD_addErr
* - Adds an error to the list of errors I know about.
* - ErrStr can not be string literal - it must have an address and keep existing after function returns
* - because characters are not copied, just the address
* Arguments: 1
* errStr - pointer to the character string that is your error message
* returns: offset to this error in the error string array, or 0 if needed to allocate and could not
* Author: Jamie Boyd
* Date: 2022/03/16
************************************************************************************/
unsigned char libCMD_addErr (char * errStr){
    unsigned char rVal =1;
    if (gNumErrs == gErrArraySize){
        char ** newArray = (char **)malloc (gErrArraySize * 2 * sizeof (char *));
        if (newArray != NULL){
            unsigned char iMem;
            for (iMem=0;iMem < gErrArraySize; iMem +=1){
                newArray [iMem] = gErrArrayPtr [iMem];
            }
            gErrArraySize *= 2;
            free (gErrArrayPtr);
            gErrArrayPtr = newArray;
        }else{
            rVal = 0;
        }
    }
    if (rVal ==1){
        gErrArrayPtr [gNumErrs] = errStr;
        rVal = gNumErrs;  // offset to this error
        gNumErrs +=1;
    }
    return rVal;
}

/*********************************** libCMD_run *************************************************
* Function: libCMD_run
* - main loop. Goes into low power mode. When woken, processes commands till command buffer is empty
* Arguments: None
* returns: Nothing
* Author: Jamie Boyd
* Date: 2022/03/10
*************************************************************************************/
void libCMD_run (void){

    while (1){
        __low_power_mode_0();
        while (gCmdBufState > BUFF_EMPTY){
            libCMD_doNextCommand ();
        }
    }
}

/**************************** timer0A1Isr ***************************************
 * -timer interrupt that checks if command stings are ready, then parses, validates, and runs commands
 #pragma vector = TIMER0_A1_VECTOR
__interrupt void timer0A1Isr(void) {
    static unsigned char lastCommand =0;
     if ((gCmdCnt > lastCommand) && (gCmdBufState > 0)){       // command buffer is not empty, so process a command
         //TA0CTL &= ~TAIE;         // so no reentrant interrupts when we:
         //_enable_interrupts();       // enable other interrupts, allowing this potentially slow interrupt to be interrupted by reading and writing
         libCMD_doNextCommand ();
         lastCommand +=1;

        // TA0CTL |= TAIE;         // turn our own interrupts back on
     }
     TA0CTL &= ~TAIFG;
 }*/


/*********************************** libCMD_doNextCommand *************************************************
* Function: libCMD_doNextCommand
* - parses next command line from user, filling out data in the CMDdata structure
*  using info from the command's entry in CMD list, then runs command function with parsed arguments,
*  and adds result to the print buffer
* Arguments: None
* returns: Nothing - lotsa side effects obviously
* Author: Jamie Boyd
* Date: 2022/02/10
*************************************************************************************/
 void libCMD_doNextCommand (void){
     static char * cmdLine = gCMDstrs;   // start at 1st command, then add STR_SIZE size each time through
     static char * resLine = gErrs;      // start at first result, then add STR_SIZE each time through
    // used by string tokenizing
    char * contextPtr = NULL;   // always starts off NULL do tokenizer knows it is first string
    char * aToken;              // used for string tokenizer
    // for dealing with tokenized arguments
    unsigned char cmdIndex;      // index into commands, as found by validateCMD, or -1 for not found
    signed int argVal;         // all numeric arguments are unsigned ints
    unsigned char ii;        // to iterate through commands, and arguments as they get tokenized
    unsigned char errVal;     // 0 for success or an error code
    command theCommand;        // the function to run, with signature defined in typedef for command function
    unsigned int resultType =0;
    if (gCmdBufState > BUFF_EMPTY){
        // see if command name exists
        aToken = libCMD_strTok (cmdLine, &contextPtr); // first token contains command name.
        errVal = 1;         // start with 1, code for command not exists
        for (ii =0; ii < gNumCommands; ii +=1){
            if (libCMD_strCmp(aToken, (char *)gCmdArrayPtr[ii].name)){
                cmdIndex = ii;
                errVal =0;
                break;
            }
        }
        if (errVal == 0){     // command was found
            // get numeric args, if any
            for (ii =0; ii < gCmdArrayPtr[(unsigned int)cmdIndex].nArgs; ii +=1){
                aToken = libCMD_strTok (NULL, &contextPtr);
                if (aToken != NULL){
                    argVal = libCMD_parseArg (aToken, &errVal);     // errVal is set if number can't be tokenized
                    if (errVal == 0){
                        gCMDdata.args[ii] = argVal;
                    }else{
                        break;
                    }
                }else{        // not enough numeric arguments
                    errVal = 2;
                    break;
                }
            }
            if (errVal == 0){
                // get string args, if any
                for (ii =0; ii < gCmdArrayPtr[cmdIndex].nStrArgs; ii +=1){
                    aToken = libCMD_strTok (NULL, &contextPtr);
                    if (aToken != NULL){
                        libCMD_strCpy (aToken, gCMDdata.strArgs[ii]);
                    }else{        // not enough string arguments
                        errVal = 4;
                        break;
                    }
                }
                if (errVal == 0){     // this next read SHOULD be NULL
                    aToken = libCMD_strTok (NULL, &contextPtr);
                    if (aToken != NULL){
                        errVal = 5;    // too many args
                    }
                    if (errVal == 0){     // finally, run the command if no error so far
                        theCommand = gCmdArrayPtr[(unsigned char)cmdIndex].theCommand;
                        errVal = theCommand (&gCMDdata); // run the command with the data that was parsed, get result to print
                        resultType = gCmdArrayPtr[(unsigned char)cmdIndex].resultType;
                    }
                }
            }
        }
    }

    if ((errVal > 0) || (resultType ==0)){       // print error/result message
        sprintf ((char *)resLine, "CMD %d->%s\r\0",  gCmdCntOut, gErrArrayPtr[errVal]); // err code matches index of error string
    }else{      // print return value
        switch (resultType){
        case R_UCHAR:
            sprintf (resLine, "CMD %d->%u\r\0",  gCmdCntOut, (unsigned char)gCMDdata.result);
            break;
        case R_SCHAR:
            sprintf (resLine, "CMD %d->%d\r\0",  gCmdCntOut, (signed char)gCMDdata.result);
            break;
        case R_UINT:
            sprintf (resLine, "CMD %d->%hu\r\0",  gCmdCntOut, (unsigned int)gCMDdata.result);
            break;
        case R_SINT:
            sprintf (resLine, "CMD %d->%hd\r\0",  gCmdCntOut, (signed int)gCMDdata.result);
            break;
        case R_ULONG:
            sprintf (resLine, "CMD %d->%lu\r\0",  gCmdCntOut, (unsigned long int)gCMDdata.result);
            break;
        case R_SLONG:
            sprintf (resLine, "CMD %d->%ld\r\0",  gCmdCntOut, (signed long int)gCMDdata.result);
            break;
        case R_FLOAT:
            sprintf (resLine, "CMD %d->%f\r\0",  gCmdCntOut, (float)gCMDdata.result);
            break;
        case R_STRING:
            sprintf (resLine, "CMD %d->%s\r\0",  gCmdCntOut, (char *)gCMDdata.result);
        }
    }
//    usciA1UartTxString (resLine);
    gCmdCntOut +=1;

    // increment out position in buffer of commands, cause we processed one
     gOutCmd += 1;
     cmdLine += STR_SIZE;
    if (gCmdBufState == BUFF_FULL){  //  command buffer state was full,, now room for one more
        gCmdBufState = BUFF_AVAIL;
    }
    if (gOutCmd == BUFF_SIZE){
        gOutCmd = 0;
        cmdLine = gCMDstrs;
    }
    if (gOutCmd == gInCmd){
        gCmdBufState = BUFF_EMPTY;
    }
    // increment in position in buffer of errMsgs to be printed, cause we added one
    gInErr +=1;
    resLine += STR_SIZE;
    if (gErrBufState == BUFF_EMPTY){     // error buffer was empty, now has 1 message in it
        gErrBufState = BUFF_AVAIL;
     }
    if (gInErr == BUFF_SIZE){
        gInErr = 0;
        resLine = gErrs;
    }
    if (gInErr == gOutErr){
        gErrBufState = BUFF_FULL;
    }
    if (!(gIsPrinting)){
            usciA1UartEnableTxInt (1);
            gIsPrinting = 1;
    }
}


 /*********************************** libCMD_RxInterrupt *************************************************
 * Function: libCMD_RxInterrupt
 * - runs when a character is received. Adds it to the buffer of command strings.
 * - Does NOT echo character - host computer terminal app must have echo on
 * Arguments: 1
 *   RXBUF - the character in the buffer
 * returns: 1 if a command has been entered and is ready to process, else 0
 * Author: Jamie Boyd
 * Date: 2022/03/20
 *************************************************************************************/
unsigned char libCMD_RxInterrupt (char  RXBUF){
    static unsigned int rCharCount = 255;    // count of characters in command buffer, 255 is start signal condition
    static char * cmdPtr = gCMDstrs;         // pointer will kept pointing to start of current result message, incremented by STR_SIZE at end
    static char promptStr [15];             // small string buffer for command prompt
    unsigned char lpm =0;                   // return value, will be set to 1 to wake from low power mode at end of a command
    if (gCmdBufState < 2){                  // Buffer is not full, we can accept new commands
        if (rCharCount == 255){              // signal value for start of a new command
            gStopPrinting = 1;              // not a good time to be printing to host - user just started entering a command
            while (gIsPrinting){};
            gIsPrinting = 1;
            gStopPrinting = 0;
            sprintf (promptStr,"\bCMD %d:\0", gCmdCntIn); // display prompt for user, back-apcing over character used to get our attention
            usciA1UartTxString (promptStr);
            rCharCount =0;                       // reset char count to 0
        } else{                                 // in the middle of a command
            if (RXBUF == 127) {                  // delete key, so delete previous char in buffer, by decrementing count
                if (rCharCount > 0){
                    rCharCount -= 1;
                }
            } else{
                if (rCharCount == (STR_SIZE - 1)){                       // this command is full, and its last char is not \r.  Reset
                   sprintf (promptStr,"\r< %d chars!\r\0", rCharCount);    // tell user that buffer was exceeded
                   usciA1UartTxString (promptStr);
                   sprintf (promptStr,"CMD %d:\0", gCmdCntIn); // display new command prompt for user
                   usciA1UartTxString (promptStr);
                   rCharCount =0;
               }else{
                   if (RXBUF == '\r'){                     // command fully entered
                       lpm = 1;                            // set lpm to wake from low power mode
                       gCmdCntIn +=1;
                       // gCMDstrs [gInCmd * charCount] = '\0';
                       *(cmdPtr + rCharCount) = '\0';        // null terminate the string
                       rCharCount = 255;                              // reset char count
                       gInCmd += 1;                                   // increment gInCmd
                       cmdPtr += STR_SIZE;
                       if (gInCmd == gNumCommands){
                           gInCmd = 0;
                           cmdPtr = gCMDstrs;
                       }
                       if (gCmdBufState == BUFF_EMPTY){
                           gCmdBufState = BUFF_AVAIL;
                       }
                       if (gInCmd == gOutCmd){
                           gCmdBufState = BUFF_FULL;
                       }
                       if (gErrBufState > BUFF_EMPTY){   // we can allow some printing now, so turn on Tx interrupt if err buffer is not empty
                           usciA1UartEnableTxInt (1);
                       }else{
                           gIsPrinting = 0;
                       }
                   }else{                                  // command not fully entered yet
                        *(cmdPtr + rCharCount++) = RXBUF; // add received char to buffer, post-increment character count
                    }
                }
            }
        }
    }
   return lpm;
}

/******************************** libCMD_TxInterrupt ****************************************************
* Function: libCMD_TxInterrupt prints results messages from buffer
* - Called when it is enabled and TXBUF is empty. disables itself when result buffer is empty
* Arguments: 1
*   lpm  - pointer to an unsigned char which can be set to 1 to wake from low power mode, but we always leave it at 0
* returns: the next character from the global gErrStr
* Author: Jamie Boyd
* Date: 2022/03/16
************************************************************************************/
char libCMD_TxInterrupt (unsigned char* lpm){
    static unsigned char tCharCount=0;
    static char * errPtr = gErrs;
    char rChar;
    rChar = *(errPtr + tCharCount++) ;    // get next char in this result message, and increment char count
    if (*(errPtr + tCharCount) == '\0'){  // next char is terminator, so this string is done
        tCharCount = 0;
        gOutErr += 1;
        errPtr += STR_SIZE;
        if (gOutErr == BUFF_SIZE){  // wrap around to start of buffer
            gOutErr = 0;
            errPtr = gErrs;
        }
        if (gErrBufState == BUFF_FULL){  // err buffer was full
            gErrBufState = BUFF_AVAIL;    // but not any more
        }
        if (gOutErr == gInErr){  // buffer is empty
           gErrBufState = BUFF_EMPTY;
        }
        if ((gStopPrinting ==1) || (gErrBufState == BUFF_EMPTY)){
              gIsPrinting = 0;
            usciA1UartEnableTxInt (0);  // disable tx interrupt
        }
    }
    return rChar;
}


/*************************************libCMD_strTok***********************************************
* Function: libCMD_strTok
* - a custom string tokenizer. Works on null-terminated string with separators (space, comma, tab)
* Arguments: 2
* argument 1: buffer - on first call, pass pointer to text buffer to be tokenized, subsequently pass NULL
* argument 2: contextP, address of a char pointer. Set on 1st call to end of token and used on subsequent calls
* returns: pointer to tokenized portion of string, or NULL if no tokens left in string
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
char * libCMD_strTok (char * stringWithSeps, char** contextP){
    if (stringWithSeps == NULL){ // this is a repeated call and contextPtr MUST be set
        if (*contextP == NULL){ // so check that it is not null, and return null if so
            return NULL;
        }
    }else{                           // this is the first call to strTok for this string
        *contextP = stringWithSeps;  // so initialize contextP to start of string
    }
    // check if we are at end of string
    if (**contextP == '\0'){
        return NULL;
    }
    // skip over any extra delimiters at start of string
    while ((((**contextP == ' ') || (**contextP == ',')) || (**contextP == '\t')) && (**contextP != '\0')){
        *contextP +=1;
    }
    if (**contextP == '\0'){ // we do not have another token
        return NULL;
    }else{ // we do have a token, get ready to return it
        stringWithSeps = *contextP;
        // look for delimiter at end of token, or \0 for end of string
        while ((((**contextP != ' ') && (**contextP != ',')) && (**contextP != '\t')) && (**contextP != '\0')){
            *contextP +=1;
        }
        if (**contextP != '\0'){
            **contextP = '\0'; // write terminator over first separator character
            *contextP +=1;   // increment pointer to start of next token, for next call
        }
        return stringWithSeps;
    }
}

/************************************************************************************
* Function: libCMD_strCmp
* -  Compares two strings and returns the truth that they are equivalent
* Arguments: 2
* argument 1: string 1
* argument 2: string 2
* returns: 1 if strings are equivalent, 0 if they are not
* Author: Jamie Boyd
* Date: 2022/02/10
* ***********************************************************************************/
unsigned char libCMD_strCmp (char * str1, char * str2){
    while ((*str1 == *str2) && (!(*str1 == '\0' || *str2 == '\0'))){
        str1 +=1;
        str2 +=1;
    }
    return ((*str1 == '\0') && (*str2 == '\0'));
}

/************************************************************************************
* Function: libCMD_strCpy
* -  copies the contents of string str1 to string str2
* Arguments: 2
* argument 1: string 1
* argument 2: string 2
* returns: nothing
* Author: Jamie Boyd
* Date: 2022/02/10
* ***********************************************************************************/
void libCMD_strCpy (char * str1, char * str2){
    while (*str1 != '\0'){
        *str2++ = *str1++;
    }
    *str2 = '\0';
}

/******************************* libCMD_parseArg *****************************************************
* Function: libCMD_parseArg, basically AtoI with extras for msp430
* -  parses a string representing a decimal, hex, or binay value, returning the number as a signed integer
* - because any value can be good data, a pass-by-reference argument is needed for error reporting
* Arguments: 2
* aToken: a null-terminated string containing the value
* err: a pass-by-reference value to set if an error occurs
* returns: signed int corresponding to the value
* Author: Jamie Boyd
* Date: 2022/01/15
* Modified:2022/01/23 by Jamie Boyd - now does 000110b style binary and checks for negative sign on decimals
* Modified:2022/02/14 by Jamie Boyd - made power an unsigned int to enable larger values
* ***********************************************************************************/
signed int libCMD_parseArg (char * aToken, unsigned char * err){
    signed char tokLen, tokPos;
    unsigned char decTokEnd;
    unsigned int power;
    signed int rVal =0;
    *err =0;
    for (tokLen = 0; *(aToken + tokLen) != '\0'; tokLen +=1){}; // find token length so we can work backwards
    if ((*aToken == '0') && (*(aToken + 1) == 'x')){ // we have hex digit, note the x is case sensitive
        for (power=1, tokPos = tokLen -1; tokPos >= 2; tokPos -=1, power *= 16){
            if ((*(aToken + tokPos) >= '0') && (*(aToken + tokPos) <= '9')){
                rVal += (*(aToken + tokPos) - 48) * power;
            }else{
                if ((*(aToken + tokPos) >= 'A') && (*(aToken + tokPos) <= 'F')){ // note case sensitive
                    rVal += (*(aToken + tokPos) - 55) * power;
                }else{
                    *err = 3;    // error parsing number
                    break;
                }
            }
        }
    }else{
        if(*(aToken + tokLen-1) == 'b'){    // we have binary values, note the b is case sensitive
            for (power=1, tokPos = tokLen-2; tokPos >= 0; tokPos -=1, power *= 2){
                if (*(aToken + tokPos) == '1') {
                    rVal += power;
                }else{
                   if (*(aToken + tokPos) != '0'){
                       *err = 3;
                       break;
                   }
                }
            }
        } else{     // we have decimal values
            if (*aToken == '-'){
                decTokEnd = 1;      // negative value
            }else{
                decTokEnd = 0;
            }

            for (power=1, tokPos = tokLen-1; tokPos >= decTokEnd; tokPos -=1, power *= 10){
               if ((*(aToken + tokPos) >= '0') && (*(aToken + tokPos) <= '9')){
                   rVal += (*(aToken + tokPos) - 48) * power;
               }else{
                   *err = 3; // error parsing number
                   break;
               }
            }
            if (decTokEnd == 1){
                rVal *= -1;
          }
        }
    }
    return rVal;     // data will only be valid if *err is 0
}

/************************************************************************************
* Function: validateCmd
* - searches through command names in command list looking for a match
* Arguments: 2
* argument 1: cmdList - the list of commands with names
* argument 2: a name
* returns: index of command with matching name, or -1 if no match was found
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
signed char libCMD_validateCmd(CMD * cmdList ,char * cmdName) {
    unsigned char ii;
    signed char cmdIndex = -1;
    for (ii =0; ii < gNumCommands; ii +=1){
        if (libCMD_strCmp(cmdName, (char *)cmdList[ii].name)){
            cmdIndex = ii;
            break;
        }
    }
    return cmdIndex;
}
