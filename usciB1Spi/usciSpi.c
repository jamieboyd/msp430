/*
 * usciSpi.c
 *
 * create a proper file header for the C module
 */

#include <msp430.h>
#include "usciSpi.h"




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
	UCB1CTL0 |= UCMSB;                          // most significant bit shifts first
	UCB1CTL0 &= ~UC7BIT;                        // 8 bit characters
	if (spiMST){                                // set up to be SPI master or slave
	    UCB1CTL0 |= UCMST;
	}else{
	    UCB1CTL0 &= ~UCMST;
	}
	UCB1CTL0 &= ~(UCMODE_3);                 // 3 pin SPI - no enable signal for slave, use with only 1 slave
	UCB1CTL0 |= UCSYNC;                      //  Synchronous mode enabled

	// configure the SPI B1 pins with PxSEL register to select peripherals
	P4SEL |= (SPI_MISO | SPI_MOSI | SPI_CLK);
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


// when the TXBUFFER is ready load it.    txByte-->TXBUFFER
// provide a function header
void usciB1SpiPutChar(unsigned char txByte) {
    while (!(UCB1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
        UCB1TXBUF =txByte;
}



#pragma vector=USCI_B1_VECTOR
__interrupt void usciB1SpiIsr(void) {
	
// UCB1IV interrupt handler. __even_in_range will optimize the C code so efficient jumps are implemented.
  switch(__even_in_range(UCB1IV,4)) // this will clear the current highest priority flag. TXIFG or RXIFG.
  {
  	  case 0: break;                          	// Vector 0 - no interrupt
  	  case 2:                                 	// Vector 2 - RXIFG. Highest priority
		// process RXIFG
  		
  		  break;

  	  case 4:									// Vector 4 - TXIFG
  		
		// process TXIFG (careful)
		
  		  break;

  	  default: break; 
  }
}
