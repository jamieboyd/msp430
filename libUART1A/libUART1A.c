/*************************************************************************************************
 * libUART1A.c
 * - C implementation or source file for MSP430 usci UART A1
 * Has functions for sending/receiving from strings (null terminated character arrays)
 * and buffers (character arrays with specified lengths)
 *
 *  Author: Greg Scutt
 *  Created on: March 1, 2017
 *  Modified: February 26th, 2018
 *  Modified: 2022/01/13 by Jamie Boyd
 **************************************************************************************************/

#include <msp430.h>
#include "libUART1A.h"

char rxBuffer [RX_BUF_SZ]; // buffer that receive data from usciA1UARTgets. referenced in header so can be accessed easily
void (*rxIntFuncPtr)(char) = NULL; // pointer to function to run to get a byte from RXBUFF
char (*txIntFuncPtr)(void) = NULL; // pointer to function to run to transfer a byte into TXBUFF


/************************************************************************************
* Function: usciA1UartInit
* - configures UCA1 UART to use SMCLK, no parity, 8 bit data, LSB first, one stop bit
* - assumes SMCLK = 2^20 Hz
* Arguments: 1
* argument 1: Baud, an msp430 supported baud, 16x over-sampling is used if supported for the Baud
* return: 1 if a supported Baud was requested, else 0
* Author: Greg Scutt
* Date: March 1st, 2017
* Modified: 2022/01/10 by Jamie Boyd
************************************************************************************/
int usciA1UartInit(unsigned int Baud){
    _UART_A1PSEL;                   // // macro selects special functions (TXD and RXD) for P4 pins 4 and 5 which connect to TXD, RXD jumpers
    UCA1CTL1 |= UCSWRST;            // Sets USCI A1  Software Reset Enabled bit in USC A1 CTL1 register.

    UCA1CTL1    |=  UCSSEL_2;       // sets bit 7 - selects SMCLK for BRCLK. User is responsible for setting this rate. 1.0485 MHz
    UCA1CTL1    &=  ~UCRXEIE      // clears bit 5 no erroneous char interrupt
                & ~UCBRKIE        // no break character interrupts
                &  ~UCDORM         // not dormant
                &  ~UCTXADDR       // just data, no addresses
                &  ~UCTXBRK;       // not a break

    UCA1CTL0     =  0;              // RESET UCA1CTL0 before new configuration
    UCA1CTL0    &=  ~UCPEN          // bit 7 clear means No Parity
                &   ~UCMSB          // bit 5 clear means LSB First
                &   ~UC7BIT         // bit 4 clear means 8 bits of data, not 7
                &   ~UCSPB          // bit 3 clear means 1 stop bit, not 2
                &   ~(UCMODE0 | UCMODE1) // bits 1 and 2 clear mean UART Mode
                &   ~UCSYNC;        // bit 0 clear means asynchronous mode
    int BaudOK = 1;
    switch (Baud){
        case 9600:  // UCBR = 6, UCBRS =0, UCBRF=13, can use 16x-over-sampling
            UCA1BR0 = 6; // low byte of UCBR clock pre-scaler
            UCA1BR1 = 0;  // high byte of UCBR clock pre-scaler
            UCA1MCTL = UCBRS_0 | UCBRF_13 | UCOS16;  // sets first and second clock modulators and 16X over-sampling
            break;
        case 19200: // UCBR = 3, UCBRS =1, UCBRF=6, can use 16x-over-sampling
            UCA1BR0 = 3; // low byte of UCBR clock pre-scaler
            UCA1BR1 = 0;  // high byte of UCBR clock pre-scaler
            UCA1MCTL = UCBRS_1 | UCBRF_6| UCOS16;  // sets first and second clock modulators and 16X over-sampling
            break;
        case 38400:     // UCBR = 27, UCBRS = 2, UCBRF = 0, 16x-over-sampling not available
            UCA1BR0 = 27;
            UCA1BR1 = 0;
            UCA1MCTL = UCBRS_2 | UCBRF_0; // sets first and second clock modulators and NO 16x over-sampling
            break;
        case 57600: // UCBR = 17, UCBRS = 3, UCBRF = 0, 16x-over-sampling not available
            UCA1BR0 = 17;
            UCA1BR1 = 0;
            UCA1MCTL = UCBRS_3 | UCBRF_0; // sets first and second clock modulators and NO 16x over-sampling
            break;
        case 11520: //  UCBR = 9, UCBRS = 1, UCBRF = 0, 16x-over-sampling not available
            UCA1BR0 = 9;
            UCA1BR1 = 0;
            UCA1MCTL = UCBRS_1 | UCBRF_0; // sets first and second clock modulators and NO 16x over-sampling
            break;
        default:    // a non-supported Baud was requested, not OK
            BaudOK = 0;
            break;
    }
    UCA1CTL1 &= ~UCSWRST;        //  configured. take state machine out of reset.
    return BaudOK;
}

/************************************************************************************
* Function: usciA1UartTxChar
* - writes a single character to UCA1TXBUF, first waiting until the write register UCA1TXBUF is empty
* Arguments:1
* argument1: txChar - byte to be transmitted
* return: none
* Author: Greg Scutt
* Date: March 1st, 2017
* Modified: 2022/01/10 by Jamie Boyd
************************************************************************************/
void usciA1UartTxChar(char txChar) {

    while (!(UCA1IFG & UCTXIFG)); // is this efficient ? No, it is polling, could use interrupt
        UCA1TXBUF = txChar;  // if TXBUFF ready then transmit a byte by writing to it
}

