


/**
 * main.c
 */

#include <msp430.h>
#include "PixyCam2.h"
#include "usciB0I2C.h"
#include "libCmdInterp.h"
#include "usciSpi.h"
#include "nok5110LCD.h"


char msgStr [50];
unsigned int result;                // error code

main (void){
    WDTCTL = WDTPW | WDTHOLD;   // stop watch-dog timer

    CMD cmdList [MAX_CMDS];                 // array of commands that we know how to process
    int cmdIndex;                           // position of current command in command list, -1 for error
    gCmdCnt = 1;                            // humans like to start at 1 not 0
    initCmds (cmdList);                     // initialize command list from #defines
    char gCmdLineBuffer [CMD_LEN];

    usciA1UartInit(19200);                 // initialize UART for 19200 Baud communication
    usciB1SpiInit(1, 4, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);
    nokLcdInit();                          // initialize LCD for drawing lines for angles
    pixyInit();                            // initialize the camera
    __enable_interrupt();                   // we use interrupts for NACKs, some serial stuff as well
    pixyGetVersionCMD ();
    pixyGetFPSCMD();
    pixySetLampCMD (1,1);
    pixyGetVectorCMD(0, 3);             // positions not angles

    while (1){
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
    /* test code for mcp DAC chip. WAY EASIER COMMANDS TO LEARN I2C WITH
    signed char rBuf [5] = {0,0,0,0,0};
    signed char tBuf [6];

    unsigned int dacVal1 = 500;
    unsigned int dacVal2 = 1500;
    unsigned int dacVal3 = 3250;

    tBuf[0] = (dacVal1 >> 8)& ~0xFFF0;  // write high data bits first
    tBuf [1] =  dacVal1 & ~0xFF00;          // low data byte
    tBuf[2] = (dacVal2 >> 8)& ~0xFFF0;   // write high data bits first
    tBuf [3] =  dacVal2 & ~0xFF00;          // low data byte
    tBuf[4] = (dacVal3 >> 8)& ~0xFFF0;   // write high data bits first
    tBuf [5] =  dacVal3 & ~0xFF00;          // low data byte


    usciB1I2CInit (10);
    __enable_interrupt();

    signed char tResult;
    signed char rResult;


    while (1){


        tResult = usciB1I2CMstTransmit (tBuf, 6, 0x12);
        if (tResult < 0){
            tResult +=1;
        }
        tResult = usciB1I2CMstTransmit (tBuf, 6, 0x62);
        rResult = usciB1I2CMstReceive (rBuf, 5, 0x62);


    }
    return 0;
}
*/

