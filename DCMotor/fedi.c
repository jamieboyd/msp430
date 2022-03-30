/*
 *  fedi.c
 *  C interface file for the encoder, designed to work with libCMDinterpreter, Basically bits of the original fedi
 *  Might fold them into vnh7070 proper later
 *  Created on: 2022/03/27 by Jamie Boyd
 *      Author: jamie
 */
#include <msp430.h>
#include <libCMDInterp.h>
#include <fedi.h>

unsigned char gFediErrOffset;
signed long int gFediHomePos =0;           // fedi home position. 0 is best home position
signed long int gFediPosCount;             // global variable updated when we read the count


unsigned int fediInit (void){
    unsigned char rVal = 0;     // successs
    usciB1SpiInit(1, 4, (SPI_READS_FIRST | SPI_CLK_IDLES_LOW), 0);  // initialize SPI communication
    LS7366Rinit();                                                  // initialize and clear encoder
    LS7366Rclear(CNTR);

    rVal += libCMD_addCmd (FEDIHOME, 3, 0, R_SLONG, &fediHome);         // 1: -1 or +1 for direction, 2: MSW of long int, 3: LSW of long int
    rVal +=libCMD_addCmd (FEDIREADREG, 1, 0, R_SLONG, &fediReadReg);         // 1: code for the register, 0x20 is count, returns the count value
    rVal +=libCMD_addCmd (FEDIREAD, 0, 0, R_SLONG, &fediRead);         // 1: code for the register, 0x20 is count, returns the count value
    rVal +=libCMD_addCmd (FEDICLEAR, 0, 0, R_NONE, &fediClear);        // clears counter to 0
    if (rVal ==0){
        gFediErrOffset = libCMD_addErr (FEDI_ERR0);      // add errors for fedi commands
        if (gFediErrOffset ==0){
            rVal = 1;
        }
    }
    return rVal;
}



/****************************** fedi interface functions, can be called from command interpreter***********************/

/*************************** fediHome ***************************************
 * - initializes CTR to user-provided value
 * - Example: fediHome 32767
 * Arguments:
 * argument 1: count to copy to the encoder. basically says "here" is now to be known as "there"
 * returns: the count just entered, to verify that it worked
 * errors: none
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
unsigned char fediHome (CMDdataPtr commandData){
    signed long int posCount;
    unsigned char dummydat [4];
    signed long int * posCountLocal = (signed long int *)&dummydat;
    posCount = (unsigned int)commandData->args[1] * 65536 + (unsigned int)commandData->args[2];
    if (commandData->args[0] < 0){
        posCount *= -1;
    }
    gFediHomePos = posCount;
    *posCountLocal = posCount;
    _disable_interrupts();
    LS7366Rwrite(DTR, dummydat);
    LS7366Rload (CNTR);
    _enable_interrupts();
    commandData ->result=posCount;
    return 0;
}


unsigned char fediClear (CMDdataPtr commandData){
    LS7366Rclear(CNTR);
    gFediHomePos = 0;
    return 0;
}



unsigned char fediReadReg (CMDdataPtr commandData){
    unsigned char err;
    unsigned char dataIn [4];
    unsigned char reg = (unsigned char)commandData->args[0];
    signed long int * resultPtr = (signed long int *) dataIn;
    _disable_interrupts();
    err = LS7366Rread(reg, dataIn);
    _enable_interrupts();
    if (err){
        err = gFediErrOffset + BAD_REGISTER;
    }else{
        if(reg==MDR0||reg==MDR1||reg==STR) {// you're reading a 1 byte register
           commandData ->result = (signed long int)dataIn[0];
        }else{ // 4 byte register
           gFediPosCount = *resultPtr;
           commandData ->result = gFediPosCount;
        }
    }
    return err;
}


unsigned char fediRead (CMDdataPtr commandData){
    unsigned char dataIn [4];
    signed long int * resultPtr = (signed long int *)dataIn;
    _disable_interrupts();
    unsigned char err = LS7366Rread(0x20, dataIn);
    _enable_interrupts();
    gFediPosCount = *resultPtr;
    commandData ->result = gFediPosCount;
    return 0;
}





/*************************** simpleRound ***************************************
 * -utility function to round count to degrees not just floor when dividing
 * Arguments: 1
 * argument 1: the count of the encoder, modulus counts_per_rev
 * returns: result of the division, rounded to nearest whole number
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
signed int simpleRound (signed int count){
    signed int rounded;
    if (count < 0){
        rounded = ((COUNTS_PER_DEGREE_NUM *count) - COUNTS_PER_HALF_DEGREE)/COUNTS_PER_DEGREE_DENOM;
    }else{
        rounded = ((COUNTS_PER_DEGREE_NUM*count) + COUNTS_PER_HALF_DEGREE)/COUNTS_PER_DEGREE_DENOM;
    }
    return rounded;
}


