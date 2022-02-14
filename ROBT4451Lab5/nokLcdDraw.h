/*
 * nokLcdDraw.h
 *  Code to draw encoder angles
 *  Created on: Feb. 7, 2022
 *      Author: jamie
 */

#ifndef NOKLCDDRAW_H_
#define NOKLCDDRAW_H_

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
 * requires a ratio for integer division
 * that in integer math, first multiply pulse count by 3 and then divide by 23.
 *
 * Yes, there are a lot of "magic numbers" in this code
 */


#define COUNTS_PER_REV 1440
#define COUNTS_PER_DEGREE 4

// we want to draw relative to the "center" of the screen, but with an even number of pixels, there is no center pixel
// we use the pixel to the left of center when on left side of screen and pixel to th right of center when on right side of screen
// similarly for top and bottom. NOTE origin of screen coordinates is TOP, LEFT
#define NOK_LCD_X_CTR_L         41
#define NOK_LCD_X_CTR_R         42
#define NOK_LCD_Y_CTR_T         23
#define NOK_LCD_Y_CTR_B         24

#define NOK_LCD_CIRC_STEPS      150     // steps around the circle rounded down from (2*pi)/ atan (1/24)
#define NOK_LCD_CIRC_Q1         37      // NOK_LCD_CIRC_STEPS/4, rounded down
#define NOK_LCD_CIRC_Q2         75      // NOK_LCD_CIRC_STEPS/2
#define NOK_LCD_CIRC_Q3         112     // NOK_LCD_CIRC_STEPS * 3/4, rounded down

#define NOK_LCD_ASP_RAT_NUM     7       // To account for un-square pixels, multiple x values by NUM/DENOM
#define NOK_LCD_ASP_RAT_DENOM   6

#define NOK_LCD_MAP_NUM         5       // these form a divisor to map encoder counts per revolution to display steps
#define NOK_LCD_MAP_DENOM       48      // for 360 count/rev encoder with quadrature

//#define NOK_LCD_MAP_NUM         3       // these form a divisor to map encoder counts per revolution to display steps
//#define NOK_LCD_MAP_NUM         92      // for 1150 count/rev encoder with quadrature


#define NOK_LCD_PER_BAR_NUM         7     // 1440 counts / rev and 84 bars/count
#define NOK_LCD_PER_BAR_DENOM       120
#define NOK_LCD_BAR_MAX             504     // count rolls over

#define NOK_LCD_LINE_RES            17





void nokDrawCircle ();
void nokDrawAngle (signed int encoderCntMod);
unsigned char nokDrawGetX (unsigned char pos);
unsigned char nokDrawGetY (unsigned char pos);
void nokDrawBars (signed long int posCount);

#endif /* NOKLCDDRAW_H_ */
