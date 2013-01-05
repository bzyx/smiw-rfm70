#include <avr/io.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hd44780.h"
#include "irmp.h"
#include "irmpconfig.h"

#include "RFM70.h"
#include "protocol-active.h"

typedef unsigned char uchar;

static IRMP_DATA irmp_data;
static command_t command;
static uint8_t isChanged = 1;

float adcVal;
static int lastKey;

static char screenLeft[4][17] = { "Ekran Lewy", "-", "-", "-" };
static const char screenCenterTemplate[4][17] = { "Ten:     %s", "Piec: %s",
		"Grzej:    %s", "Przycisk:    %s" };
static char screenCenter[4][17] = { "Ten:   ", "Piec:    ", "Grzej: ",
		"Przycisk:    " };
static char screenRight[4][17] = { "Ekran Prawy", "-", "-", "-" };
static const char screenDebugTemplate[4][17] = { "RFM: %s", "Nosna: %s",
		"Odbior: %s", "" };
static char screenDebug[4][17] = { "RFM: %s", "Nosna: %s", "Odbior: %s", "" };

static int intCount = 0;
static char message[32] = "";
static char bufor[32] = "";
static char tempFromMCP[10];
static char lastKeyStr[4];

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
		//dtostrf(adcAccumlator, 6, 2, screenCenter[1]);
		dtostrf(adcAccumlator, 4, 1, tempFromMCP);

		sprintf(screenCenter[0], screenCenterTemplate[0], tempFromMCP);
		char znaki[3] = { 0xDF, 'C' };
		strcat(screenCenter[0], znaki);
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

int main(void) {
	int intro = 1;
	static char carrierErrorCount = 0;
	static char reciveErrorCount = 0;
	int z = 0;
	static bool recv = false;
	static int stop = 0;

	LCD_Initalize();
	LCD_Clear();

	intro = 0;
	if (RFM70_Initialize(0, (uint8_t*) "Smiw2")) {
		LCD_GoTo(center("Init RFM70"), 0);
		LCD_WriteText("Init RFM70");
		_delay_ms(100);
	} else {
		LCD_GoTo(center("ERR init RFM70"), 0);
		LCD_WriteText("ERR init RFM70");
		_delay_ms(100);
	}

	LCD_Clear();

	if (RFM70_Present()) {
		LCD_GoTo(center("RFM70 present"), 0);
		LCD_WriteText("RFM70 present");
		_delay_ms(100);
	} else {
		LCD_GoTo(center("RFM70 not present"), 0);
		LCD_WriteText("RFM70 not present");
		_delay_ms(100);
	}

	irmp_init(); //IR libary
	timer_init(); //IR timmer and ADC starter
	adc_init(); //ADC configuration

	sei();

	for (;;) { /* main event loop */
		if (z == 0){
		clearCommand(&command);
				 setCommandValues(&command, GRZEJNIK_NODE_ID, GRZEJNIK_READ_TEMP,
				 tempFromMCP);
				 encodeMessage(&command, message);
		Send_Packet(message, strlen(message));
		z = 1;
		} else {
		clearCommand(&command);
				 setCommandValues(&command, PIEC_NODE_ID, PIEC_READ_TEMP,
				 "11.01");
				 encodeMessage(&command, message);
		Send_Packet(message, strlen(message));
		z = 0;
		}

		//sprintf(screenCenter[1], screenCenterTemplate[1], "SEND");
		//isChanged = 1;
		Select_RX_Mode();
		_delay_ms(50);

		if (RFM70_Present()) {
			sprintf(screenDebug[0], screenDebugTemplate[0], "OK");
		} else {
			sprintf(screenDebug[0], screenDebugTemplate[0], "ERROR");
		}

		_delay_us(150);
		if (Carrier_Detected()) {
			sprintf(screenDebug[1], screenDebugTemplate[1], "OK");
			carrierErrorCount = 0;
		} else {
			carrierErrorCount++;
		}

		if (carrierErrorCount > 50) {
			sprintf(screenDebug[1], screenDebugTemplate[1], "NONE");
		}

		//cli();
		recv = Packet_Received();
		//sei();
		while( ( recv == false) || (stop == 0) ){
			cli();
			Select_RX_Mode();
			_delay_ms(1);
			recv = Packet_Received();
			sei();
	    	stop++;

	    	if (stop > 512){
	    		stop = 0;
	    		break;
	    	}

	    	LCD_GoTo(0,0);
	    	itoa(stop, bufor, 10);
	    	sprintf(message, "%d  B: %d", stop, recv);
	    	LCD_WriteText(message);
	    }

		if (recv) {
			sprintf(screenLeft[0], "PRCV: %s", "YESYES");
			Receive_Packet(bufor);
			reciveErrorCount = 0;

			if (decodeMessage(bufor, &command) == 0){
				sprintf(message, "%d %d %.5s", command.nodeId, command.funcId, command.value);
				sprintf(screenCenter[1], screenCenterTemplate[1], message);
				isChanged = 1;
				_delay_ms(150);
			}
		} else {
			reciveErrorCount++;
		}

		if (reciveErrorCount > 90) {
			sprintf(screenRight[0], screenDebugTemplate[2], "WAIT");
		}

		if (irmp_get_data(&irmp_data)) { // When IR decodes a new key presed.
			lastKey = irmp_data.command; //Save the key
			itoa(irmp_data.command, lastKeyStr, 10); //Convert it to string
			sprintf(screenCenter[3], screenCenterTemplate[3], lastKeyStr);
			isChanged = 1;
			intro = 0;
		}

		switch (lastKey) { //Change the view
		case 69:
			printScreenWithCenter(screenLeft);
			break; //CH-
		case 70:
			printScreen(screenCenter);
			break; //CH
		case 71:
			printScreenWithCenter(screenRight);
			break; //CH+
		case 82:
			printScreen(screenDebug);
			break;
		default:
			printScreen(screenCenter);
			break; //Any other key
		}
	}
	return 0;
}
