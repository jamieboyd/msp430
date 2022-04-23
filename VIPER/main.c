#include <msp430.h> 

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
/* Set up PWM output on timer A1, CCR1 on P2.0. We want 10kHz PWM update rate. clk src = SMCLCK, DIV = 1. CCR0 = 104
 * Map 0-3V3 signal to 0-104 PWM output. PWM output, TA1CCR1 = ADCValue/624   */
	P2DIR |= BIT0;         // output pin 2.0    TA1.1 pin
	P2SEL |= BIT0;          // pin 2.0
	TA1CTL = (TASSEL__SMCLK  + ID__1 + MC__UP); // Timer_A0 control register, SMCLK, , Up mode(timer counts up to CCR0)
	TA1CCTL1 = OUTMOD_7;
	TA1CCR0 = 104;
	TA1CCR1 = 52;
/* Set up clock for ADC sampling using timer A0.
 * Map 0->65535 from ADC (0-3V3) onto 0.25 Hz -> 50Hz sampling frequency  Ts 4 seconds -> 0.02 seconds
 * We want a sampling clock that can count to 4 seconds with max resolution of Tsample
 * choose SMCLCK as timer src and divisor of 4. Max count of timer is 4 seconds (2^20/4)/65536  = 4
 * Resolution Tsample = 16384 ticks/sec. CCR0 at = 65535 = 4 sec. CCR0 at 323 = 0.02
 * (CCR0  + 1) = 65536  -0.995015 * VADC  CCR0= 66535 - (199 * VADC/200)  */
    TA0CTL = TASSEL__ACLK + ID__4 + MC__UP ;         //
    TA0CCTL1 = OUTMOD_3;                        //Set, Reset on TA1. trigger sets on TA0CCR0
    TA0CCR0 = 104;                              //2^20 Hz SMCLCK * 105 = 10 kHz output
    TA0CCR1 = 52;                                // trigger for ADC resets on TA0CCR1
/* Setup 2 channels for ADC 12, using 3v3 voltage reference and repeated multi-channel sampling
 * int
 *
 */
    ADC12CTL0 &= ~ADC12ENC;         // make sure ADC conversion is not enabled before configuring
    ADC12CTL0 |= ADC12ON;           // ADC12 on
    // channel 0 pin
    P6DIR &= ~BIT0;
    P6SEL &= ~BIT0;
    ADC12MCTL0 |= ADC12INCH_0;  // this is 0, but looks nice
    CBCTL3 |= CBPD0;
    ADC12IE   |= BIT0;                              // Enable interrupt
    // channel 1 pin
    P6DIR &= ~BIT1;
    P6SEL |= BIT1;
    ADC12MCTL1 |=ADC12INCH_1;
    CBCTL3 |= CBPD1;
    ADC12IE   |= BIT1;

    ADC12MCTL0 |= ADC12SREF_0;
    ADC12MCTL1 |= ADC12SREF_0;                  // use 3v3 V in as src
    REFCTL0 &= ~(REFMSTR + REFON);              //  disable REF control. Use the ADC12A to provide 3V3
    ADC12CTL0 &= ~ADC12SHT0_15;                 // clear bits
    ADC12CTL0 |= ADC12SHT0_3;                   //  32/5E6 = 6.4 uS.

    ADC12CTL1 &= ~ADC12CSTARTADD_15;            // start conversion at channel 0
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

    ADC12MCTL0 |= ADC12INCH0;
    ADC12MCTL1 |= ADC12INCH1;

       ADC12IE   |= ADC12IE0;                              // Enable interrupt
       ADC12CTL0 |= ADC12ENC;                          // Enable Conversion
       __enable_interrupt();


    while (1){};   // the interrupt does all
    return 0;
}




/* Channel 1 has input from function generator, read this and set PWM duty cycle
 *
 * Channel 2 has input from pot. Read this, and set sampling frequency
 * */


#pragma vector = ADC12_VECTOR
interrupt void ADC12ISR(void) {

  //static unsigned int iADC = 0;
  //unsigned int thisVal;
  switch(__even_in_range(ADC12IV,34)) {
      case  0: break;                           // Vector  0:  No interrupt
      case  2: break;                           // Vector  2:  ADC overflow
      case  4: break;                           // Vector  4:  ADC timing overflow
      case  6:                                   // Vector  6:  ADC12IFG0
          ADC12IFG &= ~ADC12IFG0;
          TA1CCR1 = ADC12MEM0/624;               // changes duty cycle on PWM output
          break;
    case  8:                                   // Vector  8:  ADC12IFG1
        ADC12IFG &= ~ADC12IFG1;
        TA0CCR0= 66535 - ((199 * ADC12MEM1)/200);
        TA1CCR1 = TA0CCR0/2;
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


