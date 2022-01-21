/*
 * usciSpi.c
 *
 * create a proper file header for the C module
 */

#include <msp430.h>
#include "usciSpi.h"


// create a function header that describes the function and how to use it. Provide an example function call.
void usciB1SpiInit(unsigned char spiMST, unsigned int sclkDiv, unsigned char sclkMode, unsigned char spiLoopBack){

	UCB1CTL1 |= UCSWRST;                      	// **Put state machine in USCI reset while you intitialize it**

	
	// call usciB1SpiClkDiv

	// configure the control registers using the input arguments. See user manual, lecture notes and programmers model
	
	// configure the SPI B1 pins with PxSEL register


	UCB1CTL1 &= ~UCSWRST;                     	// **Initialize USCI state machine**  take it out of reset
}


// provide function header
// this function is complete. Understand what it is doing.  Call it when SCLKDIV needs to be changed in Lab.
void usciB1SpiClkDiv(unsigned int sclkDiv){

    UCB1CTL1 |= UCSWRST;                        // you always need to put state machine into reset when configuring USC module

    UCB1BR0 = (sclkDiv&0xFF);                   // 2
    UCB1BR1 = (sclkDiv>>8);                     //

    UCB1CTL1 &= ~UCSWRST;                       // **Initialize USCI state machine**
}


// when the TXBUFFER is ready load it.    txByte-->TXBUFFER
// provide a function header
void usciB1SpiPutChar(unsigned char txByte) {
	
	
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
