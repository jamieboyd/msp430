/*
 * pixyCam2.c - Interface for pixy2 robot vision camera (https://pixycam.com) using I2C protocol
 * with code to get data on a line from the camera, and draw a corresponding line on nokia display
 *
 * Most commands are 2 parters, a command that queries the camera, and dumps returned data in a buffer for processing by a wrapper
 * function whose name ends in CMD. Wrapper function formats and prints or draws
 *
 *
 *  Created on: Feb. 23, 2022
 *      Author: jamie
 */

#include <msp430.h>
#include "pixyCam2.h"   // functions for camera
#include <libUART1A.h>  // library to configure and use serial port
#include "usciB0I2C.h"  // I2C reading and writing on usciB0, using pins P3.0 and P3.1 - cause 4.2 and 4.2 are used by nokia display
#include <stdio.h>      // for sprintf function
#include "nok5110LCD.h" // for drawing to the screen
#include "nok5110LCD.h"

volatile unsigned char PIXY_LINE_STOP;   // used to communicate between interrupts drawing the line and waiting for the stop signal
volatile unsigned char  PIXY_LINE_DRAW;

/*************************** pixyInit ***************************************
 * - Initializes i2c bus and a timer used to time data grabs
 * Arguments: none
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/27 */
void pixyInit(void){
    usciB0I2CInit (PIXY_CLOCK_DIV); // initialize I2C
    // initialize timer for drawCMD with interrupt every 20 ms, but don't enable till we want it
    TA0CTL = TASSEL_2 | ID_0 | MC_1 ;
    TA0CCR0 = _FRAME_TIME_CNT - 1;           // set timer interval to 20 ms
    TA0CTL &= ~TAIFG;                       // clear interrupt flag
    usciA1UartInstallRxInt (&pixySetStopRxInt); // install but don't enable function that checks for the return stroke for drawCMD
}

/*************************** pixyGetVersionCMD ***************************************
 * - for Command parser, gets version and sends it to the host, nicely formatted
 * Arguments: none
 * returns: 0 for successs, -1 if there is an error in communication
 * Author: Jamie Boyd
 * Date: 2022/02/27 */
signed char pixyGetVersionCMD (){
    unsigned char PIXY_VERSION_RESPONSE_DATA [22];
    char msgStr [50];
    signed char err = pixyGetVersion(PIXY_VERSION_RESPONSE_DATA);
    if (err){
        return err;
    }
    unsigned int tempVar = PIXY_VERSION_RESPONSE_DATA [6] + PIXY_VERSION_RESPONSE_DATA [7] * 0x100;
    sprintf ((char *)msgStr,"\r\nHardware Version = %d\r\n\0", tempVar);  // version
    usciA1UartTxString (msgStr);
    sprintf ((char *)msgStr,"Firmware Version = %d.%d\r\n\0", PIXY_VERSION_RESPONSE_DATA [8], PIXY_VERSION_RESPONSE_DATA [9]);  // version
    usciA1UartTxString (msgStr);
    tempVar = PIXY_VERSION_RESPONSE_DATA [10] + PIXY_VERSION_RESPONSE_DATA [11] * 0x100;
    sprintf ((char *)msgStr,"Firmware Build Num = %d\r\n\0", tempVar);
    usciA1UartTxString (msgStr);
    sprintf ((char *)msgStr,"Firmware Type= %s\r\n\0", PIXY_VERSION_RESPONSE_DATA + 12);
    usciA1UartTxString (msgStr);
    return 0;
}

/*************************** pixyGetVersion ***************************************
 * - queries camera and gets version information
 * Arguments: 1
 * rxBuffer - buffer to receive data from camera
 * returns: 0 for successs, -1 if there is an error in communication
 * Author: Jamie Boyd
 * Date: 2022/02/27 */
signed char pixyGetVersion(unsigned char * rxBuffer){
    static unsigned char PIXY_VERSION_REQUEST_DATA [4] = {PIXY_SYNC_SEND0, PIXY_SYNC_SEND1, PIXY_VERSION_REQUEST, 0};
    signed char returnVal = usciB0I2CMstTransmit (PIXY_VERSION_REQUEST_DATA, 4, PIXY_ADDRESS);
    if (returnVal == 0){
        returnVal = usciB0I2CMstReceive (rxBuffer, 22,  PIXY_ADDRESS);
    }
return returnVal;
}

/*************************** pixyGetFPSCMD ***************************************
 * - for Command parser, prints frames/second data from camera
 * Arguments: none
 * returns: -1 if an error in communication, else 0
 * Author: Jamie Boyd
 * Date: 2022/02/27 */
signed char pixyGetFPSCMD(void){
    unsigned char PIXY_FPS_RESPONSE_DATA [10] = {0,0,0,0,0,0,0,0,0,0}; // for debugging, zero data before calling function.
    char msgStr [50];
    signed char err = pixyGetFPS(PIXY_FPS_RESPONSE_DATA);
    if (err){
        return err;
    }
    unsigned long int FPSval = PIXY_FPS_RESPONSE_DATA [6] + PIXY_FPS_RESPONSE_DATA [7] * 0x100 + PIXY_FPS_RESPONSE_DATA [8] * 0x10000 + PIXY_FPS_RESPONSE_DATA [9]*0x1000000;
    sprintf ((char *)msgStr,"FPS = ");
    usciA1UartTxString (msgStr);
    usciA1UartTxLongInt (FPSval);
    usciA1UartTxChar ('\r');
    usciA1UartTxChar ('\n');
    return 0;
}

