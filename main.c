/* Name: main.c
 * Project: hid-data, example how to use HID for data transfer
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-11
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id$
 */

/*
 This example should run on most AVRs with only little changes. No special
 hardware resources except INT0 are used. You may have to change usbconfig.h for
 different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
 at least be connected to INT0 as well.
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <avr/eeprom.h>
#include <inttypes.h>
#include <stdlib.h>
#include <util/atomic.h>
#include <string.h>
#include <stdio.h>

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv/usbdrv.h"
#include "usbdrv/oddebug.h"

#include "hd44780.h"
#include "irmp.h"
#include "irmpconfig.h"

#include "RFM70.h"
/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM char usbHidReportDescriptor[22] = { /* USB report descriptor */
0x06, 0x00, 0xff, // USAGE_PAGE (Generic Desktop)
		0x09, 0x01, // USAGE (Vendor Usage 1)
		0xa1, 0x01, // COLLECTION (Application)
		0x15, 0x00, //   LOGICAL_MINIMUM (0)
		0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
		0x75, 0x02, //   REPORT_SIZE (8) -!!!!!!!! by³o 0x01
		0x95, 0x80, //   REPORT_COUNT (128)
		0x09, 0x00, //   USAGE (Undefined)
		0xb2, 0x02, 0x01, //   FEATURE (Data,Var,Abs,Buf)
		0xc0 // END_COLLECTION
		};
/* Since we define only one feature report, we don't use report-IDs (which
 * would be the first byte of the report). The entire report consists of 128
 * opaque data bytes.
 */

/* The following variables store the status of the current data transfer */
static uchar bytesRemaining;

static IRMP_DATA irmp_data;
static uint8_t isChanged = 1;

//static double adcVal;
float adcVal;
static int lastKey;
static uchar lineNo;

static char screenLeft[4][17] = { "Ekran Lewy", "-", "-", "-" };
//static char screenCenter[4][17] = { "Temperatura", " ", "Ost. klawisz", " " };
static char screenCenter[4][17] = { "Temperatura", " ", " ", " " };
static char screenRight[4][17] = { "Ekran Prawy", "-", "-", "-" };

static int intCount = 0;

uint8_t message[32];
const char on_message[]="wys";

