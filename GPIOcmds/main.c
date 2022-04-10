#include <msp430.h> 
#include "portGPIO.h"

/**
 * main function for PortGPIO. Initializes command interpreter, adds its commands and error messages, and
 * everything else is done with interrupts.
 *
 */
int main(void) {
 	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	if (libCMD_init () ==0){
	    gPortCmdsErrOffset = portInit ();
	    libCMD_run ();
	} else{
	    P1DIR |= 1;
	    while (1){
	        P1OUT ^= 1;
	        __delay_cycles(262144);  // 2 Hz, the angriest frequency
	    }
	}
	return 0;
}
