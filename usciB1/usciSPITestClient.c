


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
    P6DIR |= 1;
    P6OUT &= ~1;
    volatile unsigned int ii;
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
    usciB1SpiInit(1, 4, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 1);
    UCB1IE |= UCRXIE;
    UCB1IE &= ~UCTXIE;
    __enable_interrupt();       // enable interrupts by setting global interrupt enable bit
#else
    usciB1SpiInit(1, 105, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);
#endif
    unsigned char buffLen;
#endif


while (1){
#ifdef SPI_QUEST
    UCB1IFG &= ~UCRXIFG;            // clear received flag
    for (ii =0; ii < 65000; ii +=1){}; // maybe introduce some random delay
    P6OUT |= 1;                         // set signal before writing byte
    UCB1TXBUF = testVal;                // write the byte to the TX buffer register
    while (!(UCB1IFG & UCTXIFG)){};   // poll, waiting for an opportunity to send, this indicates that byte has transferred from TXBUF to TXSR
    P6OUT &= ~1;                        // clear signal now. WHen does MOSI go high for MSB - that is the question.
    while (!(UCB1IFG & UCRXIFG)){};   // poll, waiting while byte is completely sent before doing it all over again
    UCB1IFG &= ~UCRXIFG;            // clear flag for next byte
#endif


#ifdef SECTION_52x
    if (usciA1UartGets (uartRxBuffer) != NULL){ // NUL is returns if string too long for buffer
            buffLen = numStringToInt ((unsigned char *)uartRxBuffer, spiTxBuffer); // atoi on each byte in rxString and store in buffer usciB1SpiTXBuffer(buffer, buffLen) // transmit buffer of integers using usciB1SpiPutChar
            usciB1SpiTXBuffer(spiTxBuffer, buffLen); // transmit buffer of integers using usciB1SpiPutChar
        }
#endif
    }
    return 0;
}
