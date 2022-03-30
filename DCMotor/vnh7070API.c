/**
 * vnh7070API.c  code for DC motor control using the vnh707 H-Bridge Driver
 * We do PWM with Timer A0 on pin 1.2
 * We use P3.0 for the INa (clockwise) signal and P3.1 for the INb (counter-clockwise) signal to the motor driver, and 3.2 for select Pin
 * We do velocity measurements with Timer A1 on pins 2.0 (channel A) and 2.2 (Channel B)
 * We do position measurements with LS7366 encoder break-out over SPI
 *  *  S1 Slave Select ->  P7.0 GPIO
 *  MOSI            ->  P4.1 UCB1MOSI
 *  MISO            ->  P4.2 UCB1MISO
 *  SCLK            ->  P4.3 UCB1CLK
 *  5V              ->  3.3V
 *  GND             ->  GND
 *
 * Created on: Mar. 25, 2022
 *      Author: jamie
 */

#include <msp430.h>
#include "vnh7070API.h"
#include <stdio.h>
#include <stdlib.h>
#include <libCmdInterp.h>
#include "PWMTimerA0.h"
#include "velocityTimerA1.h"
#include "fedi.h"

unsigned char gVNHerrOffset;

// pre-make an array for a velocity profile
// speed is proportional to voltage is proportional to PWM duty cycle (1- 100), + is CCW, - is CW
// so a signed char is a nice choice here. Move programmed for 1 second.

signed char velProfile [100];
unsigned char posProfile [100];

int main(void){

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    libCMD_init ();
    vnhInit ();
    fediInit ();
    libCMD_run ();
    return 0;
}

unsigned char vnhInit (void){

    P3DIR |= (A_CW + B_CCW + SEL_PIN);        // outputs for clockwise and counterclockwise directions, start them at 0, with PWM =0. Free-wheel state
    vnh7070InputCtrl (SEL_PIN);               // free wheels, but doesn't go into stabdby
    gPWMFreq =timerA0Init (10000);    // PWM output frequency 10 kHz, duty cycle initializes to 0
    timerA1Init (2, 0, 2, 7);       // 74898 Hz, constants used for speed measurements

    // initialize timer 2  with interrupt every 10 ms
    TA2CTL = TASSEL_2 | ID_0 | MC_1 | TAIE;
    TA2CCR0 = _50MS_CNT - 1;                // set timer interval to 50 ms
    TA2CTL &= ~TAIFG;                       // clear flag


    libCMD_addCmd (PWM_FREQ, 1, 0, R_UINT, &vnhPWMfreq);
    libCMD_addCmd (PWM_DUTY, 1, 0, R_NONE, &vnhDutyCycle);
    libCMD_addCmd (SET_MTR, 1, 0, R_NONE, &vnhSetMtr);
    libCMD_addCmd (BRAKE, 0, 0, R_NONE, &vnhBrake);
    libCMD_addCmd (GET_SPEED, 0, 0, R_FLOAT, &vnhGetSpeed);

    gVNHerrOffset = libCMD_addErr (VNH_ERR0);
    libCMD_addErr (VNH_ERR1);

    return 0;
}

/*Raw vnh7070 Input control. INa, INb, SEL. ctrl can be in range 0x0->0x7.
 *  INa = ctrl<0>, INb = ctrl<1>, SEL = ctrl
If ctrl is in range and is output, then 0 is returned otherwise -1 is returned.
 */
signed char vnh7070InputCtrl (unsigned char ctrl){
    signed char rVal =-1;
    if (ctrl <= 7){
        if (ctrl & A_CW){
            P3OUT |= A_CW;
        }else{
            P3OUT &= ~A_CW;
        }
        if (ctrl & B_CCW){
            P3OUT |= B_CCW;
        }else{
            P3OUT &= ~B_CCW;
        }
        if (ctrl & SEL_PIN){
            P3OUT |= SEL_PIN;
        }else{
            P3OUT &= ~SEL_PIN;
        }
        rVal = 0;
    }
    return rVal;
}


