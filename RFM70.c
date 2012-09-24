#include "RFM70.h"
#include "SPI.h"
#include <string.h> //for memcpy and memcmp

#define Max_Payload_Length  32

#if Payload_Length > Max_Payload_Length
# warning "Payload_Length > Max_Payload_Length"
#endif

#define Enable_Chip_Enable()    CE_DDR  |= (1<<CE_BIT)
#define Chip_Enable()           CE_PORT |= (1<<CE_BIT)	
#define Chip_Disable()          CE_PORT &=~(1<<CE_BIT)	

//------------------------------------------------------------------------------
//start of RFM70 constants (register addresses, bitvalues, settings, ...)

//RFM70 commands:
#define R_REGISTER          0x00
#define W_REGISTER          0x20
#define R_RX_PAYLOAD        0x61
#define W_TX_PAYLOAD        0xA0
#define FLUSH_TX            0xE1
#define FLUSH_RX            0xE2
#define REUSE_TX_PL         0xE3
#define ACTIVATE            0x50
#define R_RX_PL_WID         0x60
#define W_ACK_PAYLOAD       0xA8
#define W_TX_PAYLOAD_NO_ACK 0xB0
#define NOP                 0xFF    //No Operation. Might be used to read the STATUS register

//RFM70 ACTIVATE commands:
#define ACTIVATE_Toggle     0x53
#define ACTIVATE_Features   0x73

//RFM70 register addresses @ Bank0:
#define CONFIG              0x00    //Power up/power down, CRC-settings, TX-/RC-mode, enable/disable interrupts,
#define EN_AA               0x01    //Enable Auto Acknowledgment
#define EN_RXADDR           0x02    //Enabled RX addresses
#define SETUP_AW            0x03    //Setup address width
#define SETUP_RETR          0x04    //Setup Auto Retransmit
#define RF_CH               0x05    //RF channel
#define RF_SETUP            0x06    //RF setup
#define STATUS              0x07    //Status
#define OBSERVE_TX          0x08    //Observe TX
#define CD                  0x09    //Carrier Detect
#define RX_ADDR_P0          0x0A    //RX address pipe0
#define RX_ADDR_P1          0x0B  
#define RX_ADDR_P2          0x0C  
#define RX_ADDR_P3          0x0D  
#define RX_ADDR_P4          0x0E  
#define RX_ADDR_P5          0x0F  
#define TX_ADDR             0x10    //TX address
#define RX_PW_P0            0x11    //RX payload width, pipe0
#define RX_PW_P1            0x12  
#define RX_PW_P2            0x13  
#define RX_PW_P3            0x14  
#define RX_PW_P4            0x15  
#define RX_PW_P5            0x16  
#define FIFO_STATUS         0x17    //FIFO Status

#define DYNPD               0x1C    //enable dynamic payload
#define FEATURE             0x1D    //feature

#define Max_Channel 82

//RFM70 register addresses @Bank1:
#define Chip_ID             0x08    //BEKEN Chip ID (RFM70=0x00000063)
const uint8_t RFM70_ID[]={0x63,0x00,0x00,0x00};//byte order reversed?

//RFM70 register bit values:

//CONFIG:
#define MASK_RX_DR  (1<<6)
#define MASK_TX_DS  (1<<5)
#define MASK_MAX_RT (1<<4)
#define EN_CRC      (1<<3)
#define CRCO        (1<<2)
#define PWR_UP      (1<<1)
#define PRIM_RX     (1<<0)

//EN_AA:
#define ENAA_P5     (1<<5)
#define ENAA_P4     (1<<4)
#define ENAA_P3     (1<<3)
#define ENAA_P2     (1<<2)
#define ENAA_P1     (1<<1)
#define ENAA_P0     (1<<0)

//EN_RXADDR:
#define ERX_P5      (1<<5)
#define ERX_P4      (1<<4)
#define ERX_P3      (1<<3)
#define ERX_P2      (1<<2)
#define ERX_P1      (1<<1)
#define ERX_P0      (1<<0)

//SETUP_AW:
enum Address_Width{Address_3_Bytes=1, Address_4_Bytes=2, Address_5_Bytes=3};

//STATUS:
#define RBANK       (1<<7)
#define RX_DR       (1<<6)
#define TX_DS       (1<<5)
#define MAX_RT      (1<<4)
#define RX_P_NO     (7<<1)
#define TX_FULL     (1<<0)  //same name as TX_FULL in FIFO_STATUS

