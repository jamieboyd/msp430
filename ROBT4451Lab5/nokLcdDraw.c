
/*
 * nokLcdDraw.c  code to draw encoder angles and progress bars as part of the Friesen Encoder Display Interface
 * Uses drawing primitives (lines, pixels) form the nok5110LCD library to draw bars, circles, and lines at paerticular angles
 *
 *  Created on: 2022/02/07
 *      Author: jamie boyd
 */

#include "nokLcdDraw.h"


// this array contains a quadrant of a sine wave, scaled for 24 pixel radius and 2.4 degree resolution.
// we get both sin and cosine out of this by using phase relationship
static unsigned char sinArray [38] = {0, 7, 14, 21, 28, 35, 42, 49, 55, 62, 68, 75, 81,
                               87, 93, 99, 104, 110, 115, 120, 125, 129, 134, 138, 142,
                               145, 149, 152, 155, 157, 160, 162, 164, 165, 166, 167, 168, 168};


/*************************** nokDrawGetY ***************************************
 * - with an angle defined by position (150->360 degrees), gets the y coordinate for position on the circle
 * Arguments: 1
 * argument 1: position around the circle,from 0-149
 * returns: y position of point in circle at angle (pos/150)*360
 * Author: Jamie Boyd
 * Date: 2022/02/10 */
unsigned char nokDrawGetY (unsigned char pos){
    signed int returnVal;
    if (pos <=  NOK_LCD_CIRC_Q1){
        returnVal = NOK_LCD_Y_CTR_T - (sinArray [NOK_LCD_CIRC_Q1-pos]/NOK_LCD_ASP_RAT_NUM);
    }else{
        if (pos < NOK_LCD_CIRC_Q2){
            returnVal =  NOK_LCD_Y_CTR_B + (sinArray[pos - NOK_LCD_CIRC_Q1]/NOK_LCD_ASP_RAT_NUM);
        }else{
            if (pos <= NOK_LCD_CIRC_Q3){
                returnVal = NOK_LCD_Y_CTR_B + (sinArray [ NOK_LCD_CIRC_Q3 - pos]/NOK_LCD_ASP_RAT_NUM);
            }else{
                returnVal = NOK_LCD_Y_CTR_T - (sinArray [pos - NOK_LCD_CIRC_Q3]/NOK_LCD_ASP_RAT_NUM);
            }
        }
    }
    return returnVal;
}

/*************************** nokDrawGetX ***************************************
 * - with an angle defined by position (150->360 degrees), gets the x coordinate for position on the circle
 * Arguments: 1
 * argument 1: position around the circle,from 0-149 must be 0 <= pos < NOK_LCD_CIRC_STEPS
 * returns: x position of point in circle at angle (pos/150)*360
 * Author: Jamie Boyd
 * Date: 2022/02/10 */
unsigned char nokDrawGetX (unsigned char pos){
    if (pos <= NOK_LCD_CIRC_Q1){
        return NOK_LCD_X_CTR_R + (sinArray [pos]/NOK_LCD_ASP_RAT_DENOM);
    }else{
        if (pos < NOK_LCD_CIRC_Q2){
            return NOK_LCD_X_CTR_R + (sinArray[NOK_LCD_CIRC_Q2 - pos]/NOK_LCD_ASP_RAT_DENOM);
        }else{
            if (pos <= NOK_LCD_CIRC_Q3){
                return NOK_LCD_X_CTR_L - (sinArray [pos - NOK_LCD_CIRC_Q2]/NOK_LCD_ASP_RAT_DENOM);
            }else{
                return NOK_LCD_X_CTR_L - (sinArray [NOK_LCD_CIRC_STEPS-pos]/NOK_LCD_ASP_RAT_DENOM);
            }
        }
    }
}

/*************************** nokDrawCircle ***************************************
 * - Draws a circle at radius 24 centered on the screen. Used to outline space where angle will be drawn
 * Arguments: none
 * returns: none
 * Author: Jamie Boyd
 * Date: 2022/02/10 */
void nokDrawCircle (){
    unsigned char ii, xPos, yPos;
    for (ii =0; ii < NOK_LCD_CIRC_STEPS; ii += 1){
        xPos = nokDrawGetX (ii);
        yPos = nokDrawGetY (ii);
        nokLcdSetPixel(xPos, yPos);
    }
}

/*************************** nokDrawAngle ***************************************
 * - Draws a line at an angle proportional to input, from center of screen to circle
 *  saves the previous position so we can "undraw" it
 *  if we wanted to be fancy schmancy we would choose start pixel according to quadrant, a ala draw circle
 * returns: none
 * Author: Jamie Boyd
 * Date: 2022/02/10 */
