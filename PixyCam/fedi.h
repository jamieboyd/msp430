/*
 * fedi.h
 * C header file for the Friesen Encoder Display Interface
 * Where position of a whel is monitored with a LS3776R quadrature decoder and displayed in
 * various formats on a nokia 5110 LCD display, with user commands form a serial port command interpreter
 *
 * Created on: 2022/02/12
 *      Author: Jamie Boyd
 */

#ifndef FEDI_H_
#define FEDI_H_

#include <msp430.h>
#include <libUART1A.h>
#include "nok5110LCD.h"
#include "nokLcdDraw.h"
#include "LS7366R.h"
#include "libCmdInterp.h"
#include "usciSpi.h"
#include "stdio.h"

/************************* Program constants and Defines *********************************************
 * NOTE fedi also uses some constants related to the encoder defined in nokLcdDraw */
#define _50MS_CNT 50000     // number of clock events for 50 ms timer. Make this smaller to get faster position updates
#define FEDI_LINE 0         //  use the line display for angle
#define FEDI_BARS 1         // use the progress display for angle and num rotations


/************************ defines to enable testing functions *****************************************/
#undef CONSOLE_TEST             // for non-fedi control tests, define this one and one other console test define
#undef CONSOLE_TEST_CNT         // sends the position of the encoder over the serial link, updated constantly
#undef CONSOLE_TEST_ANG         // tests the single line angular position function, updating the "clock hand" angle display constantly
#undef CONSOLE_TEST_BARS        // Tests the horizontal progress bar function, updating position constantly
#define UART_CONTROL            // fedi is controlled by command interpreter, position updates and display done by timer interrupt


/*********************** Function Headers ***********************************************/

/*************************** fediHome ***************************************
 * - initializes CTR to user-provided value, clears LCD display, zeroes angular displacement.
 * - Example: fediHome 32767
 * Arguments: 1
 * argument 1: count to copy to the encoder. basically says "here" is now to be known as "there"
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
char fediHome (signed long int posCount);

/*************************** fediClr ***************************************
 * - Clears LCD display only. posCount is not modified. Useful when using display for different things
 * Arguments: none
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
void fediClr (void);

/*************************** fediRead ***************************************
 * - Prints to console contents of a LS7366R register: MDR0, MDR1, CNTR, STR.
 * - Example: fediRead CNTR
 * Arguments: 1
 * Argument 1: numeric code for the register of interest. see LS7366R.h for list
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
char fediRead (unsigned char reg);

/*************************** fediDisp ***************************************
 * -Activates the angle display mode for single line (0) or progress bar (1). See defined constants
 * - Example: fediDisp FEDI_LINE
 * Arguments: 1
 * Argument 1: mode for angle display, 0 (single line) or progress bar (1)
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
char fediDisp (unsigned char mode);


/*************************** fediFw ***************************************
 * -Displays the FW angular displacement as revolutions and angle.
 * Arguments: none
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
void fediFw (void);

/*************************** fediZero ***************************************
 * -Zeros the encoder count using the LS7366Rclear function
 * Arguments: 0
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
void fediZero (void);


/*************************** timer0A1Isr ***************************************
 * -timer interrupt that reads the encoder and updates the display
 * Arguments: 0
 * returns: nothing
 * Author: Jamie Boyd
 * Date: 2022/02/13 */
 __interrupt void timer0A1Isr(void);


#endif /* FEDI_H_ */
