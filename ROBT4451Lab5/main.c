#include <msp430.h> 
#include "nok5110LCD.h"
#include "LS7366R.h"
#include "libCmdInterp.h"
#include "usciSpi.h"
#include "nokLcdDraw.h"

    WDTCTL = WDTPW | WDTHOLD;                                       // stop watchdog timer
    usciA1UartInit(19200);                                          // initialize UART for 19200 Baud communication
    usciB1SpiInit(1, 4, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);  // initialize SPI communication
    nokLcdInit();                                                   // initialize NOKIA LCD
    LS7366Rinit();                                                  // initialize quadrature decoder chip


    unsigned char dataIn [4];       // data comes in as 4 bytes from the 4 byte counter
    volatile long int posCount;     // but we want a single signed count value


/**
 * main.c
 */

char msgStr [50];


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // stop watchdog timer
    usciB1SpiInit(1, 4, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);
    nokLcdInit();
    usciA1UartInit(19200);                  // initialize UART for 19200 Baud communication

    CMD cmdList [MAX_CMDS];                 // array of commands that we know how to process
    int cmdIndex;                           // position of current command in command list, -1 for error
    gCmdCnt = 1;                            // humans like to start at 1 not 0
    initCmds (cmdList);                     // initialize command list from #defines
    char gCmdLineBuffer [CMD_LEN];          // a simple garden-variety string


    unsigned char anglePos;
    volatile unsigned int iw;


    while (1){
        drawCircle ();
        nokLcdSetPixel(41, 23);
        nokLcdSetPixel(42, 23);
        nokLcdSetPixel(41, 24);
        nokLcdSetPixel(42, 24);
        for (anglePos =0; anglePos < 38; anglePos +=1){
                nokLcdDrawLine (42, 23, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 1);
                for (iw =0 ;iw < 25000; iw +=1){};
                nokLcdDrawLine (42, 23, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 0);
                nokLcdSetPixel(42, 23);
            }

            for (;anglePos < 75; anglePos +=1){
                nokLcdDrawLine (42, 24, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 1);
                for (iw =0 ;iw < 25000; iw +=1){};
                nokLcdDrawLine (42, 24, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 0);
                nokLcdSetPixel(42, 24);
            }
            for (;anglePos < 113; anglePos +=1){
                nokLcdDrawLine (41, 24, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 1);
                for (iw =0 ;iw < 25000; iw +=1){};
                nokLcdDrawLine (41, 24, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 0);
                nokLcdSetPixel(41, 24);
            }
            for (;anglePos < 150; anglePos +=1){
                   nokLcdDrawLine (41, 23, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 1);
                   for (iw =0 ;iw < 25000; iw +=1){};
                   nokLcdDrawLine (41, 23, nokDrawGetX (anglePos), nokDrawGetY (anglePos), 0);
                   nokLcdSetPixel(41, 23);
            }
            nokLcdClear();
    }
}
    /*

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
    }
	return 0;
}
 */

/*
 *     // configure reset pin P6.1 as input
//    P6SEL &= ~BIT1;
 //   P6DIR &= ~BIT1;
 //   P6REN &= ~BIT1;
    //while((P6IN & BIT1)==0) {};    // wait for it to go high, when switch is moved to ON
 //   for (nWaits =0; nWaits < 10; nWaits +=1){}; // this is lots of time for schmitt to go low then high again
    // Now configure reset pin P6.1 as output, if we want we can reset at anytime. REMEMBER schmitt trigger inverts output
//    P6OUT &= ~BIT1;     // first put it in NOT reset mode, cleared
//    P6DIR |= BIT1;      // then turn it to outputwhile (P6IN & ~BIT1){}  // wait for power-on-reset to start
    nWaits =0;
    while (P6IN & BIT1){nWaits +=1;}  // wait for power-on-reset to finish
    nWaits +=1;
    //P6DIR &= ~BIT0;
    while (1){
        unsigned char ii;
    for (ii =0; ii < 84 ; ii += 12){
        nokLcdDrawScrnLine (ii, 1);
    }
    nokLcdDrawScrnLine (83, 1);
    for (ii =0; ii < 48 ; ii += 12){
        nokLcdDrawScrnLine (ii, 0);
    }
    nokLcdDrawScrnLine (47, 0);
    */
