/*
 * portGPIO.c  An example using libCmdInterp to configure, write to, and read from GPIO ports on the msp430
 *
 *  Created on: Mar. 17, 2022
 *      Author: jamie
 */
#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>

#include "libCmdInterp.h"
#include "portGPIO.h"

unsigned char gPortCmdsErrOffset;

int main (void){
    gPortCmdsErrOffset = portInit ();


}



unsigned char portInit (void){
    libCMD_addCmd (PORTSETUP, 3, 0, &portSetUp);                    // add port commands
    libCMD_addCmd (PORTWRITEBITS, 3, 0, &portWriteBits);
    libCMD_addCmd (PORTWRITEBYTE, 2, 0, &portWriteByte);
    libCMD_addCmd (PORTREADBITS, 3, 0, &portReadBits);
    unsigned char portCmdsErrOffset = libCMD_addErr (PORT_ERR0);      // add errors for port commands
    libCMD_addErr (PORT_ERR1);
    libCMD_addErr (PORT_ERR2);
    libCMD_addErr (PORT_ERR3);
    libCMD_addErr (PORT_ERR4);
    libCMD_addErr (PORT_ERR5);
    return portCmdsErrOffset;
}

#ifdef f5529  // versions of functions with calculated offsets based on memory map of mspf5529 and other f series microcontrollers
unsigned char portSetUp (CMDdataPtr commandData){
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char dir;
    unsigned char mask;
    unsigned char * portPtr;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 0)&& (commandData->args[1] <= 1)){
            dir = (unsigned char)commandData->args[1];
            if ((commandData->args[2] > 0) && (commandData->args[1] <= 255)){
                mask = (unsigned char)commandData->args[2];
                portPtr = (unsigned char *) PORT_REG_ADDR(thePort, DIR_REG);
                if (dir){ // setting bits, outputs
                   *portPtr |= mask;
               }else{   // clearing bits, for inputs
                   *portPtr &= ~mask;
               }
           } else {
               rVal = gPortCmdsErrOffset + 2;
           }
       } else{
           rVal = gPortCmdsErrOffset + 1;
       }
    }else{
        rVal = gPortCmdsErrOffset + 0;
    }
    return rVal;
}

unsigned char portWriteBits (CMDdataPtr commandData){ // writes to selected bits of a port. 0:port number), 1:mode (0 for clear, 1 for set. 2 for toggle), 2:mask
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char mode;
    unsigned char mask;
    unsigned char * portPtr;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 1)&& (commandData->args[1] <= 2)){
            mode = (unsigned char)commandData->args[1];
            if ((commandData->args[2] >= 1) && (commandData->args[2] <= 255)){
                mask =  (unsigned char)commandData->args[2];
                portPtr = (unsigned char *)PORT_REG_ADDR (thePort, OUTPUT_REG);
                switch (mode){
                   case 0:
                      *portPtr &= ~mask; // clears bits
                      break;
                   case 1:
                       *portPtr |= mask; // sets bits
                       break;
                   case 2:
                       *portPtr ^= mask; // toggle bits
                       break;
                   }
            }else{  // bad mask
                rVal = gPortCmdsErrOffset + 2;
            }
        }else{      // bad mode
            rVal = gPortCmdsErrOffset + 3;
        }
    }else{  // bad port
        rVal = gPortCmdsErrOffset + 0;
    }
    return rVal;
}

unsigned char portWriteByte (CMDdataPtr commandData){ // writes a byte to a port. 0:port number, 1:byte to write
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char theByte;
    unsigned char * portPtr;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 0) && (commandData->args[1] <= 255)){
            theByte = (unsigned char) commandData->args[1];
            portPtr = (unsigned char *)PORT_REG_ADDR (thePort, OUTPUT_REG);
            *portPtr = theByte;
        } else {  // bad data byte
            rVal = gPortCmdsErrOffset + 4;
        }
    } else{  // bad port
        rVal = gPortCmdsErrOffset + 0;
    }
    return rVal;
}

unsigned char portReadBits (CMDdataPtr commandData){ // reads selected bits from a port. 0:port number 1:mask 2:type (0 for binary, 1 for text)
    static char rBuffer[40];
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char type;
    unsigned char mask;
    unsigned char portVal;
    unsigned char * portPtr;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 1) && (commandData->args[1] <= 255)){
            mask = (unsigned char)commandData->args[1];
            if ((commandData->args[2] >= 0) && (commandData->args[2] <= 1)){
                type = (unsigned char)commandData->args[2];
                portPtr = (unsigned char *)PORT_REG_ADDR (thePort, INPUT_REG);
                portVal = (*portPtr & mask);
                if(type ==0){
                  while (!(UCA1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
                  UCA1TXBUF = portVal;
                }else{
                    sprintf ((char *)rBuffer,"P%dIN = %d\0", thePort,portVal); // display first prompt for user
                    usciA1UartTxString (rBuffer);
                }
                while (!(UCA1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
                    UCA1TXBUF = '\r';
            }else{      // bad read type
                rVal =gPortCmdsErrOffset + 5;
            }
          }else{    // bad mask
              rVal =gPortCmdsErrOffset + 2;
          }
    }else{  // bad port
        rVal = gPortCmdsErrOffset + 0;
    }
    return rVal;
}
#endif



#ifndef f5529   // versions of functions that will work on other msp430 micro-controllers
//set up bits of a port for input or output 0:port number), 1:dir (0 for input, 1 for output), 2:mask
unsigned char portSetup (CMDdataPtr commandData){
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char dir;
    unsigned char mask;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 0)&& (commandData->args[1] <= 1)){
            dir = (unsigned char)commandData->args[1];
            if ((commandData->args[2] >= 1) && (commandData->args[2] <= 255)){
                mask =  (unsigned char)commandData->args[2];
                switch (thePort){
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
                  case 7:
                    if (dir){ // setting bits, outputs
                        P7DIR |= mask;
                    }else{   // clearing bits
                        P7DIR &= ~mask;
                    }
                    break;
                  case 8:
                      if (dir){ // setting bits, outputs
                          P8DIR |= mask;
                      }else{   // clearing bits
                          P8DIR &= ~mask;
                      }
                      break;
                }
           }else{
               rVal = gPortCmdsErrOffset + 2;
           }
       }else {
           rVal = gPortCmdsErrOffset + 1;
       }
    }else{  // bad port
       rVal = gPortCmdsErrOffset + 0;
    }
    return rVal;
}


