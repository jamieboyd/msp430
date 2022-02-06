/*************************************************************************************************
 * nok5110LCD.c
 * - C implementation or source file for NOKIA 5110 LCD.
 *
 *  Author: Greg Scutt
 *  Created on: Feb 20th, 2017
 **************************************************************************************************/


// nok5110LCD pin connectivity --> to MSP-EXP430F5529LP EVM.
//  8-LIGHT  	-->  	no connection necessary
//  7-SCLK  	-->  	MS430EVM  P4.3 or UCB1CLK
//  6-DN(MOSI)  -->  	MS430EVM  P4.1 or UCB1SIMO
//  5-D/C'  	-->  	MS430EVM  P4.2. 	Kept as I/O pin !!
//  4-RST'  	-->  	MS430EVM or supply VSS
//  3-SCE'  	-->  	MS430EVM  P4.0.  	Kept as I/O pin !!
//  2-GND  		-->  	MS430EVM or supply VSS
//  1-VCC  		-->  	MS430EVM or supply 3V3


#include <msp430.h>
#include "nok5110LCD.h"
#include "usciSpi.h"

// 2-D 84x6 array that stores the current pixelated state of the display.
// remember a byte (8 bits) sets 8 vertical pixels in a column allowing 8x6=48 rows
// note that this array is GLOBAL to this file only. In that way it is protected from access from other functions in files.
// said another way - it is a private global array with local scope to the file in which the defining declaration exists.
// we don't want other functions messing with the shadow RAM. This is the reason for static and for its dec/defn in the .c file
static unsigned char currentPixelDisplay[LCD_MAX_COL][LCD_MAX_ROW / LCD_ROW_IN_BANK];

/************************************************************************************
* Function: nokLcdWrite
* - performs write sequence to send data or command to nokLCD. Calls spiTxByte to transmit serially to nokLCD
* argument:
* Arguments: lcdByte - the 8 bit char (data or command) that is written to nokLCD.
* 			 cmdType - 0 - lcdByte is a cmd,   1 - lcdByte is data.
* return: none
* Author: Greg Scutt
* Date: Feb 20th, 2017
* Modified: <date of any mods> usually taken care of by rev control
************************************************************************************/
void nokLcdWrite(char lcdByte, char cmdType) {
	// check cmdType and output correct DAT_CMD signal to PORT4 based on it.
    if (cmdType == DC_DAT){    // Data NOT Command
        P4OUT |= DAT_CMD;
    }else{
        P4OUT &= ~DAT_CMD;
    }
	// activate the SCE. the chip select
    P4OUT &= ~SCE;     // active low

	// transmit lcdByte with spiTxByte
    UCB1IFG &= ~UCRXIFG;              // Clear receive flag cause it will be set because we never read RXBUF
    spiTxByte (lcdByte);
	// wait for transmission  complete ?  If so, disable the SCE
    while (!(UCB1IFG & UCRXIFG)){};   // poll, waiting while last bit in last byte to be received. Now transfer is complete
    UCB1IFG &= ~UCRXIFG;              // Always a good idea to clear the flag
    P4OUT |= SCE;                  // active low
}

/************************************************************************************
* Function: nokLcdInit
* -
* argument:
*	none
* return: none
* Author: Greg Scutt
* Date: Feb 20th, 2017
* Modified: <date of any mods> usually taken care of by rev control
************************************************************************************/
void nokLcdInit(void) {


	// gScutt.  do an SPI init with ucsiB1SpiInit  from ucsiSpi.h before this function call !!
    P4SEL &= ~SCE;
    P4SEL &= ~DAT_CMD;
    P4DIR |= SCE;   // set pins to output
    P4DIR |= DAT_CMD;
    P4OUT |= SCE;   // 4.0
    P4OUT |= DAT_CMD; // 4.2
    P4OUT |= (SCE | DAT_CMD);	// Set DC' low - which means command not data, and CE Low which means chip select
	// send initialization sequence to LCD module
	nokLcdWrite(LCD_EXT_INSTR, DC_CMD);     // enables high level command set, horizontal addressing, power ON
	nokLcdWrite(LCD_SET_OPVOLT, DC_CMD);    // low 6 bits sets voltage for contrast control, 5,4,3, and 2 are set, 60 out of 63
	nokLcdWrite(LCD_SET_TEMPCTRL, DC_CMD);  //sets temperature coefficient but constant was wrong in header
	nokLcdWrite(LCD_SET_SYSBIAS, DC_CMD);   // sets bias bit 0 and 2
	nokLcdWrite(LCD_BASIC_INSTR, DC_CMD);   // switch to basic instruction, data and addresses
	nokLcdWrite(LCD_NORMAL_DISP, DC_CMD);   // normal display mode, not cleared

	nokLcdClear(); // clear the display
}

