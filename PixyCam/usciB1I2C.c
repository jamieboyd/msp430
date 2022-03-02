/*
 * usciB1I2C.c
 *
 *  Created on: Feb. 23, 2022
 *      Author: jamie
 */


#include "usciB1I2C.h"

volatile unsigned char gNACK;

void usciB1I2CInit (unsigned int sclkDiv){
    P4SEL |= (BIT2 + BIT1);                     // P41 and P4.2 alternate functions are SDA and SCL for UCB1
    UCB1CTL1 = UCSWRST;                         // enter reset
    UCB1CTL0 = UCMST + UCMODE_3 + UCSYNC;      // I2C Master, synchronous mode
    UCB1CTL1 |= UCSSEL__SMCLK;              // Use SMCLK for I2C clock src
    UCB1BR0 = (sclkDiv & 0x00FF);                    // low byte for clock prescaler
    UCB1BR1 = (sclkDiv >> 8) ;                          // high byte for clock prescaler
    UCB1CTL1 &=  ~UCSWRST;                      // exit reset state
    UCB1IE |= UCNACKIE;                         //UCTXIE | UCRXIE;                           // enable interrupts

}


signed char usciB1I2CMstTransmit (unsigned char * txBuffer, unsigned char nBytes, unsigned char slaveAddr){
    signed char returnVal;
    unsigned char iByte;
    gNACK = 0;
    UCB1I2CSA = slaveAddr;
    UCB1IFG &= ~UCTXIFG;
    UCB1_SET_TX;
    UCB1_SET_START;
    for (iByte = 0; iByte < nBytes ; iByte +=1){
        while (!((UCB1IFG & UCTXIFG) || gNACK)){}; // wait till transmit buffer is empty
        if (gNACK) break;
        UCB1TXBUF = txBuffer [iByte];
    }
    if (gNACK){
        returnVal = -1;
    }else {
        while (!((UCB1IFG & UCTXIFG) || gNACK)){}; // wait till transmit buffer is empty
        UCB1_SET_STOP;
        returnVal = 0;
    }
    return returnVal;
}



signed char usciB1I2CMstReceive (signed char * rxBuffer, unsigned char nBytes, unsigned char slaveAddr){
    gNACK = 0;
    UCB1IFG &= ~UCRXIFG;
    signed char returnVal;
    unsigned char iByte;
    UCB1I2CSA = slaveAddr;
    UCB1_SET_RX;
    UCB1_SET_START;
    for (iByte = 0; iByte < nBytes ; iByte +=1){
        if (iByte == (nBytes -1)){
            UCB1_SET_STOP;
        }
        while (!((UCB1IFG & UCRXIFG) || gNACK)){}; // wait till buffer is set
        if (gNACK) break;
        rxBuffer [iByte] = UCB1RXBUF;
    }
    if (gNACK){
        returnVal = -1;
    }else {
        returnVal =0;
    }
    return returnVal;
}






#pragma vector = USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void) {
  switch(__even_in_range(UCB1IV,12)) {
  case  0: break;       // Vector  0: No interrupts
  case  2: break;       // Vector  2: ALIFG
  case  4:               // Vector  4: NACKIFG
      gNACK = 1;        // set global NACK flag
      UCB1_SET_STOP;
      UCB1IFG &= ~UCNACKIFG;
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
          UCB1CTL1 |= UCTXSTP;      // I2C stop condition
          UCB1IFG &=  ̃UCTXIFG;      // Clear USCI_B0 TX int flag
      }
      */

