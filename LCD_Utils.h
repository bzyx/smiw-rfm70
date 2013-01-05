/*
 * LCD_Utils.h
 *
 *  Created on: 05-01-2013
 *      Author: Dell Vostro V131
 */

#ifndef LCD_UTILS_H_
#define LCD_UTILS_H_

#include "usbdrv/usbdrv.h"

static uint8_t isChanged = 1;

void copyToScreen(uchar *data, uchar len, char screenLine[17], uchar addr);
unsigned char center(char* string);
void printScreenWithCenter(char screen[4][17]);
void printScreen(char screen[4][17]);


#endif /* LCD_UTILS_H_ */
