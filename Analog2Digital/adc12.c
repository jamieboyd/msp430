/*************************************************************************************************
 * adc12.c
 * - C implementation file for MSP430 ADC12
 *
 *  Author: Greg Scutt
 *  Created on: May 1,2018
 **************************************************************************************************/

#include <msp430.h>
#include <string.h>
#include "libUART1A.h"
#include "adc12.h"

volatile unsigned int adc12Result;
unsigned int ADC_DATA [ADC_SAMPLES];
unsigned int gADCnumSamples = ADC_SAMPLES;

unsigned char gSampMode = SAMP_MODE_PULSE;
unsigned char gTrigMode = CONVERT_TRIG_TIMER;

/************************************************************************************
* Function: adc12Cfg
* - configures an ADC channel for  single channel conversion with selectable trigger and vref.
* - extended or pulse mode sampling.
* -  12 bit resolution. Assumes VR- = AVSS = 0V. Enables conversion ADC12ENC = 1 and interrupt ADC12IE.
* -  does not select ADC12CLK.  Should be added
* Arguments:
* vref - REF+ reference. Send string "2V5", "1V5", "2V5", "3V3"
* sampMode - sample/hold and convert control. Sets/Clears ADC12SHP for pulse or extended mode.
* convTrigger - selects timer source ADC12SHS_1 or ADC12SC register bit.
            1 - will select the timer.  0 - will select SW Register bit
* adcChannel - selects adcChannel 0:15 as adc input. Not implemented.
* return: none
* Author: Greg Scutt
* Date: May 1, 2018
* Modified: April 23, 2021
*
*
* The ADC12CLK is used both as the conversion clock and to generate the sampling
*  period when the pulse sampling mode is selected.
*
************************************************************************************/
unsigned char adc12Cfg(const char * vref, char sampMode, char convTrigger, char adcChannel)  {

    /*configure REF MODULE.
    see user manual 28.2.3  and the REF Module in section 26
    Since tte MSP430F5529 has a separate REF Module, we are using it directly
    and programming the ADC12A to accept its inputs.  Make sure you understand this.*/

    ADC12CTL0 &= ~ADC12ENC;         // make sure ADC conversion is not enabled before configuring
    ADC12CTL0 |= ADC12ON;           // ADC12 on
    /************* Select a channel for repeated smapling of single channel **********************/
    ADC12MCTL0 &= ~ADC12INCH_15;    // deselect every channel
    unsigned char errCode = 0;
    switch (adcChannel){            // add the channel we want
    case 0:
        P6DIR &= ~BIT0;
        P6SEL &= ~BIT0;
        ADC12MCTL0 |= ADC12INCH_0;  // this is 0, but looks nice
        CBCTL3 |= CBPD0;
        ADC12IE   |= BIT0;                              // Enable interrupt
        break;
    case 1:
        P6DIR &= ~BIT1;
        P6SEL |= BIT1;
        ADC12MCTL0 |=ADC12INCH_1;
        CBCTL3 |= CBPD1;
        ADC12IE   |= BIT1;                              // Enable interrupt
        break;

    case 2:
        P6DIR &= ~BIT2;
        P6SEL |= BIT2;
        ADC12MCTL0 |=ADC12INCH_2;
        CBCTL3 |= CBPD2;
        ADC12IE   |= BIT2;                              // Enable interrupt
        break;

    case 3:
        P6DIR &= ~BIT3;
        P6SEL |= BIT3;
        ADC12MCTL0 |=ADC12INCH_3;
        CBCTL3 |= CBPD3;
        ADC12IE   |= BIT3;                              // Enable interrupt
        break;
    case 4:
        P6DIR &= ~BIT4;
        P6SEL |= BIT4;
        ADC12MCTL0 |=ADC12INCH_4;
        CBCTL3 |= CBPD4;
        ADC12IE   |= BIT4;                              // Enable interrupt
        break;
    case 5:
        P6DIR &= ~BIT5;
        P6SEL |= BIT5;
        ADC12MCTL0 |=ADC12INCH_5;
        CBCTL3 |= CBPD5;
        ADC12IE   |= BIT5;
        break;
    case 12:
        P7DIR &= ~BIT0;
        P7SEL |= BIT0;
        ADC12MCTL0 |=ADC12INCH_12;
        break;
    default:
        errCode = 1;                // requested channel does not exist
    }

   /**************************** configure voltage reference *******************************/
    if (strcmp(vref, "3V3") ==0) {                   //   VR+ = AVCC+ and not VREF+
        ADC12MCTL0 |= ADC12SREF_0;
        REFCTL0 &= ~(REFMSTR + REFON);              //  disable REF control. Use the ADC12A to provide 3V3
    }else{                                          // we are using one of the voltage references
        ADC12MCTL0 |= ADC12SREF_1;                      // select VR+ = VREF+
        REFCTL0 |= (REFMSTR + REFON);                   //  enable reference control. Use REF module registers.
        if (!strcmp(vref, "1V5")){
            REFCTL0 |= REFVSEL_0;
        }else{
            if(strcmp(vref, "2V0")==0){
                REFCTL0 |= REFVSEL_1;
            }else{
                if(strcmp(vref, "2V5") ==0){
                    REFCTL0 |= REFVSEL_2;
                }
            }
        }
    }
    /**************************** configure clock source and divider ******************************
     * The ADC12CLK is used both as the conversion clock and to generate the sampling period when
     * the pulse sampling mode is selected. */
    ADC12CTL1 &= ~ADC12SSEL_3;
    ADC12CTL1 |= ADC12SSEL_0;       // 00b = ADC12OSC (5MHz MODCLK), 01b = 32767 Hz ACLK, 10b = MCLK, 11b = SMCLK
    ADC12CTL1 &= ~ADC12DIV_7;
    ADC12CTL1 |= ADC12DIV_0;        // divisor: 0=1, 1 =2, 2 =3,3 =4, 4 =5, 5 =6, 6=7, 7=9

    /******************************* Convert trigger mode ***********************************
     * An analog-to-digital conversion is initiated with a rising edge of the sample input signal SHI.
     * The source for SHI is selected with the SHSx bits and includes the following:
        1) The ADC12SC bit
        2) The Timer_A Output Unit 1
        3) The Timer_B Output Unit 0
        4) The Timer_B Output Unit 1
      **********************************************************************************************/
    ADC12CTL1 &= ~ADC12SHS_3;                    // clear bits before we set them
    if(convTrigger == CONVERT_TRIG_TIMER){
        ADC12CTL1 |= ADC12SHS_1;                    // select timer. assumes timer is setup (Timer A1 CCR1??)

    }
    else{                                           // CONVERT_TRIG_SOFT convert trigger set in main code
        ADC12CTL1 |= ADC12SHS_0;                    // select SW bit ADC12SHS_0= 0, so this does nothing but looks nice
    }

    /********************************* Sampling Mode *********************************************** */
    // NOTE only lower half of channels are done here, should check which channel is used and do other half
    if (sampMode == SAMP_MODE_EXTENDED) {       // extended mode. SAMPCON follows the trigger signal width
        ADC12CTL1 &= ~ADC12SHS_3;
        ADC12CTL1  |= ADC12SHP;
    } else{
        ADC12CTL1  &= ~ADC12SHP;                    // pulse sampling sampling. SAMPCON will be controlled by ADC12SHT1x, ADC12SHT10x. Bits Not implemented here.
        ADC12CTL0 &= ~ADC12SHT0_15;                 // clear bits
        ADC12CTL0 |= ADC12SHT0_3;                   //  32/5E6 = 6.4 uS.
        //ADC12CTL0 |= ADC12MSC;
    }
    ADC12CTL1 |= ADC12CONSEQ_2;                     // Reapeated Single Channel
    ADC12CTL2 |= ADC12RES_2;                        // 12-Bit Resolution
    ADC12IE   |= ADC12IE0;                              // Enable interrupt
    ADC12CTL0 |= ADC12ENC;                          // Enable Conversion
    return errCode;
}