/*************************** pixyGetFPS ***************************************
 * queries camera and gets frames per second data
 * Arguments: 1
 * rxBuffer: buffer to put returned data in
 * returns -1 if an error in communication, else 0
 * Author: Jamie Boyd
 * Date: 2022/02/28  */
signed char pixyGetFPS (unsigned char * rxBuffer){
    static unsigned char PIXY_FPS_REQUEST_DATA [4] = {PIXY_SYNC_SEND0, PIXY_SYNC_SEND1, PIXY_FPS_REQUEST, 0};
    signed char err = usciB0I2CMstTransmit (PIXY_FPS_REQUEST_DATA, 4, PIXY_ADDRESS);
    if (err == 0){
        err = usciB0I2CMstReceive (rxBuffer, 10,  PIXY_ADDRESS);
    }
    return err;
}

/*************************** pixySetLampCMD ***************************************
 * Asks camera to turn on lamps above/below screen
 * what a chatty protocol. Camera sends us back 10 bytes. Not going to print them over UART
 * With no UART stuff, no need for separate wrapper/i2c functions for "separation of concerns"
 * Arguments: 2
 * 1) upperON - turn on (1) or off (0) upper lights
 * 2) lowerON - turn on (1) or off (0) lower lights
 * returns -1 if an error in communication, else 0
 * Author: Jamie Boyd
 * Date: 2022/03/01 */
signed char pixySetLampCMD(unsigned char upperON, unsigned char lowerON){
    unsigned char PIXY_LAMP_REQUEST_DATA [6] = {PIXY_SYNC_SEND0, PIXY_SYNC_SEND1,PIXY_LAMP_REQUEST, 2, (upperON), (lowerON)};  // turn em on or off with last 2 values
    unsigned char PIXY_LAMP_RESPONSE_DATA [10]; // not going to look at these now, but may have diagnostic values if lights do not, in fact, turn on
    signed char err = usciB0I2CMstTransmit (PIXY_LAMP_REQUEST_DATA, 6, PIXY_ADDRESS);
    if (err ==0){
        err = usciB0I2CMstReceive(PIXY_LAMP_RESPONSE_DATA, 10,PIXY_ADDRESS); // so we even have to read these back?
    }
    return err;
}

/*************************** pixyGetVectorCMD ***************************************
 * Gets a vector from camera and prints it to the console, and/or displays it on the NOKIA screen
 * can display the line drawn between endpoints, or display just the angle, losing position information
 * Can also do this once, or continuously from a timer. This is a command interpteter thing, so we actually
 * DONT want to return until we get the return key pressed, else it could get confusing for the user
 * so we have a timer running while we poll for it
 * Arguments: 2
 * 1) doContinuous - sets a continuous update mode with timer, ended when enter is pressed on console
 * 2) displayMode - set bit 0 (1) for text updates, set bit 1 (2) for nokia screen updates, set bit 2 (4)for angle. clear bit 2 for position mode
 * returns -1 if an error in communication, else 0
 * Author: Jamie Boyd
 * Date: 2022/03/01 */
signed char pixyGetVectorCMD( unsigned char doContinuous, unsigned char displayMode){
    static unsigned char PIXY_VECT_RESPONSE_DATA [15]; // room for 9 bytes of description plus 6 bytes for ONE vector. We do not read further vectors
    signed char err;
    char msgStr [50];
    char emptyMsgStr [] = {32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,13};
    if (doContinuous){
        PIXY_LINE_STOP = 0;
        TA0CTL |= TAIE; // enable timer interrupt for timing draw events
        usciA1UartEnableRxInt (1);  // enable UART interrupt for return key
    }else{
        PIXY_LINE_STOP = 1;     // so we only do it once, not continuous
    }
    PIXY_LINE_DRAW = 1;         // so we do it at least once, and always right away
    if (displayMode & BIT0){    // start on a new line, we will overwrite values every time in contunuous mode
        usciA1UartTxChar ('\r');
        usciA1UartTxChar ('\n');
    }
    PIXY_LINE_DRAW = 1; // so we always draw at least once
    do{
        while (!PIXY_LINE_DRAW){}; // wait for timer to go off again. could put ourselves to sleep here, for power saving
        PIXY_LINE_DRAW = 0; // reset timer signal variable

        err= pixyGetVector(PIXY_VECT_RESPONSE_DATA);
        if (err < 0) break;  // something bad happened
        // NOTE: we only do the FIRST vector we get passed back. For now.
        if (displayMode & BIT2){ // report angles
            if (displayMode & BIT0){    // text requested

            }
            if (displayMode & BIT1){    // drawing requested
            }
        }else {  // report end-point positions
            if (displayMode & BIT0){    // text requested
                sprintf ((char *)msgStr,"X1=%d\tY1=%d\tX2=%d\tY2=%d\r", PIXY_VECT_RESPONSE_DATA [8], PIXY_VECT_RESPONSE_DATA [9], PIXY_VECT_RESPONSE_DATA [10], PIXY_VECT_RESPONSE_DATA [11]);
                usciA1UartTxString (emptyMsgStr);  // this is ugly. But %03d is too complicated for code composer studio
                usciA1UartTxString (msgStr);
            }
            if (displayMode & BIT1){    // nokia drawing of line between endpoints requested
                pixyDrawPos (PIXY_VECT_RESPONSE_DATA [8], PIXY_VECT_RESPONSE_DATA [9], PIXY_VECT_RESPONSE_DATA [10], PIXY_VECT_RESPONSE_DATA [11]);
            }
        }
    } while (!(PIXY_LINE_STOP)); // use do-while so we always get at least one datum
    // Now we are done with drawing, one way or another
    if (err < 0){ // not a normal stop event, so we have to shut down interrupts
        usciA1UartEnableRxInt (0); // stop UART interrupt
        TA0CTL &= ~TAIE; // disable timer interrupt
    }
    if (displayMode & BIT0){    // give next command a new line to start on
       usciA1UartTxChar ('\r');
       usciA1UartTxChar ('\n');
   }
    return err;
}

