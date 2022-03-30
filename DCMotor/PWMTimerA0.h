/*
 * PWM.h
 *
 *  Created on: Mar. 22, 2022
 *      Author: jamie
 */

extern float timerA0speed;
extern unsigned int gPWMFreq;        // updated with timer frequency, in case someone asks


#ifndef PWMTIMERA0_H_
#define PWMTIMERA0_H_
unsigned int timerA0Init(unsigned int pwmFreq);
unsigned int timerA0PwmFreqSet(unsigned int pwmFreq);
unsigned char timerA0DutyCycleSet(unsigned char dutyCycle);



#endif /* PWMTIMERA0_H_ */