//RF_SETUP:
#define RF_DR       (1<<3)
enum RF_Output_Power{dBm_n10=0<<1, dBm_n5=1<<1, dBm_0=2<<1, dBm_5=3<<1};    //power from negative 10dBm - to 5dBm
#define LNA_HCURR   (1<<0)

//FIFO_STATUS(TX_FULL is same name as TX_FULL in STATUS)
#define FIFO_STATUS_TX_REUSE    (1<<6)
#define FIFO_STATUS_TX_FULL 	(1<<5)
#define FIFO_STATUS_TX_EMPTY 	(1<<4)
#define FIFO_STATUS_RX_FULL 	(1<<1)
#define FIFO_STATUS_RX_EMPTY 	(1<<0)

//CD:
#define CD_CD       (1<<0)


//Bank1 should be loaded with constants from datasheet
/*
Data bits: MSB bit in each byte first
Data Bytes LSB first: Bank0 and Bank1_Reg9 to Bank1_Reg14
Data Bytes MSB first: Bank1_Reg0 to Bank1_Reg8
*/

static const uint8_t Bank1_Reg0_13[14][4]={
//{1st byte, 2nd byte, 3rd byte, 4th byte},
{0x40,0x4B,0x01,0xE2},                              //Must write with 0x404B01E2(reversed)    
{0xC0,0x4B,0x00,0x00},                              //Must write with 0xC04B0000 
{0xD0,0xFC,0xBC,0x02},                              //Must write with 0xD0FC8C02
{0x99,0x00,0x39,0x41},                              //Must write with 0x99003941
{0xD9,0x9E,0x86,0x0B},                              //Must write with 0xD99E860B(High Power)
{0x24&~(15<(26-24)),0x06&~(1<<(18-16)),0x7F,0xA6},  //enable RSSI, Threshold -97dBm,
{0x00,0x00,0x00,0x00},                              //reserved    
{0x00,0x00,0x00,0x00},                              //reserved, RBANK=read only  
{0x00,0x00,0x00,0x00},                              //Chip ID 
{0x00,0x00,0x00,0x00},                              //reserved
{0x00,0x00,0x00,0x00},                              //reserved
{0x00,0x00,0x00,0x00},                              //reserved
{0x00,0x12,0x73,0x00},                              //Please initialize with 0x00731200  
{0x36,0xB4,0x80,0x00}                               //Please initialize with 0x0080B436  
};

/*
Ramp curve 
Please write with  
0xFFFFFEF7CF208104082041  
*/
static const uint8_t Bank1_Reg14[]={0x41,0x20,0x08,0x04,0x81,0x20,0xCF,0xF7,0xFE,0xFF,0xFF}; 
	
//end of RFM70 constants (register addresses, bitvalues, settings, ...)
//------------------------------------------------------------------------------
	
	                 

//Write a value to a register with W_register command
void SPI_Write_Register(uint8_t Register, uint8_t Value){
    Select_Slave();                         
    SPI_Master_Send(W_REGISTER|Register);   //select register...
	SPI_Master_Send(Value);                 //and write value to it
	Unselect_Slave();
}                                                         
 

//read a value from a register with R_register command
uint8_t SPI_Read_Register(uint8_t Register){                                                           
	uint8_t value;
	
	Select_Slave();
	SPI_Master_Send(R_REGISTER|Register);   //Select register to read from..
	value = SPI_Master_Receive();           //then read register value
	Unselect_Slave();
	return(value); 
}                                                           
 

//read value from multi byte register and store in array                                            
void SPI_Read_Buffer(uint8_t Register, volatile uint8_t* Buffer, uint8_t Length){                                                           
	uint8_t Byte_counter;                              
                                                            
	Select_Slave();
	SPI_Master_Send(Register);  //Select target register, and read Status
                                                            
	for(Byte_counter=0; Byte_counter<Length; Byte_counter++){	
        Buffer[Byte_counter] = SPI_Master_Receive();  //then write all data in buffer
	}	                                            
    Unselect_Slave();               
}   
                                                   

//write data from array to multi byte register
void SPI_Write_Buffer(uint8_t Register,const uint8_t *Buffer, uint8_t Length){                                                           
	uint8_t Byte_counter;                                
                                                            
	Select_Slave();
	SPI_Master_Send(Register);  //Select target register, and read Status..
	for(Byte_counter=0; Byte_counter<Length; Byte_counter++){ 
		SPI_Master_Send(*Buffer++);   //then write all data from buffer
	}		                              
	Unselect_Slave();
}


