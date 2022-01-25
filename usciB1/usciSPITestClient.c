


/**
 * main.c
 */

#include "usciSpi.h"

extern char uartRxBuffer []; // buffer that receive data from usciA1UARTgets. referenced in header so can be accessed easily
extern unsigned char spiTxBuffer [];
extern unsigned char spiRxBuffer[];

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;               // Stop watch-dog timer
    usciA1UartInit(19200);                  // initialize UART for 19200 Baud communication, used for nearly all SECTIONS

#ifdef SPI_QUEST
    usciB1SpiInit(1, 1024, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);
    // configure a SPI /SS line on P6.0
    P6DIR |= 1;
    P6OUT &= ~1;
    unsigned char testVal = 170;
#endif


#ifdef SECTION_521
    usciB1SpiInit(1, 1, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);
    usciA1UartInstallRxInt (&uartToSPI);
    usciA1UartEnableRxInt (1);
    __enable_interrupt();       // enable interrupts by setting global interrupt enable bit
#endif

#ifdef SECTION_52x              // nested #ifdefs. There's no way that can go wrong
#ifdef SECTION_524              // loopback and an interrupt on RX
    usciB1SpiInit(1, 16, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 1);
    UCB1IE |= UCRXIE;
    UCB1IE &= ~UCTXIE;
    __enable_interrupt();       // enable interrupts by setting global interrupt enable bit
#else
    usciB1SpiInit(1, 105, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);
#endif
    unsigned char buffLen;
#endif

#ifdef SECTION_525_SLV
    usciB1SpiInit(0, 16, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);
    UCB1IE |= UCRXIE;
    UCB1IE &= ~UCTXIE;
    __enable_interrupt();       // enable interrupts by setting global interrupt enable bit
#endif



while (1){

#ifdef SPI_QUEST  // assume that if MOSI goes high, TXBUF must have been transferred to TXSR
    while (!(UCB1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send
    P6OUT |= 1;                 // set before writing byte
    UCB1TXBUF = testVal;  // write the byte to the TX buffer register
    P6OUT &= ~1;                // clear after writing byte
    while (!(UCB1IFG & UCRXIFG)){};   // poll, waiting while byte is sent
    UCB1IFG &= ~UCRXIFG;            // clear flag for next byte
#endif


#ifdef SECTION_52x
    if ( usciA1UartGets (uartRxBuffer) != NULL){ // NUL is returns if string too long for buffer
            buffLen = numStringToInt ((unsigned char *)uartRxBuffer, spiTxBuffer); // atoi on each byte in rxString and store in buffer usciB1SpiTXBuffer(buffer, buffLen) // transmit buffer of integers using usciB1SpiPutChar
            usciB1SpiTXBuffer(spiTxBuffer, buffLen); // transmit buffer of integers using usciB1SpiPutChar
        }
#endif

#ifdef SECTION_525_SLV              // The MSP_SPI_SLV uses the ISR to read the RXBUF.
    spiRxPos=0;
    while (spiRxPos < 100) {};
    usciA1UartTxBuffer (spiRxBuffer, 100); // at the end, write it back
#endif
    }
    return 0;
}



/*
 *
 * volatile unsigned char gCmdCnt;                // number of command being processed = number of command just entered
//volatile unsigned char gError = 0;             // flag to be set to an error condition, there are 8, including 0-no error
  CMD cmdList [MAX_CMDS];                 // array of commands that we know how to process
  int cmdIndex;                            // position of current command in command list, -1 for error
  gCmdCnt = 1;                            // humans like to start at 1 not 0
  initCmds (cmdList);                  // initialize command list from #defines

  char gCmdLineBuffer [CMD_LEN];                      // a simple garden-variety string
  */

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
*/