/************************************************************************************
* Function: adc12SampSWConv
* - sets then clears the ADC12SC bit after SAMPLE_ADC cycles to start an ADC12 conversion.
* - make sure the delay (Tsample) created with delay_cycles and SAMPLE_ADC,MCLK is long enough. Errors arise if it is too fast. Can you explain why?
* Arguments:
* - none.
* Author: Greg Scutt
* Date: May 1, 2018
* Modified:
************************************************************************************/
void adc12SampSWConv(void){
    ADC12CTL0 |= ADC12SC;
    __delay_cycles(SAMPLE_ADC);
    ADC12CTL0 &= (~ADC12SC);  // note: the user manual indicates this bit clears automatically?  Is this true?  Verify.
}


#pragma vector = ADC12_VECTOR
interrupt void ADC12ISR(void) {

  static unsigned int iADC = 0;
  switch(__even_in_range(ADC12IV,34)) {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                   // Vector  6:  ADC12IFG0
      if (gTrigMode == CONVERT_TRIG_SOFT){
          __low_power_mode_off_on_exit();
          adc12Result = ADC12MEM0;
      }else{
          if (gTrigMode == CONVERT_TRIG_TIMER){
              ADC12IFG &= ~ADC12IFG0;
              ADC_DATA [iADC++] = ADC12MEM0;
              if (iADC == gADCnumSamples){
                  ADC12CTL0 &= ~ADC12ON;
                  TA0CTL &= ~MC__UPDOWN;
                  __low_power_mode_off_on_exit();
                  iADC = 0;
              }
          }
      }
      break;
    case  8: break;                           // Vector  8:  ADC12IFG1
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
