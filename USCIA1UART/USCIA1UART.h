/*************************************************************************************************
 * USCIA1UART.h
 * - - C interface file for MSP430 usci UART A1, A0
 *
 *  Author: Greg Scutt
 *  Created on: March 1, 2017
 *  Modified:2022/01/13 by Jamie Boyd
 **************************************************************************************************/


#ifndef USCIA1UART_H_
#define USCIA1UART_H_

#include <stdint.h>

#define     TXD_A1          BIT4                        // A1 UART Transmits Data on P4.4
#define     RXD_A1          BIT5                        // A1 UART Receives Data on P4.5
#define     _UART_A1PSEL    P4SEL |= TXD_A1 | RXD_A1    // example of using macros for short expressions.

#define     RX_BUF_SZ       50     // size for buffer to receive characters from terminal

#ifndef NULL
#define NULL 0
#endif

/********************************** Function Headers ********************************************/
int usciA1UartInit(unsigned int Baud);
void usciA1UartTxChar(char txChar);
int usciA1UartTxString(char* txChar);
int usciA1UartTxBuffer (char * buffer, unsigned int bufLen);
char * usciA1UartGets (char * rxString);
void usciA1UartInstallRxInt (void(*interuptFuncPtr)(char RXBUF));
void usciA1UartEnableRxInt (char isOnNotOFF);
void usciA1UartInstallTxInt (char(*interuptFuncPtr)(void));
void usciA1UartEnableTxInt (char isOnNotOFF);
void echoInterrupt (char  RXBUF);

#endif /* USCIA1UART_H_ */
