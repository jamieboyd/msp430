/*
 * usciB0I2C.h
 *
 *  Created on: Feb. 28, 2022
 *      Author: jamie
 */

#ifndef USCIB0I2C_H_
#define USCIB0I2C_H_

#include <msp430.h>

#define UCB0_SET_NACK        UCB0CTL1 |= UCTXNACK           // generates a NACK condition at end of byte
#define UCB0_SET_TX          UCB0CTL1 |= UCTR                 // sets msp430 to be transmitter
#define UCB0_SET_RX          UCB0CTL1 &= ~UCTR                 // sets msp430 to be receiver
#define UCB0_SET_STOP       UCB0CTL1 |= UCTXSTP         // set stop bit, with NACK set too  when transmitting
#define UCB0_SET_START       UCB0CTL1 |= UCTXSTT          // transmit start bit

 extern volatile unsigned char gNACK;

void usciB0I2CInit (unsigned int sclkDiv);
signed char usciB0I2CMstTransmit (unsigned char * txBuffer, unsigned char nBytes, unsigned char slaveAddr);
signed char usciB0I2CMstReceive (unsigned char * rxBuffer, unsigned char nBytes, unsigned char slaveAddr);
__interrupt void USCI_B0_ISR(void);



#endif /* USCIB0I2C_H_ */
