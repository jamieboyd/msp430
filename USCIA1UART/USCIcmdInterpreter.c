/*************************************************************************************************
 * UsciCmdInterpreter.c
 * - C implementation or source file for command interpreter for MSP430
 * Receives command strings from  usci UART A1 and interprets them, and executes them if they are valid
 *
 *  Author: Jamie Boyd
 *  Created on: 2022/01/10
 *  Modified: 2022/01/16 by Jamie Boyd
 **************************************************************************************************/

#include "UsciCmdInterpreter.h"


/************************************************************************************
* Function: cmdInterpInitCmds
* - Initializes an array of command structures using information in #defines in header file
* Arguments: 1
* argument1: array of command structures
* returns: nothing
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
void initCmds (CMD * cmdList){
    cmdList[0].name = CMD0;
    cmdList[0].nArgs = CMD0_NARGS;
    cmdList[1].name = CMD1;
    cmdList[1].nArgs = CMD1_NARGS;
    cmdList[2].name = CMD2;
    cmdList[2].nArgs = CMD2_NARGS;
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
char * strTok (char * stringWithSeps, char** contextP){
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
char strCmp (char * str1, char * str2){
    while ((*str1 == *str2) && (!(*str1 == '\0' || *str2 == '\0'))){
        str1 +=1;
        str2 +=1;
    }
    return ((*str1 == '\0') && (*str2 == '\0'));
}

/************************************************************************************
* Function: parseArg
* -  parses a string representing a decimal hex value, returning the number as an unsigned char
* - because any value can be good data, a pass-by-reference argument is needed for error reporting
* Arguments: 2
* argument 1: a null-terminated string containing the value
* argument 2: a pass-by-reference value to set if an error occurs
* returns: unsigned char corresponding to the value
* Author: Jamie Boyd
* Date: 2022/02/15
* ***********************************************************************************/
int parseArg (char * aToken, char * err){
    int tokLen, tokPos;
    unsigned char power, rChar =0;
    *err =0;
    for (tokLen = 0; *(aToken + tokLen) != '\0'; tokLen +=1){}; // find token length so we can work backwards
    if ((*aToken == '0') && (*(aToken + 1) == 'x')){ // we have hex digit, note the x is case sensitive
        for (power=1, tokPos = tokLen -1; tokPos >= 2; tokPos -=1, power *= 16){
            if ((*(aToken + tokPos) >= '0') && (*(aToken + tokPos) <= '9')){
                rChar += (*(aToken + tokPos) - 48) * power;
            }else{
                if ((*(aToken + tokPos) >= 'A') && (*(aToken + tokPos) <= 'F')){ // note case sensitive
                    rChar += (*(aToken + tokPos) - 55) * power;
                }else{
                    *err = 5;    // error parsing number
                    break;
                }
            }
        }
    } else{     // we have decimal values
        for (power=1, tokPos = tokLen-1; tokPos >= 0; tokPos -=1, power *= 10){
           if ((*(aToken + tokPos) >= '0') && (*(aToken + tokPos) <= '9')){
               rChar += (*(aToken + tokPos) - 48) * power;
           }else{
               *err =5; // error parsing number
               break;
           }
        }
    }
    return rChar;     // data will only be valid if *err is 0
}

