#include "SPI.h"

//MOSI is input by default to save energy in voltage divider
#define MOSI_OUTPUT()  SPI_DDR |= (1<<MOSI_BIT)
#define MOSI_INPUT()   SPI_DDR &=~(1<<MOSI_BIT)

//SPI=master, busy waiting, SPI_Speed and F_CPU determine SCK-frequency
void SPI_Master_Init(enum SPI_Speeds SPI_Speed){
	
  SPI_DDR |= (1<<SCK_BIT)|(1<<SSN_BIT); //SCK and SSN are outputs, MOSI is input by default to save energy
  
  Unselect_Slave();         //pins are low by default, slave select is inverted
  SPCR = (1<<SPE)|(1<<MSTR);//enable SPI, MSB first, SPI=master, SCK low when idle(saves energy), sample on leading edge(rising)
  
  switch(SPI_Speed){
	  case CLK_Div_16:
	    SPCR|=(1<<SPR0);    //FCPU/16	  
	  break;
	 
	  case CLK_Div_8:
	    SPCR|=(1<<SPR0);    //FCPU/16	
		SPSR|=(1<<SPI2X);   //FCPU/16*2=FCPU/8
	  break;
	  
	  case CLK_Div_4:
	                        //FCPU/4 (default)	
	  break;
	  
	  case CLK_Div_2:
	                        //FCPU/4 (default)	
	    SPSR|=(1<<SPI2X);	//FCPU/4*2=FCPU/2
	  break;
  }  
}

//Send SPI_Byte over SPI, returns MISO data, busy waiting, no interrupt, saves energy by making MOSI input after transmission
uint8_t SPI_Master_Send(uint8_t SPI_Byte){  
  MOSI_OUTPUT();                    //MOSI is input by default to save energy
  SPDR = SPI_Byte;                  //put data byte in transmit register
  while(!(SPSR & (1<<SPIF)));       //wait for transmission to complete (2us@4MHz)
  MOSI_INPUT();                     //save energy
  return SPDR;                      //SPI=full duplex, always returns data
}

