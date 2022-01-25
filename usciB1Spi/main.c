/*
 * main.c
 *
 *  Created on: Jan. 23, 2022
 *      Author: jamie
 */


#include <msp430.h>
#include "usciSpi.h"

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    usciB1SpiInit(1, 55, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 1);
    while (1){
        usciB1SpiPutChar (85);
    }

    return 0;
}