/************************************************************************************
* Function: validateCmd
* - searches through command names in command list looking for a match
* Arguments: 2
* argument 1: cmdList - the list of commands with names
* argument 2: a name
* returns: index of command with mayching name, or -1 if no match was found
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
int validateCmd(CMD * cmdList ,char * cmdName) {
    char ii;
    int cmdIndex = -1;
    for (ii =0; ii < MAX_CMDS; ii +=1){
        if (strCmp(cmdName, (char *)cmdList[ii].name)){
            cmdIndex = ii;
            break;
        }
    }
    return cmdIndex;
}


/************************************************************************************
* Function: parseCmd
* - parses command line from user, filling out data in a CMD structure from CMD list
* Arguments: 2
* argument 1: cmdList - the list of commands with names
* argument 2: cmdLine
* returns: -1 of index of command, sets error flag
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
int parseCmd(CMD * cmdList, char * cmdLine){
    char * contextPtr = NULL;
    char * aToken = strTok (cmdLine, &contextPtr); // first token contains command name.
    unsigned char iArg;
    int argVal;
    char error;
    int cmdIndex = validateCmd(cmdList ,aToken);
    if (cmdIndex == -1){
        gError = 2;         // this command name does not exist
    }else{
        for (iArg =0; iArg < cmdList[cmdIndex].nArgs; iArg +=1){
            aToken = strTok (NULL, &contextPtr);
            if (aToken == NULL){
                cmdIndex = -1;
                gError =3;     // not enough arguments
                break;
            }else{
                argVal = parseArg (aToken, &error);
                if (error){
                    cmdIndex = -1;
                    gError = error;
                    break;
                }else{
                    cmdList[cmdIndex].args[iArg] = argVal;
                }
            }
        }
        if (!(gError)){
            aToken = strTok (NULL, &contextPtr); // check there was no more data entered
            if (aToken != NULL){
                cmdIndex = -1;
                gError = 4;                       // too many arguments
            }
        }
    }
    return cmdIndex;
}

/************************************************************************************
* Function: executeCmd
* - executes the command, setting clearing needed bits
* Arguments: 2
* argument 1: cmdList - the list of commands
* argument 2: cmdIndex - index of command to process, fields must filled out already
* returns: -1 of index of command, sets error flag
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
int executeCmd(CMD * cmdList, char cmdIndex){
    int rVal = 0;       // value to return
    unsigned char pNum, mask, dir, state, p3Byte;
    if ((cmdIndex ==0) || (cmdIndex ==1)){
        if ((cmdList[cmdIndex].args[0] < 1) || (cmdList[cmdIndex].args[0] > 6)){
            rVal = -1;
            gError = 6; // port number is bad
        }else{
            pNum = (unsigned char) (cmdList[cmdIndex].args[0]);
            if ((cmdList[cmdIndex].args[1] < 0) || (cmdList[cmdIndex].args[1] > 255)){
                rVal = -1;
                gError = 7; // argument out of range, mask is an unsigned byte
            }else{
                mask = (unsigned char) (cmdList[cmdIndex].args[1]);
            }
        }
    }
    if (rVal ==-1){
        return rVal;
    }
    switch (cmdIndex){
        case 0: // set up bits of a port for input or output pNum (port number), mask (mask for setting bits), dir (0 for input, 1 for output)
           if ((cmdList[cmdIndex].args[2] < 0) || (cmdList[cmdIndex].args[2] > 1)){
               rVal = -1;
               gError = 7; // argument out of range, mask is an unsigned byte
               break;
           }
           dir = (unsigned char)(cmdList[cmdIndex].args[2]);
           switch (pNum){
               case 1:
                   if (dir){ // setting bits, outputs
                       P1DIR |= mask;
                   }else{   // clearing bits
                       P1DIR &= ~mask;
                   }
                   break;
               case 2:
                   if (dir){ // setting bits, outputs
                       P2DIR |= mask;
                  }else{   // clearing bits
                      P2DIR &= ~mask;
                  }
                  break;
               case 3:
                   if (dir){ // setting bits, outputs
                       P3DIR |= mask;
                   }else{   // clearing bits
                     P3DIR &= ~mask;
                   }
                   break;
               case 4:
                   if (dir){ // setting bits, outputs
                      P4DIR |= mask;
                  }else{   // clearing bits
                    P4DIR &= ~mask;
                  }
                   break;
               case 5:
                   if (dir){ // setting bits, outputs
                      P5DIR |= mask;
                  }else{   // clearing bits
                    P5DIR &= ~mask;
                  }
                   break;
               case 6:
                   if (dir){ // setting bits, outputs
                       P6DIR |= mask;
                   }else{   // clearing bits
                       P6DIR &= ~mask;
                   }
                   break;
               }
           rVal = 0;
           break;

       case 1:  // sets, clears, or toggles  outputs
           if ((cmdList[cmdIndex].args[2] < 0) || (cmdList[cmdIndex].args[2] > 2)){
               rVal = -1;
               gError = 7; // argument out of range, mask is an unsigned byte
               break;
           }
           state = (unsigned char)(cmdList[cmdIndex].args[2]);
           switch (pNum){
              case 1:
               switch (state){
                   case 0:
                      P1OUT &= ~mask; // clears bits
                      break;
                   case 1:
                       P1OUT |= mask; // sets bits
                       break;
                   case 2:
                       P1OUT ^= mask; // toggle bits
                       break;
                   }
                   break;
           case 2:
               switch (state){
               case 0:
                  P2OUT &= ~mask; // clears bits
                  break;
               case 1:
                   P2OUT |= mask; // sets bits
                   break;
               case 2:
                   P2OUT ^= mask; // toggle bits
                   break;
               }
               break;
           case 3:
               switch (state){
               case 0:
                  P3OUT &= ~mask; // clears bits
                  break;
               case 1:
                   P3OUT |= mask; // sets bits
                   break;
               case 2:
                   P3OUT ^= mask; // toggle bits
                   break;
               }
               break;
           case 4:
               switch (state){
               case 0:
                  P4OUT &= ~mask; // clears bits
                  break;
               case 1:
                   P4OUT |= mask; // sets bits
                   break;
               case 2:
                   P4OUT ^= mask; // toggle bits
                   break;
               }
               break;
           case 5:
               switch (state){
               case 0:
                  P5OUT &= ~mask; // clears bits
                  break;
               case 1:
                   P5OUT |= mask; // sets bits
                   break;
               case 2:
                   P5OUT ^= mask; // toggle bits
                   break;
               }
               break;
           case 6:
               switch (state){
               case 0:
                  P6OUT &= ~mask; // clears bits
                  break;
               case 1:
                   P6OUT |= mask; // sets bits
                   break;
               case 2:
                   P6OUT ^= mask; // toggle bits
                   break;
               }
               break;
           }
           rVal = 1;
           break;

       case 2:
           if ((cmdList[cmdIndex].args[0] < 0) || (cmdList[cmdIndex].args[0] > 255)){
               rVal =7;
               break;
           }
           p3Byte = (unsigned char) (cmdList[cmdIndex].args[0]);
           P3OUT = p3Byte;
           rVal = 2;
           break;
    }
    return rVal;
}

/************************************************************************************
* Function: printErr
* - prints any needed error message to terminal Tx
* Arguments: None - accesses global variable of error code
* returns: Nothing
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
void printErr (){
    sprintf ((char *)msgStr,"\r\n--CMD %d %s --\r\n\0", gCmdCnt, gErrStrs [gError]); // err code matches index of err string
    usciA1UartTxString (msgStr);
    gError =0;
}