/* ------------------------------------------------------------------------- */

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */uchar usbFunctionRead(uchar *data, uchar len) {
	uchar i;
	if (len > bytesRemaining) // len is max chunk size
		len = bytesRemaining; // send an incomplete chunk
	bytesRemaining -= len;
	for (i = 0; i < 6; i++)
		data[i] = screenCenter[1][i]; // copy the data to the buffer
	data[6] = screenCenter[3][0];
	data[7] = screenCenter[3][1];
	return len; // return real chunk size
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */uchar usbFunctionWrite(uchar *data, uchar len) {
	static uchar myAdres;

	if (len > bytesRemaining) // if this is the last incomplete chunk
		len = bytesRemaining; // limit to the amount we can store
	bytesRemaining -= len;

	if (lineNo == 0) {
		lineNo = data[0];
		myAdres = 1;
		return bytesRemaining;
	}

	switch (lineNo) {
	case 1: { //Screen left line 1
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenLeft[0], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenLeft[0], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;
	case 2: { //Screen left line 2
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenLeft[1], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenLeft[1], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;
	case 3: { //Screen left line 3
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenLeft[2], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenLeft[2], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;
	case 4: { //Screen left line 4
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenLeft[3], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenLeft[3], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;

	case 5: { //Screen right line 1
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenRight[0], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenRight[0], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;
	case 6: { //Screen right line 2
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenRight[1], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenRight[1], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;
	case 7: { //Screen right line 3
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenRight[2], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenRight[2], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;
	case 8: { //Screen right line 4
		if (myAdres == 1) { //left part of string
			copyToScreen(data, len, screenRight[3], myAdres);
			myAdres = 2;
			return bytesRemaining;
		}
		if (myAdres == 2) {
			copyToScreen(data, len, screenRight[3], myAdres);
			myAdres = 0;
			lineNo = 0;
			return bytesRemaining;
		}
	}
		break;
	default:
		break;
	}
	return bytesRemaining == 0; // return 1 if we have all data
}

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t *rq = (void *) data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) { /* HID class request */
		if (rq->bRequest == USBRQ_HID_GET_REPORT) { /* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* since we have only one report type, we can ignore the report-ID */
			bytesRemaining = 8;
			return USB_NO_MSG; /* use usbFunctionRead() to obtain data */
		} else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
			/* since we have only one report type, we can ignore the report-ID */
			bytesRemaining = 24;
			return USB_NO_MSG; /* use usbFunctionWrite() to receive data from host */
		}
	} else {
		/* ignore vendor type requests, we don't use any */
	}
	return 0;
}

/* ------------------------------------------------------------------------- */

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

/* main functions for irmp */
void timer_init(void) {
	/* IR polling timer */
	TCCR1B = (1 << WGM12) | (1 << CS10); // switch CTC Mode on, set prescaler to 1

	// may adjust IR polling rate here to optimize IR receiving:
	OCR1A = (F_CPU / F_INTERRUPTS) - 1; // compare value: 1/10000 of CPU frequency

	// enable Timer1 for IR polling
#if defined (__AVR_ATmega8__) || defined (__AVR_ATmega16__) || defined (__AVR_ATmega32__) || defined (__AVR_ATmega64__) || defined (__AVR_ATmega162__)
	TIMSK = 1 << OCIE1A; // Timer1A ISR activate
#else
	TIMSK1 = 1 << OCIE1A; // Timer1A ISR activate
#endif	// __AVR...
}
/* ------------------------------------------------------------------------- */

/* configure the ADC */
void adc_init(void) {
	//TODO: Tu cos by³o
	//DIDR0 |= 1 << ADC0D; // turn digit input ADC0 off to reduce power
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); //ADC prescaler set to divide by 128 -> 125kHz operating speed
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << REFS1); //External AVcc with a cap on Aref. 5V
	ADCSRA |= (1 << ADEN); // Enable ADC
	ADCSRA |= (1 << ADIE); // Enable ADC Interrupt
}
/* ------------------------------------------------------------------------- */

void TIMER1_COMPA_vect(void) __attribute__((interrupt));
void TIMER1_COMPA_vect(void) {
	/* main functions for irmp
	 */
	irmp_ISR(); // call irmp ISR
	if ((intCount++ % 512) == 0) {
		ADCSRA |= (1 << ADSC);
		//intCount = 0;
	}
}
#define NOOFSAMLES 128

/*ADC interput handler.*/
void ADC_vect(void) __attribute__((interrupt));
void ADC_vect(void) {
#define VOLTS_PER_BIT    0.00505 // 5.18/1024 = 0 | 0,00488 for 5V
#define ZERO_C_VOLTS     0.50000
#define MV_PER_DEGREE_C  0.01000
	static int sampleNo;
	static float adcAccumlator;

	sampleNo++;

	adcVal = (float) ADCW * VOLTS_PER_BIT;
	;

	if (adcVal > ZERO_C_VOLTS) {
		adcVal = ((adcVal - ZERO_C_VOLTS) / MV_PER_DEGREE_C);
		adcAccumlator += adcVal;
	} else {
		adcVal = -(ZERO_C_VOLTS - adcVal);
		adcVal = (adcVal / MV_PER_DEGREE_C);
		adcAccumlator += adcVal;
	}
	if (sampleNo == NOOFSAMLES) {
		adcAccumlator /= NOOFSAMLES;
		dtostrf(adcAccumlator, 6, 2, screenCenter[1]);
		isChanged = 1;
		adcAccumlator = 0;
		sampleNo = 0;
	}
}
/* ------------------------------------------------------------------------- */
unsigned char center(char* string) {
	uint8_t len = strlen(string);
	len = 16 - len;
	len /= 2;
	return len;
}

void printScreen(char screen[4][17]) {
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

int main(void) {
	uchar i;
	int intro = 1;

	wdt_enable(WDTO_1S);
	/* Even if you don't use the watchdog, turn it off here. On newer devices,
	 * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
	 */
	/* RESET status: all port bits are inputs without pull-up.
	 * That's the way we need D+ and D-. Therefore we don't need any
	 * additional hardware initialization.
	 */
	usbInit();
	usbDeviceDisconnect(); /* enforce re-enumeration, do this while interrupts are disabled! */
	i = 0;
	while (--i) { /* fake USB disconnect for > 250 ms */
		wdt_reset();
		_delay_ms(1);
	}
	usbDeviceConnect();
	sei();

	//Ports initialization and other piperials
	LCD_Initalize();
	LCD_Clear();

	/* About project screen */
	//LCD_GoTo(center("SMiW 2011/2012"), 0);
	//LCD_WriteText("SMiW 2011/2012");
	//LCD_GoTo(center("Marcin Jabrzyk"), 2);
	//LCD_WriteText("Marcin Jabrzyk");

	irmp_init(); //IR libary
	timer_init(); //IR timmer and ADC starter
	adc_init(); //ADC configuration

	intro = 0;
	//_delay_ms(1500);
	_delay_ms(500);
	if(RFM70_Initialize(0,(uint8_t*)"Smiw2")){
			LCD_GoTo(center("init RFM70"), 2);
			LCD_WriteText("init RFM70");
			_delay_ms(100);
	} else {
		LCD_GoTo(center("not init RFM70"), 1);
		LCD_WriteText("not init RFM70");
	}
	//_delay_ms(1500);
	LCD_GoTo(0,2);
	LCD_WriteText("after all");
	for (;;) { /* main event loop */
		wdt_reset();
		usbPoll();

		if (Packet_Received()) {
					Receive_Packet(message);
						strncpy(screenCenter[3], (char*)message, 7);
		}

		if (irmp_get_data(&irmp_data)) { // When IR decodes a new key presed.
			lastKey = irmp_data.command; //Save the key
			itoa(irmp_data.command, screenCenter[3], 10); //Convert it to string
			isChanged = 1;
			intro = 0;
		}
		if (intro == 0) {
			switch (lastKey) { //Change the view
			case 69:
				printScreen(screenLeft);
				break; //CH-
			case 70:
				printScreen(screenCenter);
				break; //CH
			case 71:
				printScreen(screenRight);
				break; //CH+
			default:
				printScreen(screenCenter);
				break; //Any other key
			}
		}
		usbPoll();
	}
	return 0;
}

/* ------------------------------------------------------------------------- */