/*Executes CW control signals with requested duty cycle to vnh7070. Calls vnh7070InputCtrl, timerA0DutyCycleSet
Sets SEL such that CS is High Z.
Returns 0 if successful, -1 otherwise.
*/
unsigned char vnh7070CW (unsigned char dutyCycle){
    signed char rVal;
    rVal = vnh7070InputCtrl (A_CW);
    if (rVal == 0){
        timerA0DutyCycleSet (dutyCycle);
    }
    return rVal;
}


/*Executes CCW control signals with requested duty cycle to vnh7070. Calls vnh7070InputCtrl, timerA0DutyCycleSet
Sets SEL such that CS is High Z.
Returns 0 if successful, -1 otherwise.*/
signed char vnh7070CCW (unsigned char dutyCycle){
    signed char rVal;
    rVal = vnh7070InputCtrl (B_CCW + SEL_PIN);
    if (rVal==0){
        timerA0DutyCycleSet (dutyCycle);
    }
    return rVal;
}


/* Executes vnh7070 brake signals to vnh7070 to implement dynamic braking
 * Calls vnh7070InputCtrl
pwm set HIGH.
SEL set LOW
Returns 0 if successful, -1 otherwise.*/
signed char vnh7070Brake(){
    signed char rVal;
    rVal = vnh7070InputCtrl (0);
    if (rVal == 0){
        timerA0DutyCycleSet (100);
    }
    return rVal;
}

// sets frequency for PWM signal
unsigned char vnhPWMfreq (CMDdataPtr commandData){
   unsigned char rVal =0;
    if (commandData->args[0] <= 0){
        rVal = gVNHerrOffset + NEG_VAL;
    }else{
        unsigned int newFreq = (unsigned int)commandData->args[0];
        gPWMFreq = timerA0PwmFreqSet(newFreq);
        commandData->result = (unsigned long int) gPWMFreq;
    }
    return rVal;
}


unsigned char vnhSetSpeed (float speedRS){
// initialize timer 2  with interrupt every 10 ms
   TA2CTL = TASSEL_2 | ID_0 | MC_1 | TAIE;
   TA2CCR0 = _50MS_CNT - 1;                // set timer interval to 50 ms
   TA2CTL &= ~TAIFG;
   10485.8


/************************ Functions called from command interpreter ********************************************/


// sets duty cycle without changing direction
unsigned char vnhDutyCycle (CMDdataPtr commandData){
    unsigned char rVal =0;
    if ((commandData->args[0] < 0) || (commandData->args[0] > 100)){
        rVal = gVNHerrOffset + BAD_DUTY;
    }else{
        unsigned char newDuty = (unsigned char)commandData->args[0];
        timerA0DutyCycleSet (newDuty);
    }
    return rVal;
}

// sets pwm duty cycle AND direction, because we get a signed number where negative number goes clockwise, positive counter clockwise
unsigned char vnhSetMtr (CMDdataPtr commandData){
    unsigned char rVal =0;
    unsigned char newDuty;
    if ((commandData->args[0] < -100) || (commandData->args[0] > 100)){
        rVal = gVNHerrOffset + BAD_DUTY;
   }else{
       if (commandData->args[0] < 0){
           newDuty = (unsigned char)(commandData->args[0] * -1);    // negative is clockwise
           vnh7070CW (newDuty);
       }else{
           newDuty = (unsigned char)(commandData->args[0]);         // positive is counter clockwise
           vnh7070CCW (newDuty);
       }
   }
    return rVal;
}

unsigned char vnhBrake (CMDdataPtr commandData){
    vnh7070Brake();
    return 0;
}

unsigned char vnhGetSpeed (CMDdataPtr commandData){
    float newSpeed= timerA1getVleocity ();
    float * rPtr = (float *) &commandData->result;
    *rPtr = newSpeed;
    return 0;
}

// this is the nice version with timer and velocity profile
unsigned char vnhSetSpeed (CMDdataPtr commandData){



}
unsigned char vnhGetPosition (CMDdataPtr commandData);
unsigned char vnhSetPosition (CMDdataPtr commandData);

