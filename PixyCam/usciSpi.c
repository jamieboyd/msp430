/*************************************************************************************************
 * usciSpi.c
 * - C implementation or source file for MSP430 usci SPI B1
 * Has functions for sending/receiving from strings (null terminated character arrays)
 * and buffers (character arrays with specified lengths)
 *
 *  Author: Jamie Boyd
 *  Created on:2022/01/21
 **************************************************************************************************/

#include "usciSpi.h"

char uartRxBuffer [RX_BUF_SZ]; // buffer that receive data from usciA1UARTgets. referenced in header so can be accessed easily
unsigned char spiTxBuffer [RX_BUF_SZ];
unsigned char spiRxBuffer [100];
volatile unsigned char spiTxPos =0; // used in interrupt, so make them volatile
volatile unsigned char spiRxPos=0;


/*************************** Function:uartToSPI ***************************************
 * - interrupt function for received serial data - echoes input and then transmits on SPI the byte received on serial
 * Arguments: 1
 * argument 1: the character that was just received on the UART
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/01/34 */
void uartToSPI (char RXBUF){
    while (!(UCA1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
            UCA1TXBUF =RXBUF;
    usciB1SpiPutChar (RXBUF);
}

/*************************** Function: usciB1SpiInit ******************************************************
* - configures USI B1 when used as an SPI controller
* Arguments: 3
* argument 1: spiMST – if 0 the USCIB1 is configured as a slave, otherwise it is a master.
* argument 2: sclkDIV – defines the 16 bit clock divisor used to divide 2^20 Hz SMCLK, which is used as the clock source
* argument 3: sclkMode – clock mode. bit 0 is clock polarity, bit 1 is phase. 0-3 are valid arguments
* return: nothing
* Sample use: usciB1SpiInit(1, 48, 1, 1) //
* Author: Jamie Boyd
* Date: 2022/01/23
* *********************************************************************************************************/
void usciB1SpiInit(unsigned char spiMST, unsigned int sclkDiv, unsigned char sclkMode, unsigned char spiLoopBack){
    UCB1CTL1 |= UCSWRST;                     // you always need to put state machine into reset when configuring USC module
    if (spiMST){
        UCB1CTL1 |= UCSSEL1;                    // set SMCLCK as source for making SPI clock
        usciB1SpiClkDiv(sclkDiv, 0);               // call usciB1SpiClkDiv to divide down SM clk
        if (sclkMode & SPI_READS_FIRST){        // set clock phase bit
            UCB1CTL0 |= UCCKPH;
        }else{
            UCB1CTL0 &= ~UCCKPH;
        }
        if (sclkMode & SPI_CLK_IDLES_HIGH){     // set clock polarity bit
            UCB1CTL0 |= UCCKPL;
        }else{
            UCB1CTL0 &= ~UCCKPL;
        }
    }
	UCB1CTL0 |= UCMSB;                          // most significant bit shifts first
	UCB1CTL0 &= ~UC7BIT;                        // 8 bit characters
	if (spiMST){                                // set up to be SPI master or slave
	    UCB1CTL0 |= UCMST;
	}else{
	    UCB1CTL0 &= ~UCMST;
	}
	UCB1CTL0 &= ~(UCMODE_3);                 // 3 pin SPI - no enable signal for slave, use with only 1 slave
	UCB1CTL0 |= UCSYNC;                      //  Synchronous mode enabled, no UART here

	// configure the SPI B1 pins with PxSEL register to select peripherals
	P4SEL |= (SPI_MISO | SPI_MOSI | SPI_CLK);
	//P4SEL |= ( SPI_MOSI | SPI_CLK);
	// configure a SPI /SS line on P4.0
	P4DIR |= BIT0;
	P4OUT |= BIT0; // idles high, asserts low
	UCB1CTL1 &= ~UCSWRST;                     	// **Initialize USCI state machine**  take it out of reset
}


/************************ Function: usciB1SpiClkDiv *************************************
* - divides down smclk 2^20 Hz, about 1.04 MHz, to make clock for SPI
* Arguments:2
* argument 1: sclkDiv – 16 bit value for clock divisor
* argument 2: doReset - pass 1 to put USCI into reset before and take it out afterwards. 0 if done as part of init which does the reset
* return: nothing
* Author: Jamie Boyd
* Date: 2022/01/23
* ****************************************************************************************/
void usciB1SpiClkDiv(unsigned int sclkDiv, unsigned char doReset){
    if (doReset) UCB1CTL1 |= UCSWRST;            // you always need to put state machine into reset when configuring USC module
    // SCLKDIV is a 16 bit value, it gets spread out over 2 8-bit control registers
    UCB1BR0 = (sclkDiv & 0xFF);                 // UCB1BR0 is the low byte. ANDing with 255 removes the high byte
    UCB1BR1 = (sclkDiv >> 8);                   // UCB1BR0 is the high byte. right shifting 8 bits replaces low bye with high byte
    if (doReset) UCB1CTL1 &= ~UCSWRST;          // **Initialize USCI state machine**
}


/**************************** Function: numStringToInt **************************
 * gets numbers from a a string buffer containing ASCII codes for numbers 0-9
 * Arguments:2
 * argument 1: null terminated string containing only ASCII characters for numbers 0-9
 * argument 2: a buffer, must be at least as big as the string, that gets integers corresponding to the ASCII codes
 * returns: length of rxStr, or 0 if "dirty" non-numeric characters were found
 *  Author: Jamie Boyd
 *  Date: 2022/01/23 */
unsigned char numStringToInt (unsigned char * rxStr, unsigned char * txBuff){
    unsigned char rVal;
    for (rVal =0; rxStr [rVal] != '\0'; rVal +=1){
        if ((rxStr [rVal] >= '0') && (rxStr [rVal] <= '9')){
            txBuff [rVal] = rxStr [rVal] - '0';
        }else{
            rVal =0;
            break;
        }
    }
    return rVal;
}

/************************ Function: usciB1SpiTXBuffer *************************************
-
*/
void usciB1SpiTXBuffer (const unsigned char * buffer, int bufLen){
    unsigned char iBuf;
    P4OUT &= ~BIT0;
    for (iBuf =0; iBuf < bufLen; iBuf +=1){
        usciB1SpiPutChar(buffer[iBuf]);
    }
    while (!(UCB1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send. THis means last byte has been moved into shift register
    UCB1IFG &= ~UCRXIFG;              // Clear receive flag cause it will be set because we never read RXBUF
    while (!(UCB1IFG & UCRXIFG)){};   // poll, waiting while last bit in last byte to be received. Now transfer is complete
    UCB1IFG &= ~UCRXIFG;              // Always a good idea to clear the flag
    P4OUT |= BIT0;                       // un-assert
}

// when the TXBUFFER is ready load it.    txByte-->TXBUFFER
// provide a function header
void usciB1SpiPutChar(unsigned char txByte) {
    while (!(UCB1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
    UCB1TXBUF =txByte;  // write the byte to the TX buffer register
}

#pragma vector=USCI_B1_VECTOR
__interrupt void usciB1SpiIsr(void) {
	
// UCB1IV interrupt handler. __even_in_range will optimize the C code so efficient jumps are implemented.
  switch(__even_in_range(UCB1IV,4)) // this will clear the current highest priority flag. TXIFG or RXIFG.
  {
  	  case 0: break;                          	// Vector 0 - no interrupt
  	  case 2:                                 	// Vector 2 - RXIFG. Highest priority
		// process RXIFG
  	    spiRxBuffer [spiRxPos++] = UCB1RXBUF;
  	    if (spiRxPos == RX_BUF_SZ){
  	      UCB1IE &= ~UCRXIE;
  	    }
  		  break;
  	  case 4:									// Vector 4 - TXIFG
		// process TXIFG (careful)
  	    UCB1TXBUF = spiTxBuffer [spiTxPos++];
  	    if (spiRxPos ==RX_BUF_SZ){
  	        UCB1IE &= ~UCTXIE;
  	    }
  		  break;
  	  default: break; 
  }
}
