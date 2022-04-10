#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include "adc12.h"
#include "libUART1A.h"
#include "BinaryCmdInterp.h"

/**
 * main.c for ADC lab with command interpreter, sending data to host computer
 */


int main(void) {
    usciA1UartInit(19200);
    binInterp_init ();
    binInterp_addCmd (2, &scopeInit);       // unsigned char channel number
    binInterp_addCmd (2, &scopeSetVref);
    binInterp_addCmd (6, &scopeSetSampRate);
    binInterp_addCmd (4, &scopeSetNumSamp);
    binInterp_addCmd (1, &scopeGetData);
    __enable_interrupt();
    binInterp_run ();
}

/* 2 bytes input data is [0] unsigned char FuncNumber = 0 [1] unsigned char channel number.
 * 1 byte output data is errorCode (0 = o.k, 1 = channel out of range)
 * initial voltage range is 3.3V, but can be changed
 * initial sampling rate is 10kHz but can be changed from 16 to 104
 * initial sample size is 200, but this can be decreased
 */

unsigned char scopeInit  (unsigned char * inputData, unsigned char * outputResults){
    unsigned char err =0;
    if (!((inputData[1] < 5) || (inputData [1] == 12))){
        err =1;
    } else{
        // trigger from timer, for sine wave sampling
        TA0CTL = TASSEL__SMCLK + MC__STOP;         // stopped til we want to get some data
        TA0CCTL1 = OUTMOD_3;                        //Set, Reset on TA1. trigger sets on TA0CCR0
        TA0CCR0 = 104;                              //2^20 Hz SMCLCK * 105 = 10ksps = 100 samples of 100Hz wave
        TA0CCR1 = 52;                                // trigger for ADC resets on TA0CCR1
        adc12Cfg("3V3", SAMP_MODE_PULSE, CONVERT_TRIG_TIMER, inputData[1]);
    }
    outputResults [0] = 4;
    outputResults [1] = err;
    return 1;
}

/**************************** configure voltage reference *******************************
 * 2 bytes input data is [0] unsigned char FuncNumber = 1 [2] unsigned char Vref code = 0-3
 * 1 byte output data is errorCode (0 = o.k, 1 = vRef code out of range)
 */
unsigned char scopeSetVref  (unsigned char * inputData, unsigned char * outputResults){
    unsigned char err =0;
    if (inputData [1] > 3){
        err =1;
    }else{
        if (inputData[1] == 3) {                   //   VR+ = AVCC+ and not VREF+
            ADC12MCTL0 |= ADC12SREF_0;
            REFCTL0 &= ~(REFMSTR + REFON);              //  disable REF control. Use the ADC12A to provide 3V3
        }else{                                          // we are using one of the voltage references
            ADC12MCTL0 |= ADC12SREF_1;                      // select VR+ = VREF+
            REFCTL0 |= (REFMSTR + REFON);                   //  enable reference control. Use REF module registers.
            if (inputData[1] == 0){
               REFCTL0 |= REFVSEL_0;
            }else{
               if(inputData[1] == 1){
                   REFCTL0 |= REFVSEL_1;
               }else{
                   if(inputData[1] == 2){
                       REFCTL0 |= REFVSEL_2;
                   }
               }
            }
        }
    }
    outputResults [0] = 4;
    outputResults [1] = err;
    return 1;
}

/**************************** Set sampling rate  *******************************
 * 6 bytes input data is [0] unsigned char FuncNumber = 2 [1] pad byte [2-5] long int sampling rate
 * 4 byte output data [0] error code 91 =too slow, 2=too fast), [1] padding, [2-3] unsigned in value of CCR0
 * use 2^20 Hz SMCLCK. fastest freq to try is 104 kHz, equals 10 CPU clock ticks
 * slowest frequency is 2^20/65535  = 17 Hz*/
unsigned char scopeSetSampRate (unsigned char * inputData, unsigned char * outputResults){
    unsigned char err = 0;
    unsigned long * sampRatePtr = (unsigned long *)&inputData [2];
    if (*sampRatePtr < 17){
        err =1;
    }else{
        if (*sampRatePtr > 104000){
            err = 2;
        }
    }
    if (err ==0){
        TA0CCR0 = (0x100000/ *sampRatePtr) -1;
        TA0CCR1 = TA0CCR0/2;
        unsigned int *  timValPtr = (unsigned int * ) &outputResults [2];
        * timValPtr = TA0CCR0;
    }
    outputResults [0] = 4;
    outputResults [1] = err;
    return 1;
}

/**************************** set number of samples *******************************
 * 4 bytes input data [0] unsigned char FuncNumber = 3 [1] pad byte [2-3] unsigned int number of samples
 * 1 byte output data [0] error code 1 = too many samples */
unsigned char scopeSetNumSamp (unsigned char * inputData, unsigned char * outputResults){
    unsigned char err = 0;
    unsigned int * nSamplesPtr = (unsigned int *)&inputData [2];
    if (*nSamplesPtr > ADC_SAMPLES){
        err = 1;
    }else{
        gADCnumSamples = *nSamplesPtr;
    }
    outputResults [0] = 4;
    outputResults [1] = err;
    return 1;
}

/**************************** Gets gADCnumSamples worth of data *******************************
 * 1 byte input data - [0] unsigned char FuncNumber = 4  everything else should already be configured
 * no output data - we send the data ourselves with a different interrupt function */
unsigned char scopeGetData (unsigned char * inputData, unsigned char * outputResults){
    ADC12CTL0 |= ADC12ON;
    TA0CTL |= MC__UP;
     __low_power_mode_0();  // when we wake up, we should have new data, send it
     usciA1UartEnableTxInt (0);
     usciA1UartInstallTxInt (&DATA_TxInterrupt);
     usciA1UartEnableTxInt (1);
     __low_power_mode_0();  // when we wake up, data has been sent

     usciA1UartInstallTxInt (&binInterp_TxInterrupt);
     usciA1UartEnableTxInt (1);
     return 0;      // we sent the data, nothing more to add!
}


char DATA_TxInterrupt (unsigned char* lpm){
    static unsigned char tCharCount=0;
    unsigned char * buffer = (unsigned char *) ADC_DATA;
    unsigned char rChar = buffer [tCharCount++];
    if (tCharCount == (2*gADCnumSamples)){
        *lpm = 1;
        usciA1UartEnableTxInt (0);
    }
    return rChar;
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
