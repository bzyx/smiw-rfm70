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
#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include <avr/eeprom.h>
#include <inttypes.h>
#include <stdlib.h>
#include <util/atomic.h>
#include <string.h>
#include <stdio.h>

#include "hd44780.h"
#include "irmp.h"
#include "irmpconfig.h"
#include "RFM70.h"
#include "LCD_Utils.h"
#include "Screen_Defs.h"

#include "protocol-active.h"

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM char usbHidReportDescriptor[22] = { /* USB report descriptor */
0x06, 0x00, 0xff, // USAGE_PAGE (Generic Desktop)
		0x09, 0x01, // USAGE (Vendor Usage 1)
		0xa1, 0x01, // COLLECTION (Application)
		0x15, 0x00, //   LOGICAL_MINIMUM (0)
		0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
		0x75, 0x03, //   REPORT_SIZE (8) -!!!!!!!! by³o 0x01
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
static IRMP_DATA irmp_data;
static command_t commandStruct;

//static double adcVal;
float adcVal;
static int lastKey;

static int intCount = 0;
static char message[32] = "";
static char bufor[32] = "";
int intro = 1;

static uchar bytesRemaining;
static char temperatureFromMCP[10];
static char temperatureFromPiec[10];
static char temperatureFromGrzejnik[10];
static char temperatureFromUsbToGrzej[10];
static char temperatureFromUsbToPiecWl[10];
static char temperatureFromUsbToPiecWyl[10];
static char lastKeyStr[4];
static uchar lineNo;
char znaki[4] = { 0xDF, 'C', '\0' };

/* usbFunctionRead() is called when the host requests a chunk of data from
 * the device. For more information see the documentation in usbdrv/usbdrv.h.
 */uchar usbFunctionRead(uchar *data, uchar len) {
	uchar i;

	if (len > bytesRemaining) {
		len = bytesRemaining;
		//send temp3 + irKey
		for (i = 0; i < 4; i++) {
			data[i] = temperatureFromGrzejnik[i];
		}
		for (i = 0; i < 2; i++) {
			data[i + 4] = lastKeyStr[i];
		}
	} else {
		//send temp1 + temp2
		for (i = 0; i < 4; i++) {
			data[i] = temperatureFromMCP[i];
		}
		for (i = 0; i < 4; i++) {
			data[i + 4] = temperatureFromPiec[i];
		}
		bytesRemaining -= len;

	}
	return len;                             // return real chunk size
}

/* usbFunctionWrite() is called when the host sends a chunk of data to the
 * device. For more information see the documentation in usbdrv/usbdrv.h.
 */uchar usbFunctionWrite(uchar *data, uchar len) {

	if (len > bytesRemaining) // if this is the last incomplete chunk
		len = bytesRemaining; // limit to the amount we can store
	/* D³ugosc pakietu wykosi 16 bajtow, i jest dzielona na 2 po 8.
	 * Poniewaz potrzebujemy tu 4 pierwszych bajtow mozemy pomnozyc x 2
	 * ilosc przetworzonych danych. W ten sposob poinformujemy hosta
	 * o zakoczeniu transmisji, mimo ze przeslal nam tylko 8 bajtow.
	 */
	bytesRemaining -= 2 * len;

	if (lineNo == 0) {
		lineNo = data[0];
		return bytesRemaining;
	}

	switch (lineNo) {
	case 1: { // Grzejnik
		strncpy(temperatureFromUsbToGrzej, data, 4);
		lineNo = 0;
		return bytesRemaining;
	}
		break;
	case 2: { // Piec W³¹cz
		strncpy(temperatureFromUsbToPiecWl, data, 4);
		lineNo = 0;
		return bytesRemaining;
	}
		break;
	case 3: { // Piec Wy³¹cz
		strncpy(temperatureFromUsbToPiecWyl, data, 4);
		lineNo = 0;
		return bytesRemaining;
	}
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		return bytesRemaining;
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
			bytesRemaining = 15;
			return USB_NO_MSG ; /* use usbFunctionRead() to obtain data */
		} else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
			/* since we have only one report type, we can ignore the report-ID */
			bytesRemaining = 24;
			return USB_NO_MSG ; /* use usbFunctionWrite() to receive data from host */
		}
	} else {
		/* ignore vendor type requests, we don't use any */
	}
	return 0;
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
	/* main functions for irmp */
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
#define VOLTS_PER_BIT    0.00410 // 4.20/1024 = 0.00410 0.00505 // 5.18/1024 = 0 | 0,00488 for 5V
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
		//dtostrf(adcAccumlator, 6, 2, screenCenter[1]);
		dtostrf(adcAccumlator, 4, 1, temperatureFromMCP);

		sprintf(screenCenter[3], screenCenterTemplate[3], temperatureFromMCP);
		//strcat(screenCenter[0], znaki);
		isChanged = 1;
		adcAccumlator = 0;
		sampleNo = 0;
	}
}

