/*
 * usciB1I2C.h
 *
 *  Created on: Feb. 23, 2022
 *      Author: jamie
 */

#ifndef USCIB1I2C_H_
#define USCIB1I2C_H_


#include <msp430.h>

#define UCB1_SET_NACK        UCB1CTL1 |= UCTXNACK           // generates a NACK condition at end of byte
#define UCB1_SET_TX          UCB1CTL1 |= UCTR                 // sets msp430 to be transmitter
#define UCB1_SET_RX          UCB1CTL1 &= ~UCTR                 // sets msp430 to be receiver
#define UCB1_SET_STOP       UCB1CTL1 |= UCTXSTP         // set stop bit, with NACK set too  when transmitting
#define UCB1_SET_START       UCB1CTL1 |= UCTXSTT          // transmit start bit

 extern volatile unsigned char gNACK;

void usciB1I2CInit (unsigned int sclkDiv);
signed char usciB1I2CMstTransmit (unsigned char * txBuffer, unsigned char nBytes, unsigned char slaveAddr);
signed char usciB1I2CMstReceive (signed char * rxBuffer, unsigned char nBytes, unsigned char slaveAddr);
__interrupt void USCI_B1_ISR(void);




#endif /* USCIB1I2C_H_ */
