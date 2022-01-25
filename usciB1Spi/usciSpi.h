/*
 * usciSpi.h
 *
 *  Created on: Jan. 21, 2022
 *      Author: jamie
 *      4.1 UCB1 MOSI
 *      4.2 UCB1 MISO
 *      4.3 UCB1 clock
 */



/************************* SPI Clock Modes *********************************
*   Mode    UCCKPL      UCCKPH      Description
*   0       0           1           lock idles low. Data is read on first rising edge and written on rising edge
*   1       0           0           clock idles low. Data is written on first falling edge and read on rising edge
*   2       1           1           clock idles high. Data is captured on rising edge and changed on falling edge
*   3       1           0           clock idles high  Data is changed on falling edge and captured on rising edge
****************************************************************************/

/******************************* defines **********************************/

#define     SPI_READS_FIRST         1      // used to set clock phase bit in control register.
#define     SPI_WRITES_FIRST        0      // used to set clock phase bit in control register.
#define     SPI_CLK_IDLES_HIGH      2      // used to set clock polarity select bit in control register
#define     SPI_CLK_IDLES_LOW       0      // used to set clock polarity select bit in control register
#define     SPI_MISO                2
#define     SPI_MOSI                4
#define     SPI_CLK                 8

#ifndef USCISPI_H_
#define USCISPI_H_

/******************************* Function Headers **********************************/
/* Function: usciB1SpiInit
* - configures USI B1 when used as an SPI controller
* Arguments: 3
* argument 1: spiMST – if 0 the USCIB1 is configured as a slave, otherwise it is a master.
* argument 2: sclkDIV – defines the 16 bit clock divisor used to divide 2^20 Hz SMCLK, which is used as the clock source
* argument 3: sclkMode – clock mode. bit 0 is clock polarity, bit 1 is phase. 0-3 are valid arguments
* return: nothing
* Sample use: usciB1SpiInit(1, 48, 1, unsigned char spiLoopBack)
* Author: Jamie Boyd
* Date: 2022/01/23 */
void usciB1SpiInit(unsigned char spiMST, unsigned int sclkDiv, unsigned char sclkMode, unsigned char spiLoopBack);

/* Function: usciB1SpiClkDiv
* - divides down smclk 2^20 Hz, about 1.04 MHz, to make clock for SPI
* Arguments:2
* argument 1: sclkDiv – 16 bit value for clock divisor
* argument 2: doReset - pass 1 to put USCI into reset before and take it out afterwards. 0 if done as part of init which does the reset
* return: nothing
* Author: Jamie Boyd
* Date: 2022/01/23 */
void usciB1SpiClkDiv(unsigned int sclkDiv, unsigned char doReset);
void usciB1SpiPutChar(unsigned char txByte);


#endif /* USCISPI_H_ */
