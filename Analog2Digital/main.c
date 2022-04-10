#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include "adc12.h"
#include "libUART1A.h"


/**
 * main.c
 */

volatile unsigned int adc12Result;

int main(void) {
    float ADCval;
    char resultBuf [40];
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    TA0CTL = TASSEL__ACLK | ID_0 | MC_1 | TAIE;
    TA0CCR0 = 8192;                // set timer interval to 250 ms
    TA0CTL &= ~TAIFG;              // clear flag

    usciA1UartInit(19200);
    adc12Cfg("3V3", SAMP_MODE_EXTENDED,CONVERT_TRIG_SOFT, 0);
    __enable_interrupt();
    while (1){
        __low_power_mode_0();
        ADCval = ((float)adc12Result/4096.0) * 3.3;
         sprintf (resultBuf, "ADC val = %.3f volts.\r", ADCval);
         usciA1UartTxString(resultBuf);
        _nop();
    }
}


#pragma vector = TIMER0_A1_VECTOR
__interrupt void TimerA0 (void){
    adc12SampSWConv ();
    TA0CTL &= ~TAIFG;              // clear flag
}




    /*
     *


    float ADCval;
    unsigned int chanErr;
#ifdef EXTENDED
   chanErr = adc12Cfg("3V3", 1, 0, 1);
   char resultBuf [40];
   WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
  // initialize timer 0 for printing results with interrupt every 100 ms
  TA0CTL = TASSEL__ACLK | ID_0 | MC_1 | TAIE;
  TA0CCR0 = 8192;                // set timer interval to 250 ms
  TA0CTL &= ~TAIFG;              // clear flag
#endif
#ifdef PULSE
	 chanErr = adc12Cfg("3V3", 0, 1, 1);
#endif
	 if (!(chanErr)){
	 usciA1UartInit(19200); // initialise UART for 19200 Baud communication
	 __enable_interrupt();
	 while (1){
      __low_power_mode_0();

#ifdef EXTENDED
	     ADCval = ((float)adc12Result/4096.0) * 3.3;
	     sprintf (resultBuf, "ADC val = %.3f volts.\r", ADCval);
	     usciA1UartTxString(resultBuf);
#endif
#ifdef PULSE
	     _nop();
	     ADC12IE   |= ADC12IE0;
	     ADC12CTL0 |= ADC12ON;
	     ADC12CTL0 |= ADC12SC;

#endif
     }
	}
	return 0;
}


#pragma vector = TIMER0_A1_VECTOR
__interrupt void TimerA0 (void){
#ifdef EXTENDED
    adc12SampSWConv ();
#endif

    TA0CTL &= ~TAIFG;              // clear flag
}

*/