/*************************** pixyGetVector ***************************************
 * queries camera and gets data for first vector. There may be data for more than one vector, but we only get the first one
 * Do we need to get them all to prevent stale data in a camera buffer or out-of-order processing? don't think so
 * Arguments: 1
 * rxBuffer: buffer to put returned data in
 * returns: -1 if an error in communication, else 0
 * Author: Jamie Boyd
 * Date: 2022/02/28  */
signed char pixyGetVector(unsigned char * rxBuffer){
    static unsigned char PIXY_VECT_REQUEST_DATA [6] = {PIXY_SYNC_SEND0, PIXY_SYNC_SEND1, PIXY_MAIN_REQUEST, 2, PIXY_MAIN_FEATURES, PIXY_VECTOR_REQUEST};
    signed char err;
    err = usciB0I2CMstTransmit (PIXY_VECT_REQUEST_DATA, 6, PIXY_ADDRESS);
    if (err == 0){
        err = usciB0I2CMstReceive (rxBuffer, 15,  PIXY_ADDRESS);
        if (err ==0){
            if (rxBuffer [7] < 6){  // not enough data for one vector
                err =-1;
            }
         }
    }
    return err;
}

/*************************** pixyDrawPos ***************************************
 * - draws a line from Pixy Data, scrunching data to ends of screen. Not particularly elegant approach
 *  but takes advantage of the near 1:1 mapping of camera space (78:51) to screen space (84:48),
 *  after accounting for non-square screen pixels
 *  NOTE: the camera pixels appear to be pretty much square
 *  saves the previous position so we can "undraw" it
 *  Arguments: the 4 camera coordinates
 * returns: none
 * Author: Jamie Boyd
 * Date: 2022/03/01 */
void pixyDrawPos (unsigned char px1, unsigned char py1, unsigned char px2, unsigned char py2){
    static unsigned char lastx1, lastx2, lasty1, lasty2 ; // for cleaning up after last point. Should maybe let them ghost, that would look cool
    unsigned char nx1 = (px1 * 7)/6;        // some Nokia specific knowledge creeping in here. Should use constants, or put this code in NokDraw
    if (nx1 > 83){
        nx1 = 83;
    }
    unsigned char nx2 = (px2 * 7)/6;
    if (nx2 > 83){
        nx2 = 83;
    }
    unsigned char ny1 = py1;
    if (ny1 > 47) ny1 = 47;
    unsigned char ny2 = py2;
    if (ny2 > 47) ny2 = 47;

    if (!((((nx1 == lastx1) && (nx2 == lastx2)) && (ny1 == lasty1)) && (ny2 == lasty2))){
        nokLcdDrawLine (lastx1, lasty1, lastx2, lasty2, 0);
        nokLcdDrawLine (nx1, ny1, nx2, ny2, 1);
        lastx1 = nx1;lastx2=nx2; lasty1 = ny1; lasty2 = ny2;
    }
}



void pixySetStopRxInt (char theChar){
    if (theChar == '\r'){   // or we could respond to ANY character, always wanted an excuse to say "press any key to continue"
        PIXY_LINE_STOP = 1;      // one and done kinda thing
        usciA1UartEnableRxInt (0); // stop UART interrupt
        TA0CTL &= ~TAIE; // disable timer interrupt
    }
}


/*************************** timer0A1Isr ***************************************
* -timer interrupt just tells you to do a draw */
#pragma vector = TIMER0_A1_VECTOR
__interrupt void timer0A1Isr(void) {
    PIXY_LINE_DRAW =1;              // time to draw the line
    TA0CTL &= ~TAIFG;               // clear TAIFG interrupt flag
}
