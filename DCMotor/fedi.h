/*
 * fedi.h
 * C header file for the Friesen Encoder Display Interface with user commands from a serial port command interpreter
 *
 * Created on: 2022/03/27
 *      Author: Jamie Boyd
 */

#ifndef FEDI_H_
#define FEDI_H_

#include "LS7366R.h"
#include "usciSpi.h"

// define some static strings for command names
#define     FEDIREADREG            "fediReadReg"            // read a register from an address code
#define     FEDIREAD               "fediRead"               // just read the count, no messing around with a code
#define     FEDICLEAR              "fediClear"              // clears the count to zero, effectively setting home position
#define     FEDIHOME               "fediHome"               // sets home position from a given value
// some static strings for error messages for these commands
#define     FEDI_ERR0         "Invalid register specified"


// some mnemonic codes for the error string offsets
#define     BAD_REGISTER        0


#define COUNTS_PER_REV          4600        // 1150 * 4 quadrature
#define COUNTS_PER_DEGREE_NUM   9           // for integer math rounding. 360/4600 = 9/115
#define COUNTS_PER_DEGREE_DENOM 115
#define COUNTS_PER_HALF_DEGREE  57           // before dividing by COUNTS_PER_DEGREE_DENOM. 9 * (4600/360)/2 used for rounding counts to nearest degree.


/*********************** Function Headers ***********************************************/


unsigned int fediInit (void);

signed int simpleRound (signed int count);

/*************************** fediHome ***************************************
 * - initializes CTR to user-provided value, copies to the encoder. basically says "here" is now to be known as 0
 * - Example: fediHome 32767
 * Arguments: 3
 * argument 1:
 * returns: the value copied to the encoder
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
unsigned char fediHome (CMDdataPtr commandData);


/*************************** fediRead ***************************************
 * - Prints to console contents of a LS7366R register: MDR0, MDR1, CNTR, STR.
 * - Example: fediRead CNTR
 * Arguments: 1
 * Argument 1: numeric code for the register of interest. see LS7366R.h for list
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
unsigned char fediRead  (CMDdataPtr commandData);

unsigned char fediReadReg  (CMDdataPtr commandData);


/*************************** fediZero ***************************************
 * -Zeros the encoder count using the LS7366Rclear function
 * Arguments: 0
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
unsigned char fediClear  (CMDdataPtr commandData);



#endif /* FEDI_H_ */
