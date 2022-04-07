/*
 * vnh7070API.h - header file for the vnh707 H-Bridge Driver.
 * We do PWM with Timer A0 on pin 1.2
 * We use P3.3 for the INa (clockwise) signal and P3.4 for the INb (counter-clockwise) signal to the motor driver
 * We do velocity measurements with Timer A1 on pins 2.0 (channel A) and 2.2 (Channel B)
 * We do position measurements with LS7366 encoder break-out over SPI
 *  *  S1 Slave Select ->  P7.0 GPIO
 *  MOSI            ->  P4.1 UCB1MOSI
 *  MISO            ->  P4.2 UCB1MISO
 *  SCLK            ->  P4.3 UCB1CLK
 *  5V              ->  3.3V
 *  GND             ->  GND
 *
 *  Created on: Mar. 25, 2022
 *      Author: jamie
 */

#ifndef VNH7070API_H_
#define VNH7070API_H_
 #include <libCmdInterp.h>


#define     MV10MS                  10486       // timer ticks for 10 ms interrupt

#define     A_CW                    BIT0
#define     B_CCW                   BIT1
#define     SEL_PIN                 BIT2

// some static strings for dc motor commands
#define     PWM_FREQ        "pwmFreq"             // 1: set pwm frequency in Hz integer number result is the freq
#define     PWM_DUTY        "pwmDuty\0"         // 1:duty cycle, in percent, integer from 0 to 100.
#define     SET_MTR         "setMtr\0"           // 1:direction and duty cycle, in percent, 0 to 100, signed integer -value = CCC, + value = CW
#define     BRAKE           "brake\0"            //0
#define     GET_SPEED      "getSpeed\0"         //1: mode 0 = degrees/second, 1 = rpm result is speed, in chosen units
#define     SET_SPEED       "setSpeed\0"        //  1: speed degrees/second
#define     GET_POS       "getPos\0"            // 0
#define     SET_POS       "setPos\0"            // 1: position in degrees, unsigned integer, 2) unsigned integer, 3) signed char for direction

// some static strings for error messages for these commands
#define     VNH_ERR0         "Value must be positive."
#define     VNH_ERR1         "PWM duty must be <= 100\0"

// and some matching mnemonics
#define     NEG_VAL         0
#define     BAD_DUTY        1



// adds CMD interpreter commands and error messages for dc motor
unsigned char vnhInit (void);

unsigned char vnhPWMfreq (CMDdataPtr commandData);
unsigned char vnhDutyCycle (CMDdataPtr commandData);
unsigned char vnhSetMtr (CMDdataPtr commandData);
unsigned char vnhBrake (CMDdataPtr commandData);
unsigned char vnhGetSpeed (CMDdataPtr commandData);

unsigned char vnhSetSpeed (CMDdataPtr commandData);
unsigned char vnhGetPosition (CMDdataPtr commandData);
unsigned char vnhSetPosition (CMDdataPtr commandData);


//unsigned char vnhSetSpeed (float speedRS);


/*Raw vnh7070 Input control. INa, INb, SEL. ctrl can be in range 0x0->0x7.
 *  INa = ctrl<0>, INb = ctrl<1>, SEL = ctrl
If ctrl is in range and is output, then 0 is returned otherwise -1 is returned.
 */
signed char vnh7070InputCtrl (unsigned char ctrl);

/*Executes CW control signals with requested duty cycle to vnh7070. Calls vnh7070InputCtrl, timerA0DutyCycleSet
Sets SEL such that CS is High Z.
Returns 0 if successful, -1 otherwise.
*/
unsigned char vnh7070CW (unsigned char dutyCycle);

/*Executes CCW control signals with requested duty cycle to vnh7070. Calls vnh7070InputCtrl, timerA0DutyCycleSet
Sets SEL such that CS is High Z.
Returns 0 if successful, -1 otherwise.*/
signed char vnh7070CCW (unsigned char dutyCycle);

/* Executes vnh7070 brake signals to vnh7070 to implement dynamic braking
 * Calls vnh7070InputCtrl
pwm set HIGH.
SEL set LOW
Returns 0 if successful, -1 otherwise.*/
signed char vnh7070Brake();

#endif /* VNH7070API_H_ */