/************************************************************************************
* Function: usciA1UartTxString
* - writes a C string of characters, one char at a time to UCA1TXBUF by calling
*   usciA1UartTxChar. Stops when it encounters  the NULL character in the string
*   does NOT transmit the NULL character
* argument:
* Arguments:1
* argument1: txChar - pointer to char (string) to be transmitted
* return: number of characters transmitted
* Author: Greg Scutt
* Date: March 1st, 2017
* Modified: 2022/01/10 by Jamie Boyd
************************************************************************************/
int usciA1UartTxString(char* txChar){
    char* txCharLocal = txChar;       // make a local copy of txChar pointer
    while (*txCharLocal != '\0'){                 // pre-test for the terminating NULL, it is not transmitted
        usciA1UartTxChar (*txCharLocal);       // transmit one character at a time
        txCharLocal +=1;                       // increment local pointer
    }
    return (txCharLocal - txChar);             // returns number of characters sent
}

/************************************************************************************
* Function: usciA1UartTxBuffer
* - transmits bufLen characters from a text buffer
* Arguments:2
* argument1: buffer - unsigned char pointer to text buffer to be transmitted
* argument2: bufLen - integer number of characters transmitted
* return: number of bytes transmitted
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
int usciA1UartTxBuffer (char * buffer, unsigned int bufLen){
    unsigned int ii =0;
    for (ii =0; ii < bufLen; ii +=1){
        usciA1UartTxChar (buffer[ii]);
    }
    return ii;
}

/************************************************************************************
* Function: usciA1UartGets
* - receive a string entered from the console and store it into an array pointed to by rxString.
* Arguments:1
* argument1: rxString - unsigned char pointer to text buffer to put received characters into
* return:  pointer to rxString or NULL if unsuccessful (too many characters entered)
* Author: Jamie Boyd
* Date: 2022/02/10
************************************************************************************/
char * usciA1UartGets (char * rxString){
    unsigned char count;
    unsigned char rxVal;
    for (count =0; count < RX_BUF_SZ; count+=1){
        while (!(UCA1IFG & UCRXIFG));   // poll, waiting for a RX character to be ready
        rxVal = UCA1RXBUF;
        if (rxVal == '\r'){           // return was received
            rxBuffer [count] = '\0'; // add NULL termination
            break;
        }else{
            rxBuffer [count] = rxVal;
            usciA1UartTxChar (rxVal);   // echo entered character back to sender
        }
    }
    char * OutputStr;
    if (count < RX_BUF_SZ){     // we did not overflow buffer
        OutputStr = rxString;
        for (count +=1; count > 0; count -= 1){ // offset needed to avoid useless comparison with 0. Thanks compiler for flagging that
            OutputStr [count-1] = rxBuffer[count-1];
        }
    }else{
        OutputStr = NULL;
    }
    return OutputStr;
}

/************************************************************************************
* Function: usciA1UartInstallRxInt
* - saves a global pointer to a function to be run when a character has been received.
* Arguments:1
* argument1: interuptFuncPtr - pointer to a function that has a single char argument
* returns:nothing
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
void usciA1UartInstallRxInt (void(*interuptFuncPtr)(char  RXBUF)){
    rxIntFuncPtr = interuptFuncPtr;
}

/************************************************************************************
* Function: usciA1UartEnableRxInt
* - enables or disables interupts for character in Rx buffer
* Arguments:1
* argument1:isOnNotOFF - non-zero enables, 0 disables
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
void usciA1UartEnableRxInt (char isOnNotOFF){
    if (isOnNotOFF){
        UCA1IE |= UCRXIE;           // set receive enable bit in UART1 interrupt enable register.
    }else{
        UCA1IE &= ~UCRXIE;          // clear receive enable bit in UART1 interrupt enable register.
    }
}

/************************************************************************************
* Function: usciA1UartInstallTxInt
* - saves a global pointer to a function to be run when a character can be transmitter.
* Arguments:1
* argument1: interuptFuncPtr - pointer to a function that returns a single char
* returns: unsigned char that will be put in the Tx buffer
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
void usciA1UartInstallTxInt (char(*interuptFuncPtr)(void)){
    txIntFuncPtr = interuptFuncPtr;
}

/************************************************************************************
* Function: usciA1UartEnableTxInt
* - enables or disables interrupts for Tx buffer ready for a character
* Arguments:1
* argument:isOnNotOFF - non-zero enables, 0 disables
* returns: nothing
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
void usciA1UartEnableTxInt (char isOnNotOFF){
    if (isOnNotOFF){
        UCA1IE |= UCTXIE;           // set transmit enable bit in UART1 interrupt enable register.
    }else{
        UCA1IE &= ~UCTXIE;           // clear transmit enable bit in UART1 interrupt enable register.
    }
}

/************************************************************************************
* Function: USCI_A1_ISR
* - Interrupt function for USCIA1 vector. Calls functions installed by usciA1UartInstallTxInt
* or usciA1UartInstallRxInt. So you had better install the function before enabling
* the corresponding interrupt
* Arguments:none
* returns: nothing
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
#pragma vector = USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void) {
  switch(__even_in_range(UCA1IV,4))
  {
  case 0:break;
  case 2:   // UCRXIFG - run installed function to deal with received character
      (*rxIntFuncPtr)(UCA1RXBUF);
    break;
  case 4:   //UCTXIFG - transmit character returned from installed function
      UCA1TXBUF =(*txIntFuncPtr)();
      break;
  default: break;
  }
}

/************************************************************************************
* Function: echoInterrupt
* a simple Rx interrupt function that can be installed by usciA1UartInstallRxInt
* It echoes received characters back to host
* Arguments:1
* argument 1: the character in the Rx buffer
* returns: nothing
* Author: Jamie Boyd
* Date: 2022/02/13
************************************************************************************/
void echoInterrupt (char  RXBUF){
    while (!(UCA1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
    UCA1TXBUF = RXBUF;
}
