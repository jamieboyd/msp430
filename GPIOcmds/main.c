#include <msp430.h> 
#include "portGPIO.h"

/**
 * main function for PortGPIO. Initializes command interpreter, adds its commands and error messages, and
 * everything else is done with interrupts.
 *
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	if (libCMD_init ()){
	    gPortCmdsErrOffset = portInit ();
	    libCMD_run ();
	}
	return 0;
}
