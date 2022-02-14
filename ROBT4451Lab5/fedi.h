/*
 * LS7366TestBench.h
 *
 *  Created on: Feb. 13, 2022
 *      Author: jamie
 */

#ifndef FEDI_H_
#define FEDI_H_

#define _50MS_CNT 50000 // number of clock events for 50 ms


#undef CONSOLE_TEST
#undef CONSOLE_TEST_CNT
#undef CONSOLE_TEST_ANG
#undef CONSOLE_TEST_BARS
#define UART_CONTROL

char fediHome (signed long int posCount);
void fediClr (void);
char fediRead (unsigned char reg);
char fediDisp (unsigned char mode);
void fediFw (void);
void fediZero (void);

 __interrupt void timer0A1Isr(void);


#endif /* FEDI_H_ */
