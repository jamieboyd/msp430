/*
 * ls7366TestBench.c
 *
 *  Created on: Feb. 7, 2022
 *      Author: jamie
 */

#include <fedi.h>
#include <msp430.h>
#include <libUART1A.h>
#include "nok5110LCD.h"
#include "nokLcdDraw.h"
#include "LS7366R.h"
#include "libCmdInterp.h"
#include "usciSpi.h"
#include "stdio.h"


char msgStr [50];

/**
 * main.c
 */

    unsigned char dataIn [4];           // data comes in as 4 bytes from the 4 byte counter
    signed long int posCount;           // but we want a single signed count value
    unsigned int result;                // error code
    unsigned int drawBarsNotAngle = 1;              // for drawing
    signed long int homePos = 0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;                                       // stop watchdog timer
    usciA1UartInit(19200);                                          // initialize UART for 19200 Baud communication
    usciB1SpiInit(1, 4, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);  // initialize SPI communication
    nokLcdInit();                                                   // initialize NOKIA LCD
    nokLcdClear();
    LS7366Rinit();
    LS7366Rclear(CNTR);


#ifdef UART_CONTROL
    CMD cmdList [MAX_CMDS];                 // array of commands that we know how to process
    int cmdIndex;                           // position of current command in command list, -1 for error
    gCmdCnt = 1;                            // humans like to start at 1 not 0
    initCmds (cmdList);                     // initialize command list from #defines
    char gCmdLineBuffer [CMD_LEN];          // a simple garden-variety string


    TA0CTL = TASSEL_2 | ID_0 | MC_1 | TAIE;
    TA0CCR0 = _50MS_CNT - 1; // set rollover interval to 50 ms
    TA0CTL &= ~TAIFG;
    _enable_interrupts();
#endif



#ifdef CONSOLE_TEST
    signed int nRotations;      // + or - from 0
    signed int angle;           // + or - from 0, up to 360
#ifdef CONSOLE_TEST_ANG
    nokDrawCircle ();
    nokDrawAngle (0);
#endif
#endif

    while (1){
#ifdef CONSOLE_TEST
      result=LS7366Rread(CNTR, dataIn); // puts count onto the SPI bus
      posCount = *(long int*)dataIn;
#ifdef CONSOLE_TEST_CNT                 // barfs out raw encoder counts formatted better than minimal sprintf allows
      usciA1UartTxLongInt (posCount);   // does not use sprintf, does its own conversion.
      usciA1UartTxChar ('\r');          // return but don't do a new line, so is contantly update
#endif

#ifdef CONSOLE_TEST_ANG
      nRotations = posCount/COUNTS_PER_REV;
      angle = posCount % COUNTS_PER_REV;
      sprintf ((char *)msgStr,"Rev: %d Angle: %d deg      \r", nRotations, angle/COUNTS_PER_DEGREE);
      usciA1UartTxString (msgStr);
      nokDrawAngle (angle);
#endif

#ifdef CONSOLE_TEST_BARS
      nRotations = posCount/COUNTS_PER_REV;
      angle = posCount % COUNTS_PER_REV;
      sprintf ((char *)msgStr,"Rev: %d Angle: %d deg      \r", nRotations, angle/COUNTS_PER_DEGREE);
      usciA1UartTxString (msgStr);
      nokDrawBars (posCount);
#endif
#endif

#ifdef UART_CONTROL
      sprintf ((char *)msgStr,"CMD %d:\0", gCmdCnt);  // display a prompt for user
      usciA1UartTxString (msgStr);        // prompt user for a command
      if (usciA1UartGets (gCmdLineBuffer) == NULL){    //user entered too many characters
        gError = 1;
        printErr ();
        continue;
      }
      cmdIndex = parseCmd(cmdList, gCmdLineBuffer);
      if (cmdIndex ==-1){
          printErr ();
          continue;
      }
      cmdIndex = executeCmd(cmdList, cmdIndex);
      printErr ();  // error from executeCmd, or no error
      if (cmdIndex != -1){
          gCmdCnt +=1;            // we only count commands that succeed in this version
      }
#endif
    }
    return 0;
}


char fediHome (signed long int posCount){
    _disable_interrupts();
    unsigned char dummydat [4];
    signed long int * posCountLocal = (signed long int *)&dummydat;
    *posCountLocal = posCount;
    LS7366Rwrite(DTR, dummydat);
    LS7366Rload (CNTR);

    homePos = posCount;
    _enable_interrupts();
    return 0;

}

void fediClr (void){
    nokLcdClear();
    if (drawBarsNotAngle ==0){
        nokDrawCircle ();
    }else{
        nokDrawBars (0);
    }
}

char fediRead (unsigned char reg){
    _disable_interrupts();
    unsigned char err = LS7366Rread(reg, dataIn);
    if (!err){
        if(reg==MDR0||reg==MDR1||reg==STR) {// you're reading an 1 byte register
            sprintf ((char *)msgStr,"\tRegister contains %d", dataIn[0]);
            usciA1UartTxString (msgStr);
        }else{ // 4 byte register
            sprintf ((char *)msgStr,"\tRegister contains ");
            usciA1UartTxString (msgStr);
            usciA1UartTxLongInt (*(long int*)dataIn);
        }

    }
    _enable_interrupts();
    return err;
}

char fediDisp (unsigned char mode){
    char rVal = 1;
    if (mode < 2){
        nokLcdClear();
        if (mode ==0){
            nokDrawCircle ();
        }else{
            nokDrawBars (0);
        }
        drawBarsNotAngle = mode;
        rVal = 0;
    }
    return rVal;
}



void fediFw (void){
    signed int nRotations = posCount/COUNTS_PER_REV;
    signed int angle = posCount % COUNTS_PER_REV;
    sprintf ((char *)msgStr,"\tRev: %d Angle: %d deg", nRotations, angle/COUNTS_PER_DEGREE);
    usciA1UartTxString (msgStr);
}

void fediZero (void){
    LS7366Rclear(CNTR);
    homePos = 0;
}


/*  Define one of the TIMER A0's interrupt vectors
    Remember there are two ISR vectors for each timer.  They are defined in msp430F5529.h
    TIMER0_A1_VECTOR = 0xFFE8. Understand why this vector and not the other !! */
    #pragma vector = TIMER0_A1_VECTOR
    __interrupt void timer0A1Isr(void) {
        /*   count is static and will persist for the life time of main.c.  Only accessible by function it is defined in
            Its useful for ISR's */
        static unsigned int count = 0;
        if ((count % 2) ==0){
            result=LS7366Rread(CNTR, dataIn); // puts count onto the SPI bus
            posCount = *(long int*)dataIn - homePos;
        }
        if ((count % 5) == 0){
            if (drawBarsNotAngle){
                nokDrawBars (posCount);
            }else{
                nokDrawAngle ( posCount % COUNTS_PER_REV);

            }
        }
        count++; // increment counter.
        if (count == 65530){
            count = 0;
        }

        TA0CTL &= ~TAIFG;  // clear TAIFG interrupt flag. It will not clear automatically unless you do this
    }
