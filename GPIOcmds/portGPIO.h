/*
 * portGPIO.h - An example using libCmdInterp to configure, write to, and read from GPIO ports on the msp430
 *
 *  Created on: Mar. 17, 2022
 *      Author: jamie
 *      sample usage at command line
 *      portSetup 1 1 1         // setup port 1 to write to bit 0 (red led on launcpad)
 *      writeBits 1 1 1         // turn on the LED
 *      writeBits 1 0 1         // turn off the LED
 *      writeBits 1 2 1         // toggle the LED state
 *      portSetup 1 0 2         // set up port 1 to read from pin 1 (the button the launchpad)
 *      resEnable 1 1 2         // enable port 1 pull-up resistor on pin 1
 *      readBits 1 2            // read button state
 *
 */

#ifndef PORTGPIO_H_
#define PORTGPIO_H_

#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include <libCmdInterp.h>

// some static strings for port commands, reading and writing GPIO
#define     PORTSETUP       "portSetup\0"      // 1:portNum, 2:read =0, write = 1, 3:mask
#define     PORTWRITEBITS   "writeBits\0"     // 1:portNum, 2:clear =0, set =1, toggle =2, 3:mask
#define     PORTWRITEBYTE   "writeByte\0"     // 1:portNum, 2: byte
#define     PORTREADBITS    "readBits\0"      // 1:portNum, 2:mask
#define     PORTREN         "resEnable\0"     // 1: portNum, 2: type (0 = pullDown, 1 = pullUp), 3:mask

// some static strings for error messages for these commands
#define     PORT_ERR0         "arg 1 port must be <= 8\0"
#define     PORT_ERR1         "arg 2 must be 0=input, 1=output\0"
#define     PORT_ERR2         "arg 3 mask must be 1 to 255\0"
#define     PORT_ERR3         "arg 2 mode must 0=clear, 1=set, 2-toggle\0"
#define     PORT_ERR4         "arg 2 data must be 0 to 255\0"
#define     PORT_ERR5         "arg 2 type must be 0=binary, 1=text\0"
#define     PORT_ERR6         "arg 2 must be 0=pull-Down, 1=pull-Up\0"

// and some matching mnemonics
#define     BAD_PORT        0
#define     BAD_DIR         1
#define     BAD_MASK        2
#define     BAD_SET_MODE    3
#define     BAD_DATA        4
#define     BAD_PRINT_TYPE  5
#define     BAD_RES_TYPE    6

extern unsigned char gPortCmdsErrOffset;        // holds offset to GPIO error messages, when they are added

// some shortcuts, defined using the memory mapping of msp430f5529, or similar
// These can be used to get register addresses for port access when using one of these controllers
// else we have to use the versions of the functions with long switch statements with a case for each port number
#define f5529
//#undef f5529

#ifdef f5529
#define     INPUT_REG           0           // when port is used as an input
#define     OUTPUT_REG          0x02        // when port is used as an output
#define     DIR_REG             0x04        // sets direction of port, input or output
#define     REN_REG             0x06        // sets use of pull-up or pull-down resistors when used as input
#define     SEL_REG             0x0A        // selects special functions that may be available
#define     INT_DIR_SEL_REG     0x18        // interrupt edge select register
#define     INT_ENABLE_REG      0x1A        // interrupt enable register
#define     INT_FLAG_REG        0x1C        // interrupt flag register
#define     PORT_OFFSET         0x0200      // offset in memory to port 1, all others are calculated relative to this
#define     PORT_MEM            0x20        // the size of the memory used for each port
#define     PORT_REG_ADDR(_portNum, _portReg)(PORT_OFFSET + (((_portNum - 1)/2) * PORT_MEM) + _portReg + ((_portNum-1) % 2)) // a macro to
                                                // get the memory location of a given function register for a given port
#endif

// function headers
unsigned char portInit (void);
unsigned char portSetUp (CMDdataPtr commandData);
unsigned char portRen (CMDdataPtr commandData);
unsigned char portWriteBits (CMDdataPtr commandData);
unsigned char portWriteByte (CMDdataPtr commandData);
unsigned char portReadBits (CMDdataPtr commandData);

//#ifdef f5529
//unsigned char portReadAny (CMDdataPtr commandData);
//unsigned char portWriteAny (CMDdataPtr commandData);
//#endif

#endif /* PORTGPIO_H_ */