//Some commands have no data, saves time and prevents undefined behavior
uint8_t SPI_Write_Command(uint8_t Command){                                                           
	uint8_t Status;                     
	Select_Slave();
	Status=SPI_Master_Send(Command);          
	Unselect_Slave();
	return Status;
}

//NOP command has no effect, module always returns value of STATUS
#define SPI_Read_STATUS() SPI_Write_Command(NOP)

//switches module to RX-mode, use after Send_Packet to receive data
void Select_RX_Mode(){
	uint8_t Value;
	SPI_Write_Command(FLUSH_RX);    //flush RX

	Value=SPI_Read_STATUS();
	SPI_Write_Register(STATUS,Value);  //clear RX_DR or TX_DS or MAX_RT interrupt flag
    
	Chip_Disable();                                 //without this, the module won't switch modes properly
	Value=SPI_Read_Register(CONFIG);	            //keep CONFIG's value
	if( (Value&PRIM_RX)==0 ){                       //switch if NOT in RX-mode
	    SPI_Write_Register(CONFIG, Value|PRIM_RX);  //set PRIM_RX in CONFIG's value and write it back back
	}
	Chip_Enable();                                  //without this, the module won't switch modes properly	
}

//switches module to TX-mode
void Select_TX_Mode(){
	uint8_t Config;
	SPI_Write_Command(FLUSH_TX); //flush TX
	
	Chip_Disable();                                     //without this, the module won't switch modes properly
	Config=SPI_Read_Register(CONFIG);                   //keep CONFIG's value
	if( (Config&PRIM_RX)!=0 ){                          //switch if in RX-mode
		SPI_Write_Register(CONFIG, Config&(~PRIM_RX));  //clear PRIM_RX in CONFIG's value and write it back back
	}
	Chip_Enable();                                      //without this, the module won't switch modes properly
}

/*
Command: ACTIVATE

This write command followed by data 0x53 toggles 
the register bank, and the current register bank 
number can be read out from REG7(STATUS)
*/
//sets bank to 0 or 1, other values set bank to 1
void Switch_Bank(uint8_t Bank){
	uint8_t Data=ACTIVATE_Toggle;
	uint8_t Status=SPI_Read_STATUS();
	
	if( ((Status&RBANK)==0) != (Bank==0) ){ //logic XOR, if not in desired bank, switch
		SPI_Write_Buffer(ACTIVATE,&Data,1); //toggle between Bank0 and Bank1
	}
}

//Max_Channel=82, channel>82 -> channel=82
void Set_Channel(uint8_t Channel){
	if(Channel>Max_Channel) Channel = Max_Channel;
	SPI_Write_Register(RF_CH,Channel);
}

/*
Command: ACTIVATE

This write command followed by data 0x73 activates 
the following features: 
•  R_RX_PL_WID
•  W_ACK_PAYLOAD 
•  W_TX_PAYLOAD_NOACK 
A new ACTIVATE command with the same data 
deactivates them again. This is executable in power 
down or stand by modes only. 
 
The R_RX_PL_WID, W_ACK_PAYLOAD, and 
W_TX_PAYLOAD_NOACK features registers are 
initially in a deactivated state; a write has no effect, a 
read only results in zeros on MISO. To activate these 
registers, use the ACTIVATE command followed by 
data 0x73. Then they can be accessed as any other 
register. Use the same command and data to 
deactivate the registers again. 

summary:
Check if activated, if not activated; activate
Run before putting device in rx-mode
*/
void Activate_Feature_registers(){
	uint8_t Feature;
	uint8_t Data=ACTIVATE_Features;

	Feature=SPI_Read_Register(FEATURE);

	if(Feature==0){ //not activated yet
		SPI_Write_Buffer(ACTIVATE,&Data,1);
	}	
}

//returns true when handshake with module is successful
bool RFM70_Present(){
	uint8_t Data[sizeof(RFM70_ID)];
	
	Switch_Bank(1);  //Chip_ID is located in Bank1
	SPI_Read_Buffer(Chip_ID,Data,sizeof(RFM70_ID));
	Switch_Bank(0);  //switch back
	
	return memcmp(Data, RFM70_ID,sizeof(RFM70_ID)) == 0;    //int memcmp ( const void * ptr1, const void * ptr2, size_t num );	
}

