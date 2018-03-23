/*
 * main.c
 *
 * Created: 3/23/2017 12:32:50 PM
 * Author : Lou
 */ 

#include "AntCtrl.h"


/** Circular buffer to hold data from the host */
static RingBuffer_t FromHost_Buffer;

/** Underlying data buffer for \ref FromHost_Buffer, where the stored bytes are located. */
static uint8_t      FromHost_Buffer_Data[128];

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

	sei(); /*  Enable interrupts */
	
	
	RingBuffer_InitBuffer(&FromHost_Buffer, FromHost_Buffer_Data, sizeof(FromHost_Buffer_Data));

	GlobalInterruptEnable();
	
	static uint8_t polarity = 0;
	static uint16_t epmNum = 0;

	
	static uint8_t ReceivedCMD = 0;
	for (;;)
	{
		/* Only try to read in bytes from the CDC interface if the buffer is not full */
		if (!(RingBuffer_IsFull(&FromHost_Buffer)))	{
			
			int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);


			/* Read bytes from the USB OUT endpoint into the buffer */
			if (!(ReceivedByte < 0)) {
			  RingBuffer_Insert(&FromHost_Buffer, ReceivedByte);
			}
			else
			  continue;
		}
		
		uint16_t CMD_Chunk = RingBuffer_Remove(&FromHost_Buffer);
		if (CMD_Chunk == 0x0A) {
			continue;
		}
		
		if (ReceivedCMD) {
			epmNum = CMD_Chunk;
			SendPulse(polarity, epmNum);
			ReceivedCMD = 0;
			continue;
		}
		
		if (CMD_Chunk == 0x2B) {
			polarity = 1;
			ReceivedCMD = 1;
		}
		else if (CMD_Chunk == 0x2D) {
			polarity = 0;
			ReceivedCMD = 1;
		}
		else {
			CDC_Device_SendString(&VirtualSerial_CDC_Interface, " Bad Command ");
			ReceivedCMD = 0;
		}
			
			CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
			USB_USBTask();
			
		
	}
}

uint8_t SendPulse(uint8_t polarity, uint16_t epmNum) {
	// Sets up MUX and delivers pulse to H-bridge controller
		
	char reply_str[24];
	
	
	PORTB = epmNum-1;
	_delay_us(50);
	if (polarity) {
		PORTD |= 0b00010000;  // Turn on the correct transistors
		PORTC |= 0b01000000;  
		_delay_us(100);  // Wait 100 us
		PORTD &= 0b11100111; // Turn everything off
		PORTC &= 0b00111111;
		
		sprintf(reply_str, " Pulse sent to %c ", epmNum);
		CDC_Device_SendString(&VirtualSerial_CDC_Interface, reply_str);
		
	}
	else if (!polarity) {
		PORTD |= 0b00001000;  // Turn on the correct transistors
		PORTC |= 0b10000000;
		_delay_us(100);  // Wait 100 us
		PORTD &= 0b11100111; // Turn everything off
		PORTC &= 0b00111111;
		
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

void Blink(void) {
	// Blinks blue LED with specified period
	
	uint8_t i;
	uint16_t num_blinks = 10000;
	
	for (i = 1; num_blinks; i++) {
		PORTD ^= 1<<5;
		_delay_ms(500);
	}
	return;
	CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Blink");
	
}


void setupHardware(void) {


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
	DDRB = 0b00000011;
	
	/* enable SPI */
	SPI_Init(SPI_SPEED_FCPU_DIV_2 | SPI_ORDER_MSB_FIRST | SPI_SCK_LEAD_FALLING |
         SPI_SAMPLE_TRAILING | SPI_MODE_MASTER)
	
	//PORTD &= 0b11100111; // Turn everything off
	//PORTC &= 0b00111111;

	return;
}


/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}



