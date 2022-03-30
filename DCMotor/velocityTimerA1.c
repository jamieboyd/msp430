/*
 * timerA1.c - uses TimerA1 for capture mode to get velocity, direction measurement from a quadrature encoder input
 *
 *  Created on: Mar. 6, 2022
 *
 *
*/

#include <msp430.h>
#include <velocityTimerA1.h>

volatile unsigned char gDirection;            // 0 for clockwise, 2 for counter-clockwise
volatile unsigned long int gCurent_Count;       // long in case we have roll-overs
float gTimerA0velocity =0;                      // starts at 0, we init with PWM =0

 /*************************** timerA1XT1toACLK ***************************************
  * - sets up ACLK to use 32768 Hz crystal XT1 as source - it's already on the launchpad
  * - Supposed to have better stability than default REFOCLK with same frequency but  I didn't
  * - see any difference, assuming it was actually working as intended
  * - Adapted from http://mostlyanalog.blogspot.com/2015/05/clocking-msp430f5529-launchpad-part-ii.html
  * Arguments: None
  * returns: nothing
  * Author: Jamie Boyd
  * Date: 2022/03/06 */
void timerA1XT1toACLK (void){
  P5SEL |= BIT4 + BIT5;
  UCSCTL6 &= ~XT1OFF;
  UCSCTL6 |= XCAP_3;
  UCSCTL7 &= ~(XT2OFFG | XT1LFOFFG | DCOFFG) ;
  while (SFRIFG1 & OFIFG) {
      SFRIFG1 &= ~OFIFG;
  }
  UCSCTL6 &= ~XT1DRIVE_3;
  UCSCTL4 &= ~0X700;
}

/*************************** timerA1Init ***************************************
 * - Initializes timer A1 for capture mode and enables interrupt
 * Arguments: 4
 * src: 1 = ACLK, 2 = SMCLK
 * doExport: if set, ACLK or SMCLK as appropriate is output on its usual pin
 * div1: 1st divisor for clock source, one of 1, 2, 4, or 8
 * div2: 2nd divisor for clock source, from 1 to 8
 * returns: -1 if dividers were out of spec, else 0
 * Author: Jamie Boyd
 * Date: 2022/03/06 */
signed char timerA1Init (unsigned char src, unsigned char doExport, unsigned int div1, unsigned int div2){
    signed char rVal = 0;
    P2SEL |= BIT0;                                      // signal input is on pin is P2.0
    P2DIR &= ~BIT0;
    TA1CTL = TACLR;
    TA1CTL =  MC__CONTINUOUS + TAIE;
    switch (src){
    case 1:
        TA1CTL |= TASSEL__ACLK;
        if (doExport){
            // put ACLK on an output pin so we can have a look at it
            P1SEL |= BIT0;
            P1DIR |= BIT0;
        }
        break;
    case 2:
        TA1CTL |= TASSEL__SMCLK;
        if (doExport){
            P2SEL |= BIT2;
            P2DIR |= BIT2;
        }
        break;
    default:
        rVal = -1;
        break;
    }
    if (rVal == -1){
        return -1;
    }
    switch (div1){
    case 1:
        TA1CTL |= ID__1;
        break;
    case 2:
        TA1CTL |= ID__2;
        break;
    case 4:
        TA1CTL |= ID__4;
        break;
    case 8:
        TA1CTL |= ID__8;
    }
    switch (div2){
    case 1:
        TA1EX0 = TAIDEX_0;
        break;
    case 2:
        TA1EX0 = TAIDEX_1;
        break;
    case 3:
        TA1EX0 = TAIDEX_2;
        break;
    case 4:
        TA1EX0 = TAIDEX_3;
        break;
    case 5:
        TA1EX0 = TAIDEX_4;
        break;
    case 6:
        TA1EX0 = TAIDEX_5;
        break;
    case 7:
        TA1EX0 = TAIDEX_6;
        break;
    case 8:
        TA1EX0 = TAIDEX_7;
        break;
    default:
        rVal = -1;
        break;
    }
    if (rVal== -1){
        return -1;
    }
    TA1CCTL1 = CM_1 + CCIS_0 + SCS + CAP +  CCIE;        // use CCTL1 in capture mode rising edge on p2.0
    TA1CCTL1 &= ~CCIFG;                                 // clear flags
    TA1R = 0;
    P2DIR &= ~2;  // p2.2 as input pin for channel b for direction
    return rVal;
}


float timerA1getVleocity(){
    if (gDirection  == 100){  // we set this to impossible value to signal lack of updates
        gTimerA0velocity =0;
     }else{
         gTimerA0velocity = RADIANS_SEC_COUNT/gCurent_Count;
         if (gDirection){
             gTimerA0velocity *= -1;
          }
     }
     gDirection = 100;
     return gTimerA0velocity;
}

/*************************** TimerA1 ***************************************
 * - Timer interrupt - takes a measurement, updates global variables
 * Arguments: None
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/03/06 */
#pragma vector = TIMER1_A1_VECTOR
__interrupt void TimerA1 (void){
    static unsigned char overFlow = 0;
    static unsigned long int startTime =0;
    static unsigned long int endTime;
    switch(__even_in_range(TA1IV,14)) {
    case  0: break;       // Vector  0: No interrupts
    case  2:              // Vector  2: CCIFG
        endTime = TA1CCR1;
        if (overFlow){
            endTime += (overFlow * 65535);
            overFlow =0;
        }
        gCurent_Count = endTime - startTime;
        startTime = TA1CCR1;
        gDirection = (P2IN & BIT2);
        break;
    case 4:
        break;
    case 6:
        break;
    case 8:
        break;
    case 10:
        break;
    case 12:
        break;
    case 14:
        TA1R =0;
        overFlow += 1;
        break;
    }
    TA1CCTL1 &= ~TAIFG;

}