/*
configures RFM70 as receiver, Send_Packet automatically switches device to TX-mode,
switch back manually to RX-mode with Select_RX_Mode()
returns true when handshake with module is successful
*/
bool RFM70_Initialize(uint8_t Channel, uint8_t* Address){
	uint8_t i;//loop counter
	 
	SPI_Master_Init(CLK_Div_16);
	Enable_Chip_Enable();
	Chip_Enable();
	
	if(!RFM70_Present()){
	    return false;	
	}
	
	//start at Bank0
	Activate_Feature_registers();
	
	SPI_Write_Register(CONFIG,MASK_RX_DR|MASK_TX_DS|MAX_RT|EN_CRC|CRCO|PWR_UP|PRIM_RX);
	SPI_Write_Register(EN_AA,0);
	SPI_Write_Register(EN_RXADDR,ERX_P0);
	SPI_Write_Register(SETUP_AW,Address_5_Bytes);
	SPI_Write_Register(RF_SETUP,dBm_5|LNA_HCURR);   //todo: uses defines for power and data rate
	SPI_Write_Register(RX_PW_P0, Payload_Length);
	
	Set_Channel(Channel);

	SPI_Write_Buffer(W_REGISTER|RX_ADDR_P0,Address,5);
	SPI_Write_Buffer(W_REGISTER|TX_ADDR   ,Address,5);    //TX-address is address of receiver, Master and slave use same address so they are equal
	
	Switch_Bank(1);

	for(i=0;i<=6;i++){	
		SPI_Write_Buffer((W_REGISTER|i),&Bank1_Reg0_13[i][0],4);
	}
	//registers 7-11 are reserved
	for(i=12;i<=13;i++){	
		SPI_Write_Buffer((W_REGISTER|i),&Bank1_Reg0_13[i][0],4);
	}
	SPI_Write_Buffer((W_REGISTER|14),Bank1_Reg14,11);   //register 14 is extra wide

	Switch_Bank(0);  //back again
	//Select_RX_Mode();//RX-mode is default, switch back manually after TX-mode
	
	return true;    //success
}

//send any data type not greater than Payload_Length, returns true on success
bool Send_Packet(const void *Buffer, uint8_t Length){
	uint8_t FIFO_Status;
	uint8_t Payload[Payload_Length];

	
	if(Length>Payload_Length){
	    return false;	
	}
    memcpy(Payload, Buffer, Length);  //void * memcpy ( void * destination, const void * source, size_t num ) returns destination
	
	Select_TX_Mode();
	    
	Chip_Disable(); //prevent device from sending packet after 1 byte 	
	FIFO_Status=SPI_Read_Register(FIFO_STATUS); //read register FIFO_STATUS's value
	if( (FIFO_Status&FIFO_STATUS_TX_FULL)==0 ){                     //if not full, send data (write buff)
		SPI_Write_Buffer(W_TX_PAYLOAD, Payload, Payload_Length);    //writes data to buffer
	}
	else{
		return false;		
	}	
	Chip_Enable();  //done with updating payload
	return true;
}


#define FIFO_Size 3

/*
use only after Packet_Received(); function can clear interrupt in worst case scenario
if packet is received, function will fill RX_Buffer with received data
*/
void Receive_Packet(void *Buffer){
	uint8_t Length, Status, FIFO_Status;
	uint8_t Loop_Counter;

	Status=SPI_Read_Register(STATUS);	// read register STATUS's value

	if( (Status&RX_DR)!=0 ){				// if receive data ready (RX_DR) interrupt
		Loop_Counter=0;
		do{
			Length=SPI_Read_Register(R_RX_PL_WID);
			SPI_Read_Buffer(R_RX_PAYLOAD,Buffer,Length);// read receive payload from RX_FIFO buffer
			FIFO_Status=SPI_Read_Register(FIFO_STATUS);
			Loop_Counter++;//emptied one FIFO, exit loop after FIFO_Size cycles to prevent freezing
		}while( ((FIFO_Status&FIFO_STATUS_RX_EMPTY)==0) && (Loop_Counter<FIFO_Size) ); //while not empty and not stuck
	}
	
	SPI_Write_Register(STATUS,RX_DR);// clear RX_DR interrupt flag	
}

//returns true if packet received returns, false after Receive_Packet()
bool Packet_Received(){
    return (SPI_Read_STATUS() & RX_DR) != 0;
}


//returns true if data is send, clears interrupt, so result is volatile
bool Packet_Send(){
	uint8_t Status=SPI_Read_STATUS();
	
	if( (Status&TX_DS) != 0 ){
		SPI_Write_Register(STATUS,Status);//clear TX_DS
		return true;		
	}
	else{
		return false;		
	}
}

/*
returns true if carrier detected, result is volatile
wait at least 150us before polling again
*/
bool Carrier_Detected(){
	return (SPI_Read_Register(CD) & CD_CD) != 0;
}
