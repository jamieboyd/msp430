/*
 *  fedi.c
 *  C interface file for the Friesen Encoder Display Interface
 *  Created on: 2022/02/12 by Jamie Boyd
 *      Author: jamie
 */
#include <fedi.h>

    // globals. accessed from interrupt and from main
    char msgStr [50];
    unsigned char dataIn [4];           // data comes in as 4 bytes from the 4 byte counter
    signed long int posCount;           // but we want a single signed count value
    unsigned int result;                // error code
    unsigned int drawBarsNotAngle = 1;              // for drawing
    signed long int homePos = 0;

 // main function for fedi. does some init stuff, then loops
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;                                       // stop watchdog timer
    usciA1UartInit(19200);                                          // initialize UART for 19200 Baud communication
    usciB1SpiInit(1, 4, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);  // initialize SPI communication
    nokLcdInit();                                                   // initialize and clear NOKIA LCD
    nokLcdClear();
    LS7366Rinit();                                                  // initialize and clear encoder
    LS7366Rclear(CNTR);

// initialization for UART command interpreter, the real fedi
#ifdef UART_CONTROL
    CMD cmdList [MAX_CMDS];                 // array of commands that we know how to process
    int cmdIndex;                           // position of current command in command list, -1 for error
    gCmdCnt = 1;                            // humans like to start at 1 not 0
    initCmds (cmdList);                     // initialize command list from #defines
    char gCmdLineBuffer [CMD_LEN];

    // initialize timer with interrupt every 50 ms
    TA0CTL = TASSEL_2 | ID_0 | MC_1 | TAIE;
    TA0CCR0 = _50MS_CNT - 1;                // set timer interval to 50 ms
    TA0CTL &= ~TAIFG;                       // clear flag
    _enable_interrupts();                   // enable interrupts
#endif

// initialize for function testing
#ifdef CONSOLE_TEST
    signed int nRotations;      // + or - from 0
    signed int angle;           // + or - from 0, up to 360
#ifdef CONSOLE_TEST_ANG
    nokDrawCircle ();
    nokDrawAngle (0);
#endif
#endif


    while (1){      // main program loop
#ifdef CONSOLE_TEST                      // for testing, enable CONSOLE_TEST and one of the sub-test defines
      result=LS7366Rread(CNTR, dataIn); // puts count onto the SPI bus
      posCount = *(long int*)dataIn;

#ifdef CONSOLE_TEST_CNT                 // barfs out raw encoder counts formatted better than minimal sprintf allows
      usciA1UartTxLongInt (posCount);   // does not use sprintf, does its own conversion.
      usciA1UartTxChar ('\r');          // return but don't do a new line, so is contantly update
#endif

#ifdef CONSOLE_TEST_ANG                     // displays encoder angle continuously and also prints to console
      nRotations = posCount/COUNTS_PER_REV;
      angle = posCount % COUNTS_PER_REV;
      sprintf ((char *)msgStr,"Rev: %d Angle: %d deg      \r", nRotations, fediRound(angle));
      usciA1UartTxString (msgStr);
      nokDrawAngle (angle);
#endif

#ifdef CONSOLE_TEST_BARS                    // displays progress bar,updated constantly, and prints to console
      nRotations = posCount/COUNTS_PER_REV;
      angle = posCount % COUNTS_PER_REV;
      sprintf ((char *)msgStr,"Rev: %d Angle: %d deg      \r", nRotations, fediRound(angle));
      usciA1UartTxString (msgStr);
      nokDrawBars (posCount);
#endif
#endif // end of CONSOLE_TEST block

#ifdef UART_CONTROL     // command interpreter, used for the real fedi. With encoder check and display called from interrupt
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
/****************************** fedi interface functions, can be called from command interpreter***********************/

/*************************** fediHome ***************************************
 * - initializes CTR to user-provided value, clears LCD display, zeroes angular displacement.
 * - Example: fediHome 32767
 * Arguments: 1
 * argument 1: count to copy to the encoder. basically says "here" is now to be known as "there"
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
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

/*************************** fediClr ***************************************
 * - Clears LCD display only. posCount is not modified. Useful when using display for different things
 * Arguments: none
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
void fediClr (void){
    nokLcdClear();
    if (drawBarsNotAngle ==0){
        nokDrawCircle ();
    }else{
        nokDrawBars (0);
    }
}

/*************************** fediRead ***************************************
 * - Prints to console contents of a LS7366R register: MDR0, MDR1, CNTR, STR.
 * - Example: fediRead CNTR
 * Arguments: 1
 * Argument 1: numeric code for the register of interest. see LS7366R.h for list
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
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

/*************************** fediDisp ***************************************
 * -Activates the angle display mode for single line (0) or progress bar (1). See defined constants
 * - Example: fediDisp FEDI_LINE
 * Arguments: 1
 * Argument 1: mode for angle display, 0 (single line) or progress bar (1)
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
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


/*************************** fediFw ***************************************
 * -Displays the FW angular displacement as revolutions and angle.
 * Arguments: none
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
void fediFw (void){
    signed int nRotations = posCount/COUNTS_PER_REV;
    signed int angleCount = posCount % COUNTS_PER_REV;
    signed int angleVal = simpleRound (angleCount);
    // This is where we would put code to +/- 360 depending on negative/positive total displacement
    // and/or negative/positive direction of movement and/or phases of the moon
    sprintf ((char *)msgStr,"\tRev: %d Angle: %d deg", nRotations, angleVal);
    usciA1UartTxString (msgStr);
}

void fediZero (void){
    LS7366Rclear(CNTR);
    homePos = 0;
}



/*************************** timer0A1Isr ***************************************
 * -timer interrupt that reads the encoder and updates the display */
    #pragma vector = TIMER0_A1_VECTOR
    __interrupt void timer0A1Isr(void) {
        static unsigned int count = 0;                   // use static variable to hold count for "fizz buzz" approach
        if ((count % 2) ==0){                           // update encoder count every hundred ms, every second invocation of 50ms timer
            result=LS7366Rread(CNTR, dataIn);           // reads count from the SPI bus
            posCount = *(long int*)dataIn - homePos;    // updates global variable for position, taking into account home position
        }
        if ((count % 5) == 0){                          // update angle display every 250 ms, every 5th invocation of 50ms timer
            if (drawBarsNotAngle){                      // check global variable for current display method
                nokDrawBars (posCount);
            }else{
                nokDrawAngle (posCount % COUNTS_PER_REV);

            }
        }
        count++;                                        // increment counter
        if (count == 65530){                            // a multiple of both 5 and 2 close to integer rollover
            count = 0;
        }
        TA0CTL &= ~TAIFG;                               // clear TAIFG interrupt flag
    }