void nokLcdClear(void){
    nokLcdWrite(LCD_SET_YRAM, DC_CMD);
    nokLcdWrite(LCD_SET_XRAM, DC_CMD);
    unsigned int iPos;
    unsigned int endPos = LCD_MAX_COL * (LCD_MAX_ROW / LCD_ROW_IN_BANK);
    unsigned char yPos;
    unsigned xPos;

    P4OUT &= ~SCE;     // chip select active low
    P4OUT |= DAT_CMD;   // lotsa data to follow
  for (iPos =0; iPos < endPos; iPos +=1){
        //nokLcdWrite(0, DC_DAT);
        spiTxByte (0);
    }
    while (!(UCB1IFG & UCTXIFG)){};
    UCB1IFG &= ~UCRXIFG;              // Clear receive flag cause it will be set because we never read RXBUF
    while (!(UCB1IFG & UCRXIFG)){};   // poll, waiting while last bit in last byte to be received. Now transfer is complete
    UCB1IFG &= ~UCRXIFG;              // Always a good idea to clear the flag
    P4OUT |= SCE;                  // chip select active low

    for (yPos =0; yPos < (LCD_MAX_ROW / LCD_ROW_IN_BANK); yPos +=1){
        for (xPos =0; xPos < LCD_MAX_COL; xPos +=1){
            currentPixelDisplay[xPos][yPos]=0;
        }
    }

}

/************************************************************************************
* Function: nokLcdSetPixel
* -
* argument:
*	xPos - The horizontal pixel location in the domain (0 to 83)
*	yPos - The vertical pixel location in the domain (0 to 47)
*
* return: 0 - pixel was valid and written.  1 - pixel not valid
* Author: Greg Scutt
* Date: Feb 20th, 2017
* Modified: <date of any mods> usually taken care of by rev control
************************************************************************************/
unsigned char  nokLcdSetPixel(unsigned char xPos, unsigned char yPos) {
	unsigned char bank; // a bank is a group of 8 rows, selected by 8 bits in a byte

	// verify pixel position is valid
	if ((xPos < LCD_MAX_COL) && (yPos < LCD_MAX_ROW)) {

		// if-else statement avoids costly division
		if (yPos<8) bank = 0;
		else if (yPos<16) bank = 1;
		else if (yPos<24) bank = 2;
		else if (yPos<32) bank = 3;
		else if (yPos<40) bank = 4;
		else if (yPos<48) bank = 5;

		// set the x and y RAM address  corresponding to the desired (x,bank) location. this is a command DC_CMD
		nokLcdWrite(LCD_SET_XRAM | xPos, DC_CMD);
		nokLcdWrite(LCD_SET_YRAM | bank, DC_CMD);

		// update the pixel being set in currentPixelDisplay array
		currentPixelDisplay[xPos][bank] |= BIT0 << (yPos % LCD_ROW_IN_BANK); // i.e if yPos = 7 then BIT0 is left shifted 7 positions to be 0x80. nice
		nokLcdWrite(currentPixelDisplay[xPos][bank], DC_DAT); // write the data. this is DATA DC_DAT
		return 0;
	}
	return 1;
}

/************************************************************************************
* Function: nokLcdClearPixel
* -
* argument:
*   xPos - The horizontal pixel location in the domain (0 to 83)
*   yPos - The vertical pixel location in the domain (0 to 47)
*
* return: 0 - pixel was valid and written.  1 - pixel not valid
* Author: Greg Scutt
* Date: Feb 20th, 2017
* Modified: <date of any mods> usually taken care of by rev control
************************************************************************************/
unsigned char nokLcdClearSetPixel(unsigned char xPos, unsigned char yPos, unsigned char isClearNotSet) {
    unsigned char bank; // a bank is a group of 8 rows, selected by 8 bits in a byte

    // verify pixel position is valid
    if ((xPos < LCD_MAX_COL) && (yPos < LCD_MAX_ROW)) {

        // if-else statement avoids costly division
        if (yPos<8) bank = 0;
        else if (yPos<16) bank = 1;
        else if (yPos<24) bank = 2;
        else if (yPos<32) bank = 3;
        else if (yPos<40) bank = 4;
        else if (yPos<48) bank = 5;

        // set the x and y RAM address  corresponding to the desired (x,bank) location. this is a command DC_CMD
        nokLcdWrite(LCD_SET_XRAM | xPos, DC_CMD);
        nokLcdWrite(LCD_SET_YRAM | bank, DC_CMD);

        // update the pixel being set in currentPixelDisplay array
        if (isClearNotSet){
            currentPixelDisplay[xPos][bank] &= ~(BIT0 << (yPos % LCD_ROW_IN_BANK)); // i.e if yPos = 7 then BIT0 is left shifted 7 positions to be 0x80. nice
        }else{
            currentPixelDisplay[xPos][bank] |= (BIT0 << (yPos % LCD_ROW_IN_BANK)); // i.e if yPos = 7 then BIT0 is left shifted 7 positions to be 0x80. nice
        }
        nokLcdWrite(currentPixelDisplay[xPos][bank], DC_DAT); // write the data. this is DATA DC_DAT
        return 0;
    }
    return 1;
}

