/*
 * PWM.c
 *
 *  Created on: Mar. 22, 2022
 *      Author: jamie
 */

#include <msp430.h>
#include <PWMTimerA0.h>

unsigned int gPWMFreq;
/**************************************
 * Function: void timerA0Init()
 *
 *purpose: Initialize timerA0 to the correct settings
 *purpose: also configure the port settings
 *
 *returns PWM frequency chosen closest to desired
 **************************************/
unsigned int timerA0Init(unsigned int pwmFreq){
    unsigned int retVal;
    P1DIR |= BIT2;    // output pin 1.2    TA0.1 pin
    P1SEL |= BIT2;    // pin 1.2

    TA0CCR0 = 99; // assign CCR0 value that the timer will count up to
    TA0CCR1 = 0;  // capture compare register 1 initialized to 0%
    TA0CCTL1 |= (OUTMOD_7); // reset set mode (output is reset when the timer counts to the TAxCCRn value, it is set when the time counts to the TAxCCR0 value

    TA0CTL = (TASSEL__SMCLK | MC_1); // Timer_A0 control register, SMCLK, , Up mode(timer counts up to CCR0)
    retVal = timerA0PwmFreqSet(pwmFreq);

    P1DIR |= BIT2;    // output pin 1.2    TA0.1 pin
    P1SEL |= BIT2;    // pin 1.2
    return retVal;


}
  /*  P1DIR |= BIT3;    // output pin 1.3    TA0.2 pin
    P1SEL |= BIT3;    // pin 1.3
    P1OUT &= ~BIT0;   // Reset pin
*/


/* ***************************************
 * timerA0PwmFreqSet(unsigned int pwmFreq)
 *
 * Finds closest set of divisors to given frequency, given that we want to always set CCR0 to 99 for ease of use
 * We always use SMCLK for clock source - could go from 109 Hz down to 5 Hz using 32768 Hz ACLK.
 * If we wanted to make a crusade out of making an exact frequency of square wave we would get nearest clock divisor with CCR0 = 100
 * then adjust CCR0 up or down to make the exact frequency we wanted - but for PWM the exact frequency of the carrier is seldom of interest as
 * long as it is no too fast for driver to output, and is faster than time constant of the motor. pretty much always 10kHz is a good choice

 *
 ****************************************/
