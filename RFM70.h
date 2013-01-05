/******************************************************************************
*Project title:     RFM70_hd_V1_6
*Filename:          RFM70.h
*Version and date:  V1.6 2012-01-04
*Author:            Chis Idema
*Purpose:           Send bytes using SPI-peripheral in AVR
*******************************************************************************/

#ifndef _RFM70_h
#define _RFM70_h

#include <avr/io.h>     //for register addresses and bit names of AVR
#include <stdint.h>     //for uint8_t
#include <stdbool.h>    //for bool functions

#define CE_BIT  PB1   // Chip Enable
#define CE_PORT PORTB
#define CE_DDR  DDRB

#define Payload_Length 32   //enter desired payload Length <= 32 or be warned by pre-compiler

bool RFM70_Initialize(uint8_t Channel, uint8_t* Address);       //channel 0 to 82, 5 byte address, returns true when handshake with module is successful
void Receive_Packet(void *Buffer);                              //if packet is received, function will fill RX_Buffer with received data
bool Send_Packet(const void *Buffer,uint8_t Length);            //send any data type, returns true on success
bool Packet_Received();                                         //returns true if packet received returns, false after Receive_Packet()
bool Packet_Send();                                             //returns true if data is send, clears interrupt, so result is volatile
bool Carrier_Detected();                                        //returns true if carrier detected, wait at least 150us before polling again
bool RFM70_Present();                                           //returns true when handshake with module is successful
void Select_RX_Mode();                                          //switches module to RX-mode, use after Send_Packet() to receive data

#endif
