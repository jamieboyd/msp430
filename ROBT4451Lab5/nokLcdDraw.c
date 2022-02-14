/*
 * nokLcdDraw.c
 *
 *  Created on: Feb. 7, 2022
 *      Author: jamie
 *      NOTE: non standard mapping of angular displacement.
 *      0 degrees position is at the TOP of the screen, not right edge. Positive rotation is in the clockwise direction.
 *
 */

#include "nokLcdDraw.h"
#include "nok5110LCD.h"

//
static unsigned char sinArray [38] = {0, 7, 14, 21, 28, 35, 42, 49, 55, 62, 68, 75, 81,
                               87, 93, 99, 104, 110, 115, 120, 125, 129, 134, 138, 142,
                               145, 149, 152, 155, 157, 160, 162, 164, 165, 166, 167, 168, 168};

// position must be 0 <= pos < NOK_LCD_CIRC_STEPS
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
    if (returnVal < 0){
        returnVal =0;
    }else{
        if (returnVal > 47){
            returnVal = 47;
        }
    }
    return returnVal;
}

// position must be 0 <= pos < NOK_LCD_CIRC_STEPS
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

// draws a circle outlining space where angles will be drawn
void nokDrawCircle (){
    unsigned char ii, xPos, yPos;
    for (ii =0; ii < NOK_LCD_CIRC_STEPS; ii += 1){
        xPos = nokDrawGetX (ii);
        yPos = nokDrawGetY (ii);
        nokLcdSetPixel(xPos, yPos);
    }
}


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


void nokDrawBars (signed long int posCount){
    static signed int lastPos = 0;
    static unsigned char lastY =0;
    static unsigned char lastX =0;

    signed int tempX, tempY, newPos;
    unsigned char newX, newY;
    unsigned char iY;

    tempY = (posCount / COUNTS_PER_REV);   // only 6 bars
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