signed char nokLcdDrawScrnLine (unsigned char linePos, unsigned char isVnotH){
    unsigned char bank;
    unsigned char yByte;
    unsigned char xPos;
    if (isVnotH){
        if (linePos > 83){
            return -1;
        }
        nokLcdWrite(34, DC_CMD); // vertical line, easy peasy
        nokLcdWrite(LCD_SET_YRAM, DC_CMD);
        nokLcdWrite(LCD_SET_XRAM + linePos, DC_CMD);
        for (bank =0; bank < 6; bank +=1){
            currentPixelDisplay[linePos][bank] = 255;
            nokLcdWrite(255, DC_DAT);
        }
    }else{
        if (linePos > 47){
            return -1;
        }
        nokLcdWrite(32, DC_CMD); // horizontal line, trickier
        if (linePos<8) bank = 0;
        else if (linePos<16) bank = 1;
        else if (linePos<24) bank = 2;
        else if (linePos<32) bank = 3;
        else if (linePos<40) bank = 4;
        else if (linePos<48) bank = 5;
        yByte = BIT0 << (linePos % LCD_ROW_IN_BANK);
        nokLcdWrite(LCD_SET_YRAM + bank, DC_CMD);
        for (xPos =0; xPos < LCD_MAX_COL; xPos +=1){
            currentPixelDisplay [xPos][bank] |= yByte;
            nokLcdWrite(currentPixelDisplay [xPos][bank], DC_DAT);
        }
    }
    return 0;
}

signed char nokLcdDrawLine (unsigned char xStart, unsigned char yStart, unsigned char xEnd, unsigned char yEnd){

    if ((xStart > 83) || (xEnd > 83) ||  (yStart > 47) || (yEnd < 47)){
        return -1;
    }
    signed char xPos;    // use ints to guarantee promotion for intermediate math
    signed char yPos;
    signed char rise = yEnd - yStart;
    signed char run = xEnd - xStart;
    signed int kindaB = (xEnd * yStart) - (xStart * yEnd); // equivalent to b/run from  y = (rise/run)*x + b
    signed char runAbs = run;   // absolute value of run, for comparison
    signed char riseAbs = rise;  // absolute value of rise, for comparison
    signed char incr = 1;
    if (run < 0){
        runAbs *= -1;
    }
    if (rise < 0){
        riseAbs *= -1;
    }
    if (runAbs > riseAbs){  // more pixels for run than for rise
        if (xStart > xEnd){
            incr = -1;
        }
        for (xPos = xStart; xPos != xEnd; xPos += incr){
            yPos = ((xPos * rise) + kindaB)/run;
            nokLcdSetPixel((unsigned char) xPos, (unsigned char) yPos);
        }
    }else{      // more pixels for rise than for run
        kindaB *= -1;
        if (yStart > yEnd){
            incr = -1;
        }
        for (yPos = yStart; yPos != yEnd; yPos += incr){
           xPos = ((yPos * run) + kindaB)/rise;
           nokLcdSetPixel((unsigned char) xPos, (unsigned char) yPos);
        }
    }
    return 0;
}

/************************************************************************************
* Function: spiTxByte
* - if TXBUFFER is ready!!  then a byte is written to buffer and serialized on SPI UCB1. nothing else happens.
* argument:
*	txData - character to be serialized over SPI
*
* return: none
* Author: Greg Scutt
* Date: Feb 20th, 2017
* Modified: <date of any mods> usually taken care of by rev control
************************************************************************************/
void spiTxByte(char txData)
{
	// transmit a byte on SPI if it is ready !!
    while (!(UCB1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
        UCB1TXBUF = txData;  // write the byte to the TX buffer register
}


