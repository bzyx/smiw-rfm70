/*
 * LCD_Utils.c
 *
 *  Created on: 05-01-2013
 *      Author: Dell Vostro V131
 */

#include <string.h>
#include "hd44780.h"

#include "LCD_Utils.h"

void copyToScreen(uchar *data, uchar len, char screenLine[17], uchar addr) {
	static uchar i;
	if (addr == 1) { //Left part
		for (i = 0; i < len; i++) {
			screenLine[i] = data[i];
		}
		isChanged = 1;
		//return;
	}
	if (addr == 2) { //Right part
		for (i = 0; i < len; i++) {
			screenLine[8 + i] = data[i];
		}
		screenLine[16] = '\0';
		isChanged = 1;
		//return;
	}
}

unsigned char center(char* string) {
	uint8_t len = strlen(string);
	len = 16 - len;
	len /= 2;
	return len;
}

void printScreenWithCenter(char screen[4][17]) {
	if (isChanged == 1) {
		LCD_Clear();
		isChanged = 0;
	}
	LCD_GoTo(center(screen[0]), 0);
	LCD_WriteText(screen[0]);
	LCD_GoTo(center(screen[1]), 1);
	LCD_WriteText(screen[1]);
	LCD_GoTo(center(screen[2]), 2);
	LCD_WriteText(screen[2]);
	LCD_GoTo(center(screen[3]), 3);
	LCD_WriteText(screen[3]);
}

void printScreen(char screen[4][17]) {
	if (isChanged == 1) {
		LCD_Clear();
		isChanged = 0;
	}
	LCD_GoTo(0, 0);
	LCD_WriteText(screen[0]);
	LCD_GoTo(0, 1);
	LCD_WriteText(screen[1]);
	LCD_GoTo(0, 2);
	LCD_WriteText(screen[2]);
	LCD_GoTo(0, 3);
	LCD_WriteText(screen[3]);
}

