/*
 * portGPIO.h
 *
 *  Created on: Mar. 17, 2022
 *      Author: jamie
 */

#ifndef PORTGPIO_H_
#define PORTGPIO_H_

// some static strings for port commands, reading and writing GPIO
#define     PORTSETUP       "portSetup\0"       //0:portNum, 1:read =0, write = 1, 2:mask
#define     PORTWRITEBITS   "writeBits\0"     // 0:portNum, 1:clear =0, set =1, toggle =2, 2:mask
#define     PORTWRITEBYTE   "writeByte\0"     // 0:portNum, 1: byte
#define     PORTREADBITS    "readBits\0"      // 0:portNum, 1:mask, 2:print type 0 = binary, 1=text

// some static strings for error messages for these commands
#define     PORT_ERR0         "arg 1 port must be <= 8\0"
#define     PORT_ERR1         "arg 2 must be 0=input, 1=output\0"
#define     PORT_ERR2         "arg 3 mask must be 1 to 255\0"
#define     PORT_ERR3         "arg 2 mode must 0=clear, 1=set, 2-toggle\0"
#define     PORT_ERR4         "arg 2 data must be 0 to 255\0"
#define     PORT_ERR5         "arg 2 type must be 0=binary, 1=text\0"

// some shortcuts for port access, based on using the msp430f5529.
#define f5529
//#undef f5529

#ifdef f5529
#define     INPUT_REG           0
#define     OUTPUT_REG          0x02
#define     DIR_REG             0x04
#define     REN_REG             0x06
#define     SEL_REG             0x0A
#define     INTVEC_REG          0x0E
#define     PORT_OFFSET         0x0200
#define     PORT_MEM            0x20
#define     PORT_REG_ADDR(portNum, portReg)(PORT_OFFSET + (((portNum - 1)/2) * PORT_MEM) + portReg + ((portNum -1) % 2))
#endif

// function headers
unsigned char portInit (void);
unsigned char portSetUp (CMDdataPtr commandData);
unsigned char portWriteBits (CMDdataPtr commandData);
unsigned char portWriteByte (CMDdataPtr commandData);
unsigned char portReadBits (CMDdataPtr commandData);

//#ifdef f5529
//unsigned char portReadAny (CMDdataPtr commandData);
//unsigned char portWriteAny (CMDdataPtr commandData);
//#endif

#endif /* PORTGPIO_H_ */
