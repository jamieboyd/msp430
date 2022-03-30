/*
 * PTVM.h
 * * Does velocity measurement of a quadrature encoder system using Timer A1 to measure the time between low-to-high
 * transitions on channel A of the encoder on pin 2.,0. At 20V, we get about 37.75 radians/second. With P-motor= 100 and N= 11.5
 * that's about 7000 pulses per second, or T = about 0.143 milli-seconds.  We want to make a timer with a period such
 * that we get get at least 10 counts in that time (for 10% accuracy), so about 70 kHz. At low end, 4V = about 700
 * pulses/second, so we should get 10x as many timer counts, or 1% accuracy.
 *
 * Timer srcs: ACLK = 32768 Hz SMCLK = 2^20  = 1048576 Hz.
 *
 * To get near 70Khz We select SMCLK with dividers of 2 (using TA1CTL) and 7 (using TA1EX0) = 2^20/14 = 74898 Hz
 * To get get from  n timer counts per pulse to radians/second:
 * (1 pulse/n counts) * (74898 counts/second) * (1 revolution/1150 pulses) * (2*pi radians/rev) = 409.216/n  rad/s
 *  Created on: Mar. 6, 2022
 *      Author: jamie
 *
 */

#ifndef VELOCITYTIMERA1_H_
#define VELOCITYTIMERA1_H_

#define     RADIANS_SEC_COUNT          409.216          // take this number and divide it by counts to get radians/sec

extern volatile unsigned char gDirection;            // 0 for clockwise, 2 for counter-clockwise
extern volatile unsigned long int gCurent_Count;       // long in case we have roll-overs
extern float gTimerA0velocity;


 /*************************** timerA1XT1toACLK ***************************************
  * - sets up ACLK to use 32768 Hz crystal XT1 as source - it's already on the launchpad
  * - Supposed to have better stability than default REFOCLK with same frequency but  I didn't
  * - see much difference, assuming it was actually working as intended
  * - Adapted from http://mostlyanalog.blogspot.com/2015/05/clocking-msp430f5529-launchpad-part-ii.html
  * Arguments: None
  * returns: nothing
  * Author: Jamie Boyd
  * Date: 2022/03/06 */
void timerA1XT1toACLK (void);
signed char timerA1Init (unsigned char src,  unsigned char doExport, unsigned int div1, unsigned int div2);
float timerA1getVleocity();
__interrupt void TimerA1 (void);


#endif /* VELOCITYTIMERA1_H_ */
