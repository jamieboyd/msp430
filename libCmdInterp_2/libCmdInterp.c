#include <msp430.h> 
#include "libCmdInterp.h"
#include "libUART1A.h"
#include <stdlib.h>
#include <stdio.h>

// for describing commands I know about
static CMDptr gCmdArrayPtr;
static unsigned char gCmdArraySize = INIT_SIZE;     // size of the array of commands
static unsigned char gNumCommands = 0;              // number of commands, <= array size, or will be resized

// for describing errors I know about
static char ** gErrArrayPtr;                         // array of pointers to error strings, that's pointers to char pointers
static unsigned char gErrArraySize = INIT_SIZE;
static unsigned char gNumErrs = 0;                           // number of errors, <= array size, or will be resized

// for getting user commands
volatile unsigned char gCmdCnt=1;                    // number of command being processed, increments each time
volatile char gCMDstrs[][STR_SIZE]= {"0", "1", "2", "3", "4", "5"};                // used to hold commands as entered by user
volatile unsigned char gInCmd = 0;                    // for circular buffer of commands we are processing
volatile unsigned char gOutCmd = 0;         // for circular buffer of commands we are processing
volatile unsigned char gCmdBufState = 0;      // 0 means empty, 1 means inProgress, 2 means full

// for parsing and executing user commands
CMDdata gCMDdata;                                  // structure used and re-used to pass data to each command

// for printing error messages for each command
volatile unsigned char gErrCnt = 1;                  // count of commands for which messages have been sent
volatile unsigned char gErrs [] = {0,1,2,3,4,5};     // used to hold indexes into our array of pre-made error message strings
volatile unsigned char gInErr =0;                   // for circular buffer of errors we are processing
volatile unsigned char gOutErr =0;         // for circular buffer of errors we are processing
volatile unsigned char gErrBufState = 0;      // 0 means empty, 1 means inProgress, 2 means full

volatile unsigned char gIsPrinting=0;                    // set when someone is printing to UART
volatile unsigned char gSetPrinting=0;


const char gsBufStr [STR_SIZE];

/******************************** libCMD_INIT ****************************************************
* Function: libCMD_INIT
* - Initialises
* Arguments: 0
* cmdNameP - name of the command to be sent by UART
* nArgsP - number of numeric arguments, always first in parameter list
* nStrArgsP - number of string arguments, always last in parameter list
* theCommandP - function pointer to function that takes a CMDdataPtr argument and returns an unsigned error code
* returns: 1 for success, 0 for error (if needs to allocate memory and can not)
* Author: Jamie Boyd
* Date: 2022/03/16
************************************************************************************/
unsigned char libCMD_init (){
    unsigned char rVal;
    gCmdArrayPtr = (CMDptr)malloc (INIT_SIZE * sizeof (CMD));
    gErrArrayPtr = (char **)malloc (INIT_SIZE * sizeof (char **));
    if ((gCmdArrayPtr != NULL) && (gErrArrayPtr != NULL)){
        libCMD_addErr (ERR0);       // fill first 7 error messages
        libCMD_addErr (ERR1);
        libCMD_addErr (ERR2);
        libCMD_addErr (ERR3);
        libCMD_addErr (ERR4);
        libCMD_addErr (ERR5);
        libCMD_addErr (ERR6);
        usciA1UartInit(19200);                          // initialise UART for 19200 Baud communication
        usciA1UartInstallRxInt (&libCMD_RxInterrupt);   // install UART interrupts
        usciA1UartInstallTxInt (&libCMD_TxInterrupt);
        usciA1UartEnableRxInt (1);                      // enable Rx interrupt right away
        TA1CTL = TASSEL_2 | ID_0 | MC_1 | TAIE;         // initialise timer with interrupt every 10 ms
        TA1CCR0 = 10485;                                // set timer interval to approx 10 ms (2^20 * 10E-3 - 1)
        TA1CTL &= ~TAIFG;                               // clear timer interrupt flag
        _enable_interrupts();                           // enable interrupts
        rVal = 1;
    }else{
        rVal = 0;
    }
    return rVal;
}

