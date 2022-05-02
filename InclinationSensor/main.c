#include <msp430.h> 
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include "libUART1A.h"
/**
 * main.c. Inclination sensing
 */

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    usciA1UartInit (19200);

/* Set up clock for ADC sampling using timer A0 50 50Hz sampling frequency, so 100 Hz cause channels alternate
 * x2 for nyquist sampling - 2x faster than signal bandwidth
 * choose ACLCK as timer src and divisor of 1.  CCR0 at 32 = 32768/164=200 Hz
 */
    TA0CTL = TASSEL__ACLK + ID__1 + MC__UP ;         //
    TA0CCTL1 = OUTMOD_3;                        //Set, Reset on TA0. trigger sets on TA0CCR0
    TA0CCR0 = 164;
    TA0CCR1 = 82;
/* Setup 2 channels for ADC 12, using 2v5 voltage reference and repeated multi-channel sampling
 * int
 *
 */
    ADC12CTL0 &= ~ADC12ENC;         // make sure ADC conversion is not enabled before configuring
    ADC12CTL0 |= ADC12ON;           // ADC12 on

    // channel 0 pin
    P6DIR &= ~BIT0;
    P6SEL |= BIT0;
    ADC12MCTL0 |= ADC12INCH_0;
    CBCTL3 |= CBPD0;


    // channel 1 pin
    P6DIR &= ~BIT1;
    P6SEL |= BIT1;
    ADC12MCTL1 |=ADC12INCH_1;
    CBCTL3 |= CBPD1;


    ADC12MCTL1 |= ADC12SREF_1;
    ADC12MCTL0 |= ADC12SREF_1;
    REFCTL0 |= (REFMSTR + REFON);                   //  enable reference control. Use REF module registers.
    REFCTL0 |= REFVSEL_2;


    ADC12CTL1 &= ~ADC12CSTARTADD_15;            // start conversion at channel 0, so clear channels
    ADC12MCTL1 |= ADC12EOS;

    ADC12CTL1 &= ~ADC12SSEL_3;
    ADC12CTL1 |= ADC12SSEL_0;       // ADC12 clock src and divider 00b = ADC12OSC (5MHz MODCLK)
    ADC12CTL1 &= ~ADC12DIV_7;
    ADC12CTL1 |= ADC12DIV_0;        // divisor: 0=1, 1 =2, 2 =3,3 =4, 4 =5, 5 =6, 6=7, 7=9
    ADC12CTL1 &= ~ADC12SHS_3;       // select sample timer timer A 0 output unit 1
    ADC12CTL1 |= ADC12SHS_1;
    ADC12CTL1  &= ~ADC12SHP;                    // pulse sampling sampling. SAMPCON will be controlled by ADC12SHT1x, ADC12SHT10x. Bits Not implemented here.



    ADC12CTL1 |= ADC12CONSEQ_3;                     // Repeated Sequence of Channels

    ADC12CTL2 |= ADC12RES_2;                        // 12-Bit Resolution


       ADC12IE   |= ADC12IE0 + ADC12IE1;                              // Enable interrupt
       ADC12CTL0 |= ADC12ENC;                          // Enable Conversion
       __enable_interrupt();


    while (1){};   // the interrupt does all
    return 0;
}




/* Channel 0 has input from X axis
 *
 * Channel 1 has input from Y axis
 * */


#pragma vector = ADC12_VECTOR
interrupt void ADC12ISR(void) {
  static unsigned int adcX [25];
  static unsigned int adcY [25];
  static signed long int sumX =0;
  static signed long int sumY =0;
  static unsigned char ii =0;
  char resultBuf [30];
  float angle;
  switch(__even_in_range(ADC12IV,34)) {
      case  0: break;                           // Vector  0:  No interrupt
      case  2: break;                           // Vector  2:  ADC overflow
      case  4: break;                           // Vector  4:  ADC timing overflow
                                      // Vector  6:  ADC12IFG0
      case  6:
          sumX -= adcX [ii];
          adcX [ii] = ADC12MEM0;
          sumX += adcX [ii];
          break;
      case 8:                           // Vector  8:  ADC12IFG1
        sumY -= adcY [ii];
        adcY [ii] = ADC12MEM1;
        sumY += adcY [ii];
        ii += 1;
        if (ii == 25){
            ii=0;
            //sprintf (resultBuf, "X = %d, Y = %d\r", ADC12MEM0, ADC12MEM1);
            //usciA1UartTxString (resultBuf);
            angle = 57.295*atan2((51175-sumX), (51175 - sumY));
            sprintf (resultBuf, "Avg Angle = %.3f deg.\r", angle);
            usciA1UartTxString (resultBuf);
            //angle = 57.295*atan2((2047-ADC12MEM0), (2047 - ADC12MEM1));
            //sprintf (resultBuf, "This Angle = %.3f deg.\r", angle);
            //usciA1UartTxString (resultBuf);

        }
         break;
      case 10: break;                           // Vector 10:  ADC12IFG2
    case 12: break;                           // Vector 12:  ADC12IFG3
    case 14: break;                           // Vector 14:  ADC12IFG4
    case 16: break;                           // Vector 16:  ADC12IFG5
    case 18: break;                           // Vector 18:  ADC12IFG6
    case 20: break;                           // Vector 20:  ADC12IFG7
    case 22: break;                           // Vector 22:  ADC12IFG8
    case 24: break;                           // Vector 24:  ADC12IFG9
    case 26: break;                           // Vector 26:  ADC12IFG10
    case 28: break;                           // Vector 28:  ADC12IFG11
    case 30: break;                           // Vector 30:  ADC12IFG12
    case 32: break;                           // Vector 32:  ADC12IFG13
    case 34: break;                           // Vector 34:  ADC12IFG14
    default: break;
    }
}


