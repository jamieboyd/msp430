/*
 * pixyCam2.h
 *
 *  Created on: Feb. 23, 2022
 *      Author: jamie
 *      Interface for PixyCam, using I2C. NOTE: max i2c clock clock frequency  is 100 kHz according to
 *      https://forum.pixycam.com/t/speed-and-frame-rate-for-each-interface/5542
 *
 *      Pixy2 communicates with “packets” in both directions – request and response. Each packet has the following structure, if no checksums are used:
 *
 *      Request Example (get version number)
 *      0xae    first byte of no_checksum_sync (little endian -> least-significant byte first)
 *      0xc1  // second byte of no_checksum_sync
 *      0x0e  // this is the version number request type
 *      0x00  // data_length is 0
 *
 *      Response example (return the version number)
 *      0xaf // first byte of checksum_sync (little endian -> least-significant byte first)
 *      0xc1 // second byte of checksum_sync
 *      0x0f // this is the version number response type
 *      0x10 // data_length is 0x10 (16) bytes
 *      16 bits of version number data follows here
 */

#ifndef PIXYCAM2_H_
#define PIXYCAM2_H_

#define PIXY_CLOCK_DIV          32              // 10 gives about 100Khz, max camera can do.
#define PIXY_ADDRESS            0x54            // set using PixyMon
#define PIXY_SYNC_SEND0         0xAE            // sync bits verified when received by camera
#define PIXY_SYNC_SEND1         0xC1

/************************* defines for some of the numeric codes used to communicate with camera ********************** */
#define PIXY_VERSION_REQUEST    0x0E            // camera returns version numbers
#define PIXY_FPS_REQUEST        0x18            // camera returns frames per second as unsigned long (why?)
#define PIXY_LAMP_REQUEST       0x16            // camera turns on lamp
#define PIXY_MAIN_REQUEST       0x30            // camera returns main data for vectors, bars scans,  according to request
#define PIXY_MAIN_FEATURES      0x0             // code for return main features only, not the whole thing
#define PIXY_VECTOR_REQUEST     0x1             // as part of main request, requests vector info
#define PIXY_SYNC_RECEIVE1      0xAF    // we should check these values, and checksum, etc. after every read, but we don't, Yet
#define PIXY_SYNC_RECEIVE2      0xC1


#define _FRAME_TIME_CNT 20000     // number of clock events for 20 ms timer. A little slower than frame rate (60 Hz), so should have fresh data every time

void pixyInit(void);

signed char pixyGetVersionCMD ();
signed char pixyGetVersion(unsigned char * rxBuffer);
signed char pixyGetFPSCMD(void);
signed char pixyGetFPS (unsigned char * rxBuffer);

/*************************** pixySetLampCMD ***************************************
 * Asks camera to turn on lamps above/below screen
 * what a chatty protocol. Camera sends us back 10 bytes. Not going to print them over UART
 * With no UART stuff, no need for separate wrapper/i2c functions for "separation of concerns"
 * Arguments: 2
 * 1) upperON - turn on (1) or off (0) upper lights
 * 2) lowerON - turn on (1) or off (0) lower lights
 * returns -1 if an error in communication, else 0
 * Author: Jamie Boyd
 * Date: 2022/03/01 */
signed char pixySetLampCMD(unsigned char upperON, unsigned char lowerON);


signed char pixyGetVector(unsigned char * rxBuffer);
signed char pixyGetVectorCMD( unsigned char continuous, unsigned char displayMode);

/*************************** pixyDrawPos ***************************************
 * - draws a line from Pixy Data, scrunching data to end of screen. Not particularly elegant approach
 *  but takes advantage of the near 1:1 mapping of camera space (78:51) to screen space (84:48), after accounting for non-square screen pixels
 *  NOTE: the camera pixels appear to be pretty much square
 *  saves the previous position so we can "undraw" it
 *  Arguments: the 4 camera coordinates
 * returns: none
 * Author: Jamie Boyd
 * Date: 2022/03/01 */
void pixyDrawPos (unsigned char px1, unsigned char py1, unsigned char px2, unsigned char py2);


void pixySetStopRxInt (char theChar);

__interrupt void timer0A1Isr(void);

#endif /* PIXYCAM2_H_ */