/************************************************************************************
* Function: libCMD_addCmd
* - Adds a command to the list of commands I recognise
* Arguments: 4
* cmdNameP - name of the command to be sent by UART
* nArgsP - number of numeric arguments, always first in parameter list
* nStrArgsP - number of string arguments, always last in parameter list
* theCommandP - function pointer to function that takes a CMDdataPtr argument and returns an unsigned error code
* returns: 1 for success, 0 for error (if needs to allocate memory and can not)
* Author: Jamie Boyd
* Date: 2022/03/16
************************************************************************************/
unsigned char libCMD_addCmd (char * cmdNameP, unsigned char nArgsP, unsigned char nStrAgsP, command theCommandP){
    unsigned char rVal;
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
            rVal =1;
        } else{
            rVal = 0;
        }
    }
    if (rVal == 1){
        gCmdArrayPtr[gNumCommands].name = cmdNameP;
        gCmdArrayPtr[gNumCommands].theCommand = theCommandP;
        gCmdArrayPtr[gNumCommands].nArgs = nArgsP;
        gCmdArrayPtr[gNumCommands].nStrArgs = nStrAgsP;
        gNumCommands +=1;
    }
    return rVal;
}

// returns offset to this error in the error string array, or 0 if needed to allocate and could not
// errStr can not be literals - it must have an address and keep existing after function returns
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

/*************************** timer1A1Isr ***************************************
 * -timer interrupt that checks if command stings are ready, then parses, validates, and runs commands */
 #pragma vector = TIMER1_A1_VECTOR
 __interrupt void timer1A1Isr(void) {
     signed char cmdIndex;
     command theCommand;
     if (gCmdBufState > 0){       // command buffer is not empty, so process a command
         _enable_interrupts();       // enable other interrupts, allowing this slow interrupt to be interrupted by reading and writing
         cmdIndex = libCMD_parseCmd(gOutCmd);
         if (cmdIndex >= 0){                     // command was found and args were parsed
             theCommand = gCmdArrayPtr[(unsigned char)cmdIndex].theCommand;
             cmdIndex = theCommand (&gCMDdata);
         }else {    // cmdIndex is negative of error code
             cmdIndex *= -1;
         }
         libCMD_PushErr (cmdIndex);
         if ((gIsPrinting) && (gErrBufState > 0)){
             usciA1UartEnableTxInt (1);
         }
         gOutCmd += 1;
         if (gOutCmd == BUFF_SIZE){
             gOutCmd = 0;
         }
         if (gCmdBufState == 0){
             gCmdBufState = 1;
         }
         if (gOutCmd == gInCmd){
             gCmdBufState = 0;
         }
     }
     TA1CTL &= ~TAIFG;        // clear TAIFG interrupt flag,
 }

 // puts an error code on the array of error codes
 void libCMD_PushErr (unsigned char theError){
     if (gErrBufState == 0){
         gErrBufState = 1;
     }
     if (gErrBufState < 2){
         gErrs [gInErr++] == theError;
         if (gInErr == gNumErrs){
             gInErr = 0;
         }
         if (gInErr = gOutErr){
             gErrBufState = 2;
         }
     }
 }


 void libCMD_waitOnPrint (void){
     while (gIsPrinting) {}; // need printing to be cleared
     gIsPrinting = 1;
     usciA1UartEnableTxInt (0);  // disable tx interrupt
 }


