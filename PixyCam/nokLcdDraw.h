/*
 * nokLcdDraw.h  header file for code to draw encoder angles and progress bars as part of the Friesen Encoder Display Interface
 * Uses drawing primitives (lines, pixels) form the nok5110LCD library to draw bars, circles, and lines at particular angles
 *
 *      NOTE: non standard mapping of angular displacement used in this code
 *      0 degrees position is at the TOP of the screen, not right edge. Positive rotation is in the clockwise direction.
 *
 *
 *  Created on: 2022/02/07
 *      Author: jamie boyd
 */

#ifndef NOKLCDDRAW_H_
#define NOKLCDDRAW_H_

#include "nok5110LCD.h"

/* Design to draw a line from center of screen at an angle corresponding to angle of encoder. Display Angle resolution?
 * At 0 and 180 degrees, line extends 24 pixels to edge of screen. display angular resolution = atan (1/24) = 2.38594 degrees
 * Round that to 2.4 degrees. We have 360/2.4 = 150 steps around the circle, or 37.5 steps/quadrant.
 *
 * We calculate (x,y) endpoint of line with sin and cos functions using a pre-generated array, which is a quadrant of a sine
 * wave. Array was sized for 2.4 degree increments, from 0 to 90 degrees, multiplied by line length (24) and by aspect ratio
 * numerator (7) and then rounded. The Igor Pro command to make this was sq1 = round (sin (x*(pi/180)) * 24 * 7)
 * We use symmetry and sin-cos phase relationship to generate values for any angle from 0 to 360.
 *
 *  PIXELS ARE NOT SQUARE (There are 48 pixels in the 3/4 inch screen width and 84 pixels in the 1 and 1/8 inch
 *  screen height. Y values are divided by 7 and X values are divided by 6 to account for aspect ratio. See getX and getY functions
 * Using our non-standard angle convention, x = radius * sin (angle) and y = -radius *  cos (angle).
 *
 * To map encoder resolution (say 1150 *4 pulses per revolution or 360 * 4 pulses per revolution) to the 150 steps of the display
 * we use a ratio for integer division.
 *
 */

/* we want to draw relative to the "center" of the screen, but with an even number of pixels, there is no center pixel
// We use the pixel to the right of center when on left side of screen and pixel to the left of center
// when on right side of screen.  similarly for top and bottom. NOTE origin of screen coordinates is TOP, LEFT */
#define COUNTS_PER_REV          4600         // for the motor in 1985 lab 100 pulses/rev  x 11.5:1 gear box x 4 quadrature
#define COUNTS_PER_DEGREE_NUM   9           // for integer math rounding. 360/4600 = 9/115
#define COUNTS_PER_DEGREE_DENOM 115
#define COUNTS_PER_HALF_DEGREE  57           // before dividing by COUNTS_PER_DEGREE_DENOM. 9 * (4600/360)/2 used for rounding counts to nearest degree. see simpleRound

//#define COUNTS_PER_REV            1440          // for a 360 count/degree encoder. This includes quadrature.
//#define COUNTS_PER_DEGREE_NUM     1             // easy when COUNTS_PER_REV divides 360
//#define COUNTS_PER_DEGREE_DENOM   4             // COUNTS_PER_REV dvided by 360
//#define COUNTS_PER_HALF_DEGREE    8           //  cause 8/4 = 2. for rounding counts to nearest degree. see simpleRound


#define NOK_LCD_X_CTR_L         42
#define NOK_LCD_X_CTR_R         41
#define NOK_LCD_Y_CTR_T         24
#define NOK_LCD_Y_CTR_B         23

#define NOK_LCD_CIRC_STEPS      150     // steps around the circle rounded down from (2*pi)/ atan (1/24)
#define NOK_LCD_CIRC_Q1         37      // NOK_LCD_CIRC_STEPS/4, rounded down
#define NOK_LCD_CIRC_Q2         75      // NOK_LCD_CIRC_STEPS/2
#define NOK_LCD_CIRC_Q3         112     // NOK_LCD_CIRC_STEPS * 3/4, rounded down

#define NOK_LCD_ASP_RAT_NUM     7       // To account for un-square pixels, multiple x values by NUM/DENOM
#define NOK_LCD_ASP_RAT_DENOM   6

//#define NOK_LCD_MAP_NUM         5       // these form a divisor to map encoder counts per revolution to display steps
//#define NOK_LCD_MAP_DENOM       48      // for 360 count/rev encoder with quadrature
//#define NOK_LCD_HALF_STEP       24      // 5* ((1440/150)/2) simplifies nicely

//#define NOK_LCD_PER_BAR_NUM         7     // 1440 counts / rev and 84 bars/rev
//#define NOK_LCD_PER_BAR_DENOM       120

#define NOK_LCD_MAP_NUM            3       // these form a divisor to map encoder counts per revolution to display steps
#define NOK_LCD_MAP_DENOM          92      // for 1150 x 4 = 4600 count/rev encoder with quadrature
#define NOK_LCD_HALF_STEP          46        // 3 * ((4600/150)/2)

#define NOK_LCD_PER_BAR_NUM         4     // 4600 counts/rev / rev and 84 bars/rev
#define NOK_LCD_PER_BAR_DENOM       219



#define NOK_LCD_BAR_MAX             504     // count rolls over

extern unsigned char sinArray [38]; // too good to not use in other places

void nokDrawCircle ();
void nokDrawAngle (signed int encoderCntMod);
unsigned char nokDrawGetX (unsigned char pos);
unsigned char nokDrawGetY (unsigned char pos);
void nokDrawBars (signed long int posCount);
signed int dispRound (signed int count);

/*************************** simpleRound ***************************************
  * -utility function to round count not just floor when dividing encoder counts
  * Arguments: 1
  * argument 1: the count of the encoder, modulus counts_pre_rev
  * returns: result of the division, rounded to nearest whole number
  * Author: Jamie Boyd
  * Date: 2022/02/13 */
 signed int simpleRound (signed int count);

#endif /* NOKLCDDRAW_H_ */
