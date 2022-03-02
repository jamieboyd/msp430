/*
 * usciB0I2C.c
 *
 *  Created on: Feb. 28, 2022
 *      Author: jamie
 */


/*
 * usciB1I2C.c
 *
 *  Created on: Feb. 23, 2022
 *      Author: jamie
 */


#include "usciB0I2C.h"

volatile unsigned char gNACK;                   // set by interrupt, read by functions that send or receive data

void usciB0I2CInit (unsigned int sclkDiv){
    P3SEL |= (BIT0 + BIT1);                     // P3.0 = SDA and P3.1 = SCL for UCB0
    UCB0CTL1 = UCSWRST;                         // enter reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;      // I2C Master, synchronous mode
    UCB0CTL1 |= UCSSEL__SMCLK;              // Use SMCLK for I2C clock src
    UCB0BR0 = (sclkDiv & 0x00FF);                    // low byte for clock prescaler
    UCB0BR1 = (sclkDiv >> 8) ;                          // high byte for clock prescaler
    UCB0CTL1 &=  ~UCSWRST;                      // exit reset state
    UCB0IE |= UCNACKIE;                         //UCTXIE | UCRXIE;                           // enable interrupts

}


signed char usciB0I2CMstTransmit (unsigned char * txBuffer, unsigned char nBytes, unsigned char slaveAddr){
    signed char returnVal;
    unsigned char iByte;
    gNACK = 0;
    UCB0I2CSA = slaveAddr;
    UCB0IFG &= ~UCTXIFG;
    UCB0_SET_TX;
    UCB0_SET_START;
    for (iByte = 0; iByte < nBytes ; iByte +=1){
        while (!((UCB0IFG & UCTXIFG) || gNACK)){}; // wait till transmit buffer is empty or we got nacked
        if (gNACK) break;       // the interrupt sets the STOP condition for us
        UCB0TXBUF = txBuffer [iByte];
    }
    if (gNACK){
        returnVal = -1;
    }else {
        while (!((UCB0IFG & UCTXIFG) || gNACK)){}; // wait till transmit buffer is empty
        UCB0_SET_STOP;
        returnVal = 0;
    }
    return returnVal;
}



signed char usciB0I2CMstReceive (unsigned char * rxBuffer, unsigned char nBytes, unsigned char slaveAddr){
    gNACK = 0;
    UCB0IFG &= ~UCRXIFG;
    signed char returnVal;
    unsigned char iByte;
    UCB0I2CSA = slaveAddr;
    UCB0_SET_RX;
    UCB0_SET_START;
    for (iByte = 0; iByte < nBytes ; iByte +=1){
        if (iByte == (nBytes -1)){
            UCB0_SET_STOP;
        }
        while (!((UCB0IFG & UCRXIFG) || gNACK)){}; // wait till buffer is set
        if (gNACK) break;
        rxBuffer [iByte] = UCB0RXBUF;
    }
    if (gNACK){
        returnVal = -1;
    }else {
        returnVal =0;
    }
    return returnVal;
}






#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void) {
  switch(__even_in_range(UCB0IV,12)) {
  case  0: break;       // Vector  0: No interrupts
  case  2: break;       // Vector  2: ALIFG
  case  4:               // Vector  4: NACKIFG
      gNACK = 1;        // set global NACK flag
      UCB0_SET_STOP;
      UCB0IFG &= ~UCNACKIFG;
      break;
  case  6: break;       // Vector  6: STTIFG
  case  8: break;       // Vector  8: STPIFG
  case 10: break;       // Vector 10: RXIFG
  case 12: break;           // Vector 12: TXIFG

  default: break;
  }
}


/* if (TXByteCtr) {      // Check TX byte counter
          UCB0TXBUF = *PTxData++;   // Load TX buffer
          TXByteCtr--;              // Decrement TX byte counter
      } else {
          UCB0CTL1 |= UCTXSTP;      // I2C stop condition
          UCB0IFG &=  ̃UCTXIFG;      // Clear USCI_B0 TX int flag
      }
      */