/*********************************** libCMD_parseCmd *************************************************
* Function: libCMD_parseCmd
* - parses command line from user, filling out data in a CMD structure from CMD list
* Arguments: 1
*   gOutcmd - the list of commands with names
* returns: positive index of command, or - index of error string,from -1 to -6
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
signed char libCMD_parseCmd(unsigned char gOutcmd){
    char * contextPtr = NULL;
    char * cmdLine = gCMDstrs [gOutcmd];
    char * aToken = libCMD_strTok (cmdLine, &contextPtr); // first token contains command name.
    unsigned char iArg;
    signed int argVal;
    signed char error;
    int cmdIndex = libCMD_validateCmd(gCmdArrayPtr, aToken);
    if (cmdIndex >= 0){
        // get numeric args
        for (iArg =0; iArg < gCmdArrayPtr[(unsigned int)cmdIndex].nArgs; iArg +=1){
            aToken = libCMD_strTok (NULL, &contextPtr);
            if (aToken != NULL){
                argVal = libCMD_parseArg (aToken, &error);
                if (error == 0){
                    gCMDdata.args[iArg] = argVal;
                }else{
                    cmdIndex = error; // error in parsing number
                    break;
                }
            }else{        // not enough numeric arguments
                cmdIndex = -3;
                break;
            }
        }
        if (cmdIndex >= 0){
            // get string args
            for (iArg =0; iArg < gCmdArrayPtr[cmdIndex].nStrArgs; iArg +=1){
                aToken = libCMD_strTok (NULL, &contextPtr);
                if (aToken != NULL){
                    libCMD_strCpy (aToken, gCMDdata.strArgs[iArg]);
                }else{        // not enough string arguments
                    cmdIndex = -5;
                    break;
                }
            }
            if (cmdIndex >= 0){     // this read SHOUL be NULL
                aToken = libCMD_strTok (NULL, &contextPtr);
                if (aToken != NULL){
                    cmdIndex = -6;    // too many args
                }
            }
        }
    } else{      // command does not exist
        cmdIndex = -2;
    }
    return cmdIndex;
}

// gets characters from RXBUF, puts them in command string buffer. Does NOT echo character - host computer terminal app must do that
void libCMD_RxInterrupt (char  RXBUF){
    static unsigned int charCount = 255;    // count of characters in command buffer, 255 is start signal condition
    if (gCmdBufState < 2){                  // Buffer is not full
        if (charCount == 255){
            gSetPrinting = 0; // not a good time to be printing
            while (gIsPrinting){};
            sprintf ((char *)gsBufStr,"CMD %d:\0", gCmdCnt); // display prompt for user
            usciA1UartTxString (gsBufStr);
            charCount =0;
        } else{
            if (RXBUF == 127){                  // delete previous char in buffer
                if (charCount > 0){
                    charCount -= 1;
                }
            } else {
                if (RXBUF == '\r'){     // command fully entered
                    gSetPrinting = 1;    // allow some printing
                    if (gErrBufState > 0){
                        usciA1UartEnableTxInt (1);  // enable tx interrupt
                    }
                    gCmdCnt +=1;
                    gCMDstrs [gInCmd][charCount] = '\0';        // null terminate the string
                    charCount = 255;                              // reset char count
                    gInCmd += 1;                                 // increment InCmd
                    if (gInCmd == BUFF_SIZE){
                        gInCmd = 0;
                    }
                    if (gCmdBufState == 0){
                        gCmdBufState = 1;
                    }
                    if (gInCmd == gOutCmd){
                        gCmdBufState = 2;
                    }
                }else{                                          // command not fully entered yet
                    if (charCount == (STR_SIZE - 1)){                // this command is too long and no \r in sight
                        charCount = 255;
                        libCMD_PushErr (1);
                        gCmdCnt +=1;                                // we count bad commands in this version
                    }else{
                        gCMDstrs [gInCmd][charCount++] = RXBUF; // add received char to buffer, post-increment count
                    }
                }
            }
        }
    }
}


/******************************** libCMD_TxInterrupt ****************************************************
* Function: libCMD_TxInterrupt
* - Called when it is enabled and TXBUF is empty. disables itself when buffer is empty
* Arguments: None
* returns: the next character from the global gErrStr
* Author: Jamie Boyd
* Date: 2022/03/16
************************************************************************************/
char libCMD_TxInterrupt (void){
    static unsigned char charCount=0;
    char rChar;
    if (charCount == 0){
        sprintf ((char *)gsBufStr,"\r\n--CMD %d %s --\r\n\0", gErrCnt, gErrArrayPtr[gErrs [gOutErr]]); // err code matches index of err string
    }
    rChar = gsBufStr [charCount++];
    if (gsBufStr [charCount] == '\0'){
        charCount = 0;
        gErrCnt +=1;
        gOutErr += 1;
        if (gOutErr == BUFF_SIZE){
            gOutErr = 0;
        }
        if (gErrBufState ==2){  //full
            gErrBufState =1;
        }
        if (gOutErr == gInErr){
           gErrBufState = 0;
           gIsPrinting = 0;            // not currently sending characters to UART
        }
        if (gSetPrinting ==0){
            usciA1UartEnableTxInt (0);  // disable tx interrupt
            gIsPrinting = 0;
        }
    }
    return rChar;
}



/************************************************************************************
* Function: strTok
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
* Function: strCmp
* -  Compares two strings and returns the truth that they are equivalent
* Arguments: 2
* argument 1: string 1
* argument 2: string 2
* returns: 1 if strings are equivalent, 0 if they are not
* Author: Jamie Boyd
* Date: 2022/02/10
* ***********************************************************************************/
char libCMD_strCmp (char * str1, char * str2){
    while ((*str1 == *str2) && (!(*str1 == '\0' || *str2 == '\0'))){
        str1 +=1;
        str2 +=1;
    }
    return ((*str1 == '\0') && (*str2 == '\0'));
}

// copies the contents of string str1 to string str2
void libCMD_strCpy (char * str1, char * str2){
    while (*str1 != '\0'){
        *str2++ = *str1++;
    }
    *str2 = '\0';
}



unsigned char strLen (char * strBuffer){
    unsigned char rVal =0;
    for (rVal = 0; *(strBuffer + rVal) != '\0'; rVal +=1){};
    return rVal;
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
signed char libCMD_parseArg (char * aToken, signed char * err){
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
                    *err = -4;    // error parsing number
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
                       *err = -4;
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
                   *err = -4; // error parsing number
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
