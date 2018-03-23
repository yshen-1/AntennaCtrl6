/*
 * main.c
 *
 * Created: 3/23/2017 12:32:50 PM
 * Author : Lou
 */ 

#include "AntCtrl.h"


/** Circular buffer to hold data from the host */
//static RingBuffer_t FromHost_Buffer;

/** Underlying data buffer for \ref FromHost_Buffer, where the stored bytes are located. */
//static uint8_t      FromHost_Buffer_Data[128];

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
{
	.Config =
	{
		.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
		.DataINEndpoint           =
		{
			.Address          = CDC_TX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.DataOUTEndpoint =
		{
			.Address          = CDC_RX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.NotificationEndpoint =
		{
			.Address          = CDC_NOTIFICATION_EPADDR,
			.Size             = CDC_NOTIFICATION_EPSIZE,
			.Banks            = 1,
		},
	},
};



int main(void)
{

	setupHardware();
	//sei(); /*  Enable interrupts */
	//GlobalInterruptEnable();
	
	while(1){
		//pot_set(0) sets voltage to 40 V.
		//pot_set(128) sets voltage to 75 V.
		//pot values are inverted b/c P0W and P0B are shorted.
		pot_set(128);
		_delay_ms(5);
	}
	return -1;
}

uint8_t SendPulse(uint8_t polarity, uint16_t epmNum) {
	// Sets up MUX and delivers pulse to H-bridge controller
		
	char reply_str[24];

	PORTD &= set_antenna(epmNum);
	_delay_us(50);
	if (polarity) {
		pot_set(80);
		PORTC &= 0b10010000; // turn on HC and LI
		_delay_us(100);      // wait 100 us
		PORTC &= 0b00000000; // turn off HC and LI

		sprintf(reply_str, " Pulse sent to %c ", epmNum);
		CDC_Device_SendString(&VirtualSerial_CDC_Interface, reply_str);
		
	}
	else if (!polarity) {
		pot_set(30);
		PORTC &= 0b01100000; // turn on LC and HI
		_delay_us(100);      // Wait 100 us
		PORTC &= 0b00000000; // turn off LC and HI
		
		sprintf(reply_str, " Pulse sent to %c ", epmNum);
		CDC_Device_SendString(&VirtualSerial_CDC_Interface, reply_str);
		
	}
	else {
		
		sprintf(reply_str, " Pulse Failed: %c", epmNum);
		CDC_Device_SendString(&VirtualSerial_CDC_Interface, reply_str);
		return 0;
		
	}
	
	return 1;

}


uint16_t set_antenna(uint16_t epmNum){
	if( epmNum < 1 || epmNum > 70 ){
		CDC_Device_SendString(&VirtualSerial_CDC_Interface, " Invalid epmNum ");
	}
	else if (epmNum <= 16){
		PORTD &= 0b00010000;
	}
	else if (epmNum <= 32){
		PORTD &= 0b00100000; 
	}
	else if (epmNum <= 48){
		PORTD &= 0b00110000; 
	}
	else if (epmNum <= 64){
		PORTD &= 0b01000000; 
	}
	else {
		PORTD &= 0b01010000;
	}

	switch((epmNum-1)%16){
		case 0:
			PORTD |= 0b00000000;
			break;
		case 1:
			PORTD |= 0b00000001;
			break;
		case 2:
			PORTD |= 0b00000010;
			break;
		case 3:
			PORTD |= 0b00000011;
			break;
		case 4:
			PORTD |= 0b00000100;
			break;
		case 5:
			PORTD |= 0b00000101;
			break;
		case 6:
			PORTD |= 0b00000110;
			break;
		case 7:
			PORTD |= 0b00000111;
			break;
		case 8:
			PORTD |= 0b00001000;
			break;
		case 9:
			PORTD |= 0b00001001;
			break;
		case 10:
			PORTD |= 0b00001010;
			break;
		case 11:
			PORTD |= 0b00001011;
			break;
		case 12:
			PORTD |= 0b00001100;
			break;
		case 13:
			PORTD |= 0b00001101;
			break;
		case 14:
			PORTD |= 0b00001110;
			break;
		case 15:
			PORTD |= 0b00001111;
			break;
		default:
			PORTD |= 0b00000000;
			break;
	}
	return PORTD;
	
}

int pot_set (uint16_t val) {
	//val is btwn 0 and 256
	uint8_t cmd_error_mask = 0b00000010;
	uint16_t data_mask = 0b0000000111111111;
	uint16_t hi_mask = 0b1111111100000000;
	uint16_t lo_mask = ~hi_mask;
	uint16_t cmd = (((uint16_t)cmd_error_mask) << 8) | (val & data_mask);
	uint8_t cmd_hi = (uint8_t)((hi_mask & cmd) >> 8);
	uint8_t cmd_lo = (uint8_t)(lo_mask & cmd);
	PORTB &= 0b11111110;
	_delay_ms(1);
	SPI_MasterTransmit(cmd_hi);
	SPI_MasterTransmit(cmd_lo);
	PORTB |= 0b00000001;
	return 0;
}
/*
void pot_set (uint16_t v_out) {
	//NOTE: use uint32_t b/c 69800 is large
	uint16_t R6 = 69800;  //69.8kohm
	uint16_t R_AB = 5000; //5kohm
	uint16_t R_W = 75;    //75ohm
	uint16_t R5;
	uint8_t N;
	
	uint8_t cmd = 0b00000000; //write to address 0000

	R5 = R6/(v_out/1.6 - 1); // Calculate resistor value for pot
	N = 256*(R5 - R_W)/R_AB;  // wiper setting

	PORTB &= 0b11111110;  // CS (PB0) low
	SPDR = SPI_TransferByte(cmd);
	SPDR = SPI_TransferByte(N);
	PORTB &= 0b11111111;  // CS (PB0) high

}
*/

void error(void) {
	//Turns on blue LED to indicate error
	PORTB = 0b01000000;
	return;
}

void Blink(void) {
	// Blinks blue LED with specified period
	PORTB |= 0b01000000;
	_delay_ms(1000);
	PORTB &= (~0b01000000);
	return;
	//CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Blink");
	
}


void setupHardware(void) {

	//CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Setting up hardware");

	/* Disable watchdog */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	/* Disable prescaler */
	clock_prescale_set(clock_div_1);
	
	/* Hardware Initialization */
	USB_Init();
	
	/* Start the flush timer so that overflows occur rapidly to push received bytes to the USB interface */
	//TCCR0B = (1 << CS02);
	
	
	DDRD = 0b01111111;
	DDRC = 0b11110000;
	SPI_MasterInit();
	
	//
	
	/* Enable SPI */
	//SPI_Init(SPI_SPEED_FCPU_DIV_128 | SPI_ORDER_MSB_FIRST | SPI_SCK_LEAD_RISING |
	//SPI_SAMPLE_TRAILING | SPI_MODE_MASTER);
	return;
}

void SPI_MasterInit(void) {
	DDRB = 0b01001111;
	SPCR = 0b01010011;
	PORTB |= 0b00000001;
}

void SPI_MasterTransmit(char cData) {
	SPDR = cData;
	while(!(SPSR & (1<<SPIF)))
	;
}