unsigned char portWriteBits (CMDdataPtr commandData){ // writes to selected bits of a port. 0:port number), 1:mode (0 for clear, 1 for set. 2 for toggle), 2:mask
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char mode;
    unsigned char theMask;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 0) && (commandData->args[1] <= 2)){
            mode = (unsigned char)commandData->args[1];
            if ((commandData->args[2] >= 1) && (commandData->args[2]<= 255)){
                mask =  (unsigned char)commandData->args[2];
                switch (thePort){
                   case 1:
                       switch (mode){
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
                       switch (mode){
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
                       switch (mode){
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
                       switch (mode){
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
                       switch (mode){
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
                       switch (mode){
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
                       case 7:
                          switch (mode){
                              case 0:
                                 P7OUT &= ~mask; // clears bits
                                 break;
                              case 1:
                                  P7OUT |= mask; // sets bits
                                  break;
                              case 2:
                                  P7OUT ^= mask; // toggle bits
                                  break;
                              }
                              break;
                          case 8:
                            switch (mode){
                                case 0:
                                   P8OUT &= ~mask; // clears bits
                                   break;
                                case 1:
                                    P8OUT |= mask; // sets bits
                                    break;
                                case 2:
                                    P8OUT ^= mask; // toggle bits
                                    break;
                                }
                                break;
                }
            }else{ //bad mask
                rVal = gPortCmdsErrOffset + 2;
            }
       }else{ // bad mode
           rVal = gPortCmdsErrOffset + 3;
       }
    }else{ // bad port
        rVal = gPortCmdsErrOffset + 0;
    }
    return rVal;
}

unsigned char portWriteByte (CMDdataPtr commandData){ // writes a byte to a port. 0:port number, 1:byte to write
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char theByte;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 0) && (commandData->args[1] <= 255)){
            theByte = (unsigned char) commandData->args[1];
            switch (thePort){
               case 1:
                   P1OUT = theByte;
                   break
               case 2:
                   P2OUT = theByte;
                   break
               case 3:
                   P3OUT = theByte;
                   break;
               case 4:
                   P4OUT = theByte;
                   break;
               case 5:
                   P5OUT = theByte;
                   break;
               case 6:
                   P6OUT = theByte;
                   break;
               case 7:
                  P7OUT = theByte;
                  break;
               case 7:
                 P7OUT = theByte;
                 break;
          }
        } else { // bad data byte
            rVal =gPortCmdsErrOffset + 4;
       }
    }else{  // bad port
        rVal = gPortCmdsErrOffset + 0;
    }
    return rVal;
}

unsigned char portReadBits (CMDdataPtr commandData){ // reads selected bits from a port. 0:port number 1:mask 2:type (0 for binary, 1 for text)
    unsigned char rVal = 0;  // for success
    unsigned char thePort;
    unsigned char type;
    unsigned char mask;
    unsigned char portVal;
    if ((commandData->args[0] >= 1) && (commandData->args[0] <= 8)){
        thePort = (unsigned char)commandData->args[0];
        if ((commandData->args[1] >= 1) && (commandData->args[1] <= 255)){
            mask = (unsigned char)commandData->args[1];
            if ((commandData->args[2] >= 0) || (commandData->args[2] <= 1)){
                type = (unsigned char)commandData->args[2];
                switch (thePort){
                   case 1:
                       portVal = (P1IN & mask);
                       break;
                   case 2:
                       portVal = (P2IN & mask);
                       break;
                   case 3:
                       portVal = (P3IN & mask);
                       break;
                   case 4:
                       portVal = (P4IN & mask);
                       break
                   case 5:
                       portVal = (P5IN & mask);
                       break;
                   case 6:
                       portVal = (P6IN & mask);
                       break;
                }
                if(type ==0){
                    while (!(UCA1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
                    UCA1TXBUF = portVal;
                }else{
                    sprintf ((char *)rBuffer,"P%dIN = %d\0", thePort,portVal); // text version
                    usciA1UartTxString (rBuffer);
                }
                while (!(UCA1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
                    UCA1TXBUF = '\r';
          }else{ // bad print type
              rVal =gPortCmdsErrOffset + 5;
          }
       }else{  //bad mask
           rVal =gPortCmdsErrOffset + 2;
       }
    }else{  // bad port
        rVal = gPortCmdsErrOffset;
    }
    return rVal;
}
#endif
