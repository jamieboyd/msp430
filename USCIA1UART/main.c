/*************************************************************************************************
 * main.c
 * - top level file with main{} for UART lab1
 *
 *  Author: Greg Scutt
 *  Created on: March 1, 2018
 *  Modified: 2022/01/13 by Jamie Boyd
 **************************************************************************************************/
#include <msp430.h> 
#include <USCIA1UART.h>
#include <stdio.h>// used ONLY for sprintf function

/* Define Task as follows to perform a section of the lab
 *          551         5.5.1   Observing a UART transmit frame
 *          552         5.5.2   Sending a character to the PC terminal App
 *          553         5.5.3   Completing usciA1UartTxString function
 *          554         5.5.4   Complete usciA1UartTXBuffer function
 *          561         5.6.1   RX->TX Echo using interrupt
 *          562         5.6.2   char * usciA1UartGets (char * rxString)
 */
#define     TASK        562     // selects Task to compile

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watch-dog timer
    usciA1UartInit(19200);      // initialize UART for 19200 Baud communication
   /************** Configure selected TASK ************************************/
#if TASK == 551                             // 5.5.1 Observing a UART transmit frame
    const unsigned char txChar = 0x7F;      //  = 127 all data bits set except bit 7
#endif
#if TASK == 552                             // 5.5.2   Sending a character to the PC terminal App
    const char txChar = 'J';                //J, a printable character for the terminal
#endif
#if TASK == 553
    static const char testString1 [] = "BCIT MECHATRONICS\r\n\0"; // 5.5.3 send a string
#endif
#if TASK == 554                             // 5.5.4 send a text buffer
    static char dataArray [26];    // make a text buffer from A-Z
    unsigned int ii;
    const unsigned int base = 65;         // ASCII A, other capital letters follow in order
    for (ii=0; ii < 26; ii +=1){
        dataArray [ii] = (char)(base + ii);
    }
    static char msg [40];          // make a text buffer for the diagnostic message
    unsigned int count;
#endif
#if TASK == 561                 // 5.6.1 echo characters back to user, using interrupt
    usciA1UartInstallRxInt (&echoInterrupt);    // installs call-back function
    usciA1UartEnableRxInt (1);    // sets receive enable bit in UART1 interrupt enable register.
    __enable_interrupt();       // enable interrupts by setting global interrupt enable bit
#endif
#if TASK == 562                             // 5.6.2 receive string into a buffer
      char rxStr [100];
      char * rxPtr;
#endif


    /************* Do Selected TASK Repeatedly *****************************/
    while (1){
#if TASK <= 552
        usciA1UartTxChar(txChar);
#endif
#if TASK == 553
        usciA1UartTxString ((char *)testString1);
#endif
#if TASK == 554
        for (count =1; count <= 26; count += 5){
            sprintf ((char *)msg, "\r\n%d characters transmitted.\r\n", usciA1UartTxBuffer (dataArray, count));
            usciA1UartTxString (msg);
        }
#endif
#if TASK == 562
        rxPtr = usciA1UartGets (rxStr);
        if (rxPtr == NULL){
            sprintf ((char *)rxStr, "\r\nYou can only send %d characters.\r\n", RX_BUF_SZ-1);
            usciA1UartTxString (rxStr);
        }else{
            usciA1UartTxChar ('\r');
            usciA1UartTxChar ('\n');
            usciA1UartTxString (rxStr);
            usciA1UartTxChar ('\r');
            usciA1UartTxChar ('\n');
        }
#endif
    }
    return 0;
}
