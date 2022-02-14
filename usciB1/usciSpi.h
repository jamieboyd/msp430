/*************************************************************************************************
 * usciSpi.h
 * - header file for MSP430 usci SPI B1
 * Has functions for sending/receiving from strings (null terminated character arrays)
 * and buffers (character arrays with specified lengths)
 *
 *  Author: Jamie Boyd
 *  Created on:2022/01/23
 **************************************************************************************************/


#ifndef USCISPI_H_
#define USCISPI_H_

#include <msp430.h>
#include <libCmdInterp.h>      // Command Interpreter code, made into a library. Uses libUART1A.h
#include <libUART1A.h>         // coe for UART on USCI 1A, made into a library
#include <stdio.h>// used ONLY for sprintf function

/************************* SPI Clock Modes *********************************
*   Mode    UCCKPH      UCCKPL      Description from perspective of Master
*   0       1           0           clock idles low. Data is read (shifted in from MISO) on rising edge and written (appears on MOSI) on falling edge
*   1       0           0           clock idles low. Data is written (appears on MOSI) on rising edge and read (shifted in from MISO) on falling edge
*   2       1           1           clock idles high. Data is written (appears on MOSI) on falling edge and read (shifted in from MISO) on rising edge
*   3       0           1           clock idles high. Data is read (shifted in from MISO) on falling edge and written (appears on MOSI) on rising edge
****************************************************************************/


/***************************************** Output pins for SPI USCI 1B *************************
 *
 *      P4.1 UCB1 MOSI
 *      P4.2 UCB1 MISO
 *      P4.3 UCB1 clock
 ************************************************************************************/


/******************************* defines **********************************/

#define     SPI_READS_FIRST         1      // used to set clock phase bit in control register.
#define     SPI_WRITES_FIRST        0      // used to set clock phase bit in control register.
#define     SPI_CLK_IDLES_HIGH      2      // used to set clock polarity select bit in control register
#define     SPI_CLK_IDLES_LOW       0      // used to set clock polarity select bit in control register
#define     SPI_MISO                2      // BIT 1
#define     SPI_MOSI                4       // BIT 2
#define     SPI_CLK                 8       // BIT 3

// buffers that receive/send data from UART or SPI. referenced in header so can be accessed easily
extern char uartRxBuffer [];
extern unsigned char spiTxBuffer [];
extern unsigned char spiRxBuffer [];

// define which part of the lab to compile and run
#define  SPI_QUEST              // a quest to determine timing of first bit on MOSI with UCCKPHUCCKPL 1,0
#undef   SECTION_521             // SPI transmit characters using interrupt
#undef   SECTION_52x             // transmit chain simple (5.22) or with /SS (5.23, or with loopback (5.24)
#undef   SECTION_523             // 5.23 transmit chain with /SS, define SECTION_52X as well
#undef   SECTION_524             // 5.24 now with loopback.  define SECTION_52X as well


/******************************* Function Headers **********************************/
/* Function: usciB1SpiInit
* - configures USI B1 when used as an SPI controller. NOTE: this code does not do slave select - for that you are on your own
* Arguments: 3
* argument 1: spiMST – if 0 the USCIB1 is configured as a slave, otherwise it is a master.
* argument 2: sclkDIV – defines the 16 bit clock divisor used to divide 2^20 Hz SMCLK, which is used as the clock source
* argument 3: sclkMode – clock mode. bit 0 is clock polarity, bit 1 is phase. 0-3 are valid arguments
* return: nothing
* Sample use: usciB1SpiInit(1, 48, 1, 0)
* Author: Jamie Boyd
* Date: 2022/01/23 */
void usciB1SpiInit(unsigned char spiMST, unsigned int sclkDiv, unsigned char sclkMode, unsigned char spiLoopBack);

/* Function: usciB1SpiClkDiv
* - divides down smclk 2^20 Hz, about 1.04 MHz, to make clock for SPI
* Arguments:2
* argument 1: sclkDiv – 16 bit value for clock divisor
* argument 2: doReset - pass 1 to put USCI into reset before and take it out afterwards. 0 if done as part of init which does the reset
* return: nothing
* Author: Jamie Boyd
* Date: 2022/01/23 */
void usciB1SpiClkDiv(unsigned int sclkDiv, unsigned char doReset);

void usciB1SpiPutChar(unsigned char txByte);

void usciB1SpiTXBuffer (const unsigned char * buffer, int bufLen);

/*************************** Function:uartToSPI ***************************************
 * - interrupt function for received serial data - immediately transmits on SPI the byte received on serial
 * Arguments: 1
 * argument 1: the character that was just received on the UART
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/01/23 */
void uartToSPI (char RXBUF);

/**************************** Function: strLen ********************************
 * returns the length of a null-terminated string
 * Arguments: 1
 * argument 1: the string
 * returns: length of the stirng
 * Author: Jamie Boyd
 * Date: 2022/01/23 */
unsigned char strLen (char * strBuffer);

/**************************** Function: numStringToInt **************************
 * gets numbers from a a string buffer containing ASCII codes for numbers 0-9
 * Arguments:2
 * argument 1: null terminated string containing only ASCII characters for numbers 0-9
 * argument 2: a buffer, must be at least as big as the string, that gets integers corresponding to the ASCII codes
 * returns: length of rxStr, or 0 if "dirty" non-numeric characters were found
 *  Author: Jamie Boyd
 *  Date: 2022/01/23 */
unsigned char  numStringToInt (unsigned char * rxStr, unsigned char * txBuff);


#endif /* USCISPI_H_ */
