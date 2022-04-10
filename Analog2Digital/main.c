#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include "adc12.h"
#include "libUART1A.h"


/**
 * main.c
 */

unsigned char gSampMode = SAMP_MODE_PULSE;
unsigned char gTrigMode = CONVERT_TRIG_TIMER;

unsigned int ADC_DATA [ADC_SAMPLES];

volatile unsigned int adc12Result;

int main(void) {
    float ADCval;
    char resultBuf [40];
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    if (gTrigMode == CONVERT_TRIG_SOFT){
        TA0CTL = TASSEL__ACLK | ID_0 | MC__UP | TAIE;
        TA0CCR0 = 8192;                // set timer interval to 250 ms
        TA0CTL &= ~TAIFG;              // clear flag
        usciA1UartInit(19200);
    }else {                     // trigger from timer, for sine wave sampling
        TA0CTL = TASSEL__SMCLK + MC__UP;         // SMCLK, up mode, clear TAR
        TA0CCR0 = 104;                            //2^20 Hz SMCLCK * 105 = 10ksps = 100 samples of 100Hz wave
        TA0CCTL1 = OUTMOD_3;                        //Set, Reset on TA1
        TA0CCR1 = 52;                                //trigger for ADC
    }
    adc12Cfg("3V3", gSampMode,gTrigMode, 0);
    __enable_interrupt();

    if (gTrigMode == CONVERT_TRIG_SOFT){
        while (1){
             __low_power_mode_0();
            ADCval = ((float)adc12Result/4096.0) * 3.3;
             sprintf (resultBuf, "ADC val = %.3f volts.\r", ADCval);
             usciA1UartTxString(resultBuf);
        }
    } else {                // CONVERT_TRIG_TIMER, using a timer as source of conversion trigger
        while (1){
        __low_power_mode_0();
        _nop();
        ADC12IE   |= ADC12IE0;
        ADC12CTL0 |= ADC12ON;
        }
    }
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void TimerA0 (void){
    if (gSampMode == SAMP_MODE_EXTENDED){   // extended mode, adc12SampSWConv waits for sample and hold time
        adc12SampSWConv ();
    }else{                               // pulse sampling mode - toggle ADC12SC quick as you like
        ADC12CTL0 |= ADC12SC;
        ADC12CTL0 &= (~ADC12SC);  // note: the user manual indicates this bit clears automatically?  Is this true?  NO it did NOT get reset
    }
        TA0CTL &= ~TAIFG;              // clear flag
}



/*
// Timer1 interrupt service routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_A0_VECTOR))) TIMER1_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  P1OUT ^= 0x01;                            // Toggle P1.0
  TA1CCR0 += 50000;                         // Add Offset to CCR0
}



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