void nokDrawAngle (signed int encoderCntMod){
    static signed int oldPos = -1;
    signed int pos = (encoderCntMod * NOK_LCD_MAP_NUM)/NOK_LCD_MAP_DENOM;
    unsigned char xPos, yPos;
    if (pos < 0){
        pos += NOK_LCD_CIRC_STEPS;
    }
    if (pos != oldPos){
        xPos = nokDrawGetX ((unsigned char)oldPos);
        yPos = nokDrawGetY ((unsigned char)oldPos);
        nokLcdDrawLine (42, 23, xPos, yPos, 0);
        xPos = nokDrawGetX ((unsigned char)pos);
        yPos = nokDrawGetY ((unsigned char)pos);
        nokLcdDrawLine (42, 23, xPos, yPos, 1);
        oldPos = pos;
    }
}

/*************************** nokDrawBars ***************************************
 * - Draws progress bars based on wheel position. The line width is a bank (8 pixels) wide. One revolution of the flywheel is
 * represented as an activated row on the display. The length of the last row corresponds to angle around the circle
 * returns: none
 * Author: Jamie Boyd
 * Date: 2022/02/12 */
void nokDrawBars (signed long int posCount){
    static signed int lastPos = 0;
    static unsigned char lastY =0;
    static unsigned char lastX =0;

    signed int tempX, tempY, newPos;
    unsigned char newX, newY;
    unsigned char iY;

    tempY = (posCount / COUNTS_PER_REV);   // 6 bars
    tempY = tempY % 6;
    tempX =  posCount % COUNTS_PER_REV;
    tempX = ((tempX * NOK_LCD_PER_BAR_NUM)/NOK_LCD_PER_BAR_DENOM);
    newPos = tempY * 84 + tempX;

    if ((newPos - lastPos) != 0) {
        if (newPos >= 0){  // positive displacement
            if (lastPos < 0){
                nokLcdClear();
                lastPos = 0;
                lastY =0;
                lastX=0;
            }
            newY = tempY;
            newX = tempX;
            if (((newY * 84) + newX) > ((lastY * 84) + lastX)){ // count becoming more positive
                if (newY == lastY){
                    nokIncrBar (newY, lastX, newX);
                }else{
                    nokIncrBar (lastY, lastX, 83);
                    for (iY = lastY+1; iY < newY; iY +=1){
                        nokDrawBar (iY, 1);
                    }
                    nokIncrBar (newY, 0, newX);
                }
            }else{  // count becoming less positive
                if (newY == lastY){
                    nokDecrBar (newY, newX, lastX);
               }else{
                   nokDecrBar (lastY, 0, lastX);
                   for (iY = lastY-1 ; iY > newY; iY -=1){
                       nokDrawBar (iY, 0);
                   }
                   nokDecrBar (newY, newX, 83);
               }
            }
        }else{ // negative displacement
            if (lastPos >= 0){
                nokLcdClear();
                lastPos = 503;
                lastY = 5;
                lastX = 83;
            }
            newY = 5 + tempY;
            newX = 83 + tempX;
            if (((newY * 84) + newX) < ((lastY * 84) + lastX)){ // count becoming more negative
                if (newY == lastY){
                    nokIncrBar (newY, newX, lastX);
                }else{
                    nokIncrBar (lastY, 0, lastX);
                    for (iY = lastY-1; iY > newY; iY -=1){
                        nokDrawBar (iY, 1);
                    }
                    nokIncrBar (newY, newX, 83);
                }
            }else{      // count becoming less negative
                if (newY == lastY){
                    nokDecrBar (newY, lastX, newX);
                }else{
                    nokDecrBar (lastY, lastX, 83);
                    for (iY = lastY + 1 ; iY < newY; iY +=1){
                       nokDrawBar (iY, 0);
                   }
                   nokDecrBar (newY, 0, newX);
                }
            }
        }
        lastPos = newPos;
        lastY =newY;
        lastX=newX;
    }
}

/*************************** simpleRound ***************************************
 * -utility function to round count to degrees not just floor when dividing
 * Arguments: 1
 * argument 1: the count of the encoder, modulus counts_per_rev
 * returns: result of the division, rounded to nearest whole number
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
signed int simpleRound (signed int count){
    signed int rounded;
    if (count < 0){
        rounded = (COUNTS_PER_DEGREE_NUM*(count - COUNTS_PER_HALF_DEGREE))/COUNTS_PER_DEGREE_DENOM;
    }else{
        rounded = (COUNTS_PER_DEGREE_NUM*(count + COUNTS_PER_HALF_DEGREE))/COUNTS_PER_DEGREE_DENOM;
    }
    return rounded;
}
