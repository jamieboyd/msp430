/**
 * main.c
 * for ADC lab.
 * do DC voltmeter with gSampMode = SAMP_MODE_EXTENDED or SAMP_MODE_PULSE  and gTrigMode = CONVERT_TRIG_SOFT
 * do 10 kHz oscilloscope with gSampMode = SAMP_MODE_PULSE  and gTrigMode = CONVERT_TRIG_TIMER
 */


#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include "adc12.h"
#include "libUART1A.h"


char resultBuf [40];

unsigned int ADC_DATA [ADC_SAMPLES];

volatile unsigned int adc12Result;

int main(void) {
    float ADCval;
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
        ADC12CTL0 |= ADC12ON;
        TA0CTL |= MC__UP;
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
