/******************************************************************************
*Project title:     RFM70_hd_1.3
*Filename:          SPI.h
*Version and date:  V1.3 2011-12-14
*Author:            Chis Idema
*Purpose:           Send bytes using SPI-peripheral in AVR
*******************************************************************************/

#ifndef _SPI_h
#define _SPI_h

#include <avr/io.h>     //for register addresses and bit names of AVR
#include <stdint.h>     //for uint8_t


#define SCK_BIT  PINB5
#define MISO_BIT PINB4
#define MOSI_BIT PINB3
#define SSN_BIT  PINB2  //SSN/CSN (Slave Select Not / CSN Chip Select Not)
#define SPI_DDR  DDRB
#define SPI_PORT PORTB

#define Select_Slave()   SPI_PORT &=~(1<<SSN_BIT)   //active low
#define Unselect_Slave() SPI_PORT |= (1<<SSN_BIT)   //active low
/*
todo

current situation:

    SPI-commands
          ____
    _____|    |_______

    Slave select
    ___          _____
       |________|
   
test situation(can be used if SPI is a bus):

    SPI-commands
          ____
    _____|    |_______

    Slave select
      _          _
    _| |________| |___

would save energy in voltage divider, but might require extra energy from spi in module
*/

enum SPI_Speeds{CLK_Div_16, CLK_Div_8, CLK_Div_4, CLK_Div_2};   //1MHz,2MHz,4MHz,8MHz @ F_CPU=16MHz

void SPI_Master_Init(enum SPI_Speeds SPI_Speed);    //SPI=master, busy waiting, SPI_Speed and F_CPU determine SCK-frequency
uint8_t SPI_Master_Send(uint8_t SPI_Byte);          //busy waiting, no interrupt, saves energy by making MOSI input after transmission
#define SPI_Master_Receive() SPI_Master_Send(0)     //send dummy byte

#endif