unsigned int timerA0PwmFreqSet(unsigned int pwmFreq) {
    unsigned int rVal;
    if (pwmFreq > 7865){        // no division at all
        rVal = 10486;
        TA0CTL &= ~ID__8;   // clears  all the bits
        TA0EX0 = TAIDEX_0;
    }else{
        if (pwmFreq > 4369){    // divide by 2
            rVal = 5243;
            TA0CTL &= ~ID__8;
            TA0CTL |= ID__2;
           TA0EX0 = TAIDEX_0;
        }else{
            if (pwmFreq > 3058){  // divide by 3
                rVal= 3495;
                TA0CTL &= ~ID__8;
                TA0EX0 = TAIDEX_2;
            }else{
                if (pwmFreq > 2359){    // divide by 4
                    rVal= 2621;
                    TA0CTL &= ~ID__8;
                   TA0CTL |= ID__4;
                   TA0EX0 = TAIDEX_0;
                }else{
                    if (pwmFreq > 1923){    // divide by 5
                        rVal= 2097;
                       TA0CTL &= ~ID__8;
                       TA0EX0 = TAIDEX_4;
                    } else{
                        if (pwmFreq > 1623){    // divide by 6
                            rVal = 1748;
                            TA0CTL &= ~ID__8;
                            TA0EX0 = TAIDEX_5;
                        }else{
                            if (pwmFreq > 1405){    // divide by 7
                               rVal = 1498;
                               TA0CTL &= ~ID__8;
                               TA0EX0 = TAIDEX_6;
                            } else{
                                if (pwmFreq > 1180){    // divide by 8
                                  rVal = 1311;
                                  TA0CTL |= ID__8;
                                  TA0EX0 = TAIDEX_0;
                                }else{
                                    if (pwmFreq > 962){    // divide by 10 (2 X 5)
                                      rVal = 1049;
                                      TA0CTL &= ~ID__8;
                                      TA0CTL |= ID__2;
                                      TA0EX0 = TAIDEX_4;
                                    }else{
                                        if (pwmFreq > 812){     // divide by 12 (2 X 6)
                                            rVal = 874;
                                            TA0CTL &= ~ID__8;
                                             TA0CTL |= ID__2;
                                             TA0EX0 = TAIDEX_5;
                                        }else{
                                            if (pwmFreq > 702){     // divide by 14 (2 X 7)
                                                rVal = 749;
                                                TA0CTL &= ~ID__8;
                                                TA0CTL |= ID__2;
                                                TA0EX0 = TAIDEX_6;
                                            }else{
                                                if (pwmFreq > 590){     // divide by 16 (2 X 8)
                                                    rVal = 655;
                                                    TA0CTL &= ~ID__8;
                                                    TA0CTL |= ID__2;
                                                    TA0EX0 = TAIDEX_7;
                                                }else{
                                                    if (pwmFreq > 481){     // divide by 20 (4 X 5)
                                                        rVal = 524;
                                                        TA0CTL &= ~ID__8;
                                                        TA0CTL |= ID__4;
                                                        TA0EX0 = TAIDEX_4;
                                                    } else{
                                                        if (pwmFreq > 406){     // divide by 24 (4 X 6)
                                                            rVal = 437;
                                                            TA0CTL &= ~ID__8;
                                                            TA0CTL |= ID__4;
                                                            TA0EX0 = TAIDEX_5;
                                                        }else{
                                                            if (pwmFreq > 351){ // divide by 28 (4 x 7)
                                                                rVal = 374;
                                                                TA0CTL &= ~ID__8;
                                                                TA0CTL |= ID__4;
                                                                TA0EX0 = TAIDEX_6;
                                                            }else{
                                                                if (pwmFreq > 295){ // divide by 32 (4 x 8)
                                                                    rVal = 328;
                                                                    TA0CTL &= ~ID__8;
                                                                    TA0CTL |= ID__4;
                                                                    TA0EX0 = TAIDEX_7;
                                                                }else{
                                                                    if (pwmFreq > 240){ // divide by 40 (8 x 5)
                                                                        rVal = 262;
                                                                        TA0CTL |= ID__8;
                                                                        TA0EX0 = TAIDEX_4;
                                                                    }else{
                                                                        if (pwmFreq > 203){ // divide by 48 (8 x 6)
                                                                            rVal = 218;
                                                                            TA0CTL |= ID__8;
                                                                            TA0EX0 = TAIDEX_5;
                                                                        }else{
                                                                            if (pwmFreq > 176){     // divide by 56 (8 x 7)
                                                                                rVal = 187;
                                                                                TA0CTL |= ID__8;
                                                                                TA0EX0 = TAIDEX_6;
                                                                            }else{                  // divide by 64 (8 x8)
                                                                                rVal = 164;
                                                                                TA0CTL |= ID__8;
                                                                                TA0EX0 = TAIDEX_7;
                                                                            }
                                                                        }
                                                                    }
                                                                }

                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                        }
                                    }
                                }
                            }

                        }
                    }
                }
            }
        }
    }
    return rVal;
}


/* ***************************************
 * char timerA0DutyCycleSet(unsigned char dutyCycle)
 *
 * Computes and sets TACCRx register to the appropriate vallue using dutyCycle who's
 * range is integers 0:10
 *
 * returns 0.  if dutyCycle is not within the range 0-100 then return 1
 *
 ****************************************/
unsigned char timerA0DutyCycleSet(unsigned char dutyCycle){
    unsigned char rVal = 1;
    if (dutyCycle <= 100){
        rVal = 0;
        TA0CCR1 = dutyCycle;
    }
    return rVal;
}