void valueToScreen(command_t* command) {
	char tmp[10];
	strncpy(tmp, command->value, 4);
	tmp[4] = '\0';
	if (command->nodeId == GRZEJNIK_NODE_ID) {
		if (command->funcId == GRZEJNIK_READ_TEMP) {
			;
			sprintf(screenLeft[1], screenLeftTemplate[1], tmp);
			strcpy(temperatureFromGrzejnik, tmp);
			isChanged = 1;
		}
	}

	if (command->nodeId == PIEC_NODE_ID) {
		if (command->funcId == PIEC_READ_TEMP) {
			;
			sprintf(screenRight[1], screenRightTemplate[1], tmp);
			strcpy(temperatureFromPiec, tmp);
			isChanged = 1;
		}
	}

}

void updateValuesFromUSB() {
	if (strlen(temperatureFromUsbToGrzej) > 0) {
		temperatureFromUsbToGrzej[2] = '.';
		sprintf(screenLeft[2], screenLeftTemplate[2],
				temperatureFromUsbToGrzej);
		isChanged = 1;
	}
	if (strlen(temperatureFromUsbToPiecWl) >0 ) {
		temperatureFromUsbToPiecWl[2] = '.';
		sprintf(screenRight[2], screenRightTemplate[2],
				temperatureFromUsbToPiecWl);
		isChanged = 1;
	}
	if (strlen(temperatureFromUsbToPiecWyl)>0) {
		temperatureFromUsbToPiecWyl[2] = '.';
		sprintf(screenRight[3], screenRightTemplate[3],
				temperatureFromUsbToPiecWyl);
		isChanged = 1;
	}
}

void IRrecAndUpdateScreen() {
	char oldLastKey = lastKey;
	if (irmp_get_data(&irmp_data)) { // When IR decodes a new key presed.
		lastKey = irmp_data.command; //Save the key
		if (lastKey != 82) {
			itoa(oldLastKey, lastKeyStr, 10); //Convert it to string
			isChanged = 1;
		}
		//sprintf(screenCenter[3], screenCenterTemplate[3], lastKeyStr);
		isChanged = 1;
		intro = 0;
	}

	if (oldLastKey != lastKey) {
		LCD_Clear();
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
		case 82:
			printScreen(screenDebug);
			break;
		default:
			printScreen(screenCenter);
			break; //Any other key
		}
	}
}

void SendAllData() {
	static int whatToSend = 0;
	clearCommand(&commandStruct);

	if ((whatToSend == 0) && (strlen(temperatureFromUsbToGrzej) > 0 ) ) {
		setCommandValues(&commandStruct, GRZEJNIK_NODE_ID,
				GRZEJNIK_SET_DESIRED_TEMP, temperatureFromUsbToGrzej);
	}

	if ((whatToSend == 1) && (strlen(temperatureFromUsbToPiecWl) > 0) ) {
		setCommandValues(&commandStruct, PIEC_NODE_ID, PIEC_SET_ON_TEMP,
				temperatureFromUsbToPiecWl);
	}

	if ((whatToSend == 2) && (strlen(temperatureFromUsbToPiecWyl) > 0 ) ) {
		setCommandValues(&commandStruct, PIEC_NODE_ID, PIEC_SET_OFF_TEMP,
				temperatureFromUsbToPiecWyl);
	}

	if (commandStruct.nodeId != 0) {
		encodeMessage(&commandStruct, bufor);
		Send_Packet(bufor, strlen(bufor));
		Send_Packet(bufor, strlen(bufor));
		_delay_ms(1);
		Send_Packet(bufor, strlen(bufor));
		Send_Packet(bufor, strlen(bufor));

		/*sprintf(screenLeft[0], bufor);
		isChanged = 1;*/
	}

	whatToSend = whatToSend + 1;
	whatToSend = whatToSend % 3;

}

int main(void) {
	uchar i;
	static char reciveErrorCount = 0;
	static char carrierErrorCount = 0;
	static int stop = 1;
	static bool recv = false;

	wdt_enable(WDTO_1S);
	//wdt_enable(WDTO_8S);
	/* Even if you don't use the watchdog, turn it off here. On newer devices,
	 * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
	 * RESET status: all port bits are inputs without pull-up.
	 * That's the way we need D+ and D-. Therefore we don't need any
	 * additional hardware initialization.
	 */

	usbInit();
	usbDeviceDisconnect(); /* enforce re-enumeration, do this while interrupts are disabled! */
	i = 0;
	while (--i) { // fake USB disconnect for > 250 ms
		wdt_reset();
		_delay_ms(1);
	}
	usbDeviceConnect();
	//sei();

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

	cli();
	intro = 0;
	if (RFM70_Initialize(0, (uint8_t*) "Smiw2")) {
		LCD_GoTo(center("Init RFM70"), 2);
		LCD_WriteText("Init RFM70");
		//_delay_ms(100);
	} else {
		LCD_GoTo(center("ERR init RFM70"), 1);
		LCD_WriteText("ERR init RFM70");
		_delay_ms(100);
	}

	//_delay_ms(1000);
	LCD_Clear();

	if (RFM70_Present()) {
		LCD_GoTo(center("RFM70 present"), 3);
		LCD_WriteText("RFM70 present");
		//_delay_ms(100);
	} else {
		LCD_GoTo(center("RFM70 not present"), 3);
		LCD_WriteText("RFM70 not present");
		_delay_ms(100);
	}

	/*
	 * Prawodopodbnie któras z biblotek nieporpawnie inicjalizowa³a porty.
	 * I ustawia³a PB0 jako wyjcie. Kiedy ma byc wejsciem odbiornika podczerwienii.
	 */DDRB &= ~(1 << PB0);
	sei();
	for (;;) { /* main event loop */
		wdt_reset();
		usbPoll();

		SendAllData();
		Select_RX_Mode();

		if (RFM70_Present()) {
			sprintf(screenDebug[0], screenDebugTemplate[0], "OK", lastKeyStr);
		} else {
			sprintf(screenDebug[0], screenDebugTemplate[0], "ER", lastKeyStr);
		}

		if (Carrier_Detected() == true) {
			sprintf(screenDebug[1], screenDebugTemplate[1], "OK");
			carrierErrorCount = 0;
		} else {
			carrierErrorCount++;
		}
		if (carrierErrorCount > 50) {
			sprintf(screenDebug[1], screenDebugTemplate[1], "NONE");
		}

		if (reciveErrorCount > 90) {
			sprintf(screenDebug[2], screenDebugTemplate[2], "WAIT");
		} else {
			sprintf(screenDebug[2], screenDebugTemplate[2], "OK");
		}

		wdt_reset();
		usbPoll();

		IRrecAndUpdateScreen();
		//wdt_reset();
		//usbPoll();

		//cli();
		recv = Packet_Received();
		//sei();
		while ((recv == false) || (stop == 0)) {
			//cli();
			Select_RX_Mode();
			_delay_ms(1.5);
			recv = Packet_Received();
			//sei();
			stop++;

			if (stop % 32) {
				IRrecAndUpdateScreen();
			}

			if (stop % 128) {
				wdt_reset();
				usbPoll();
			}

			if (stop > 1024) {
				stop = 0;
				break;
			}

			itoa(stop, bufor, 10);
			sprintf(message, "B:%.1d %4d", recv, stop);
			sprintf(screenDebug[3], screenDebugTemplate[3], message);
		}

		wdt_reset();
		usbPoll();

		if (recv == true) {
			//wdt_reset();
			//usbPoll();
			stop = 1;
			Receive_Packet(message);
			decodeMessage(message, &commandStruct);
			valueToScreen(&commandStruct);
			//wdt_reset();
			//usbPoll();
			//_delay_ms(250);

			reciveErrorCount = 0;
		} else {
			reciveErrorCount++;
		}
		updateValuesFromUSB();
		//wdt_reset();
		//usbPoll();

		wdt_reset();
		usbPoll();

	}
	return 0;
}
