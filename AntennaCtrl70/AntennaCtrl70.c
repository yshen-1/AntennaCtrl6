/*
 * AntennaCtrl70.c
 * 
 * This file can be compiled into a .hex file with
 * the included Makefile and flashed onto the microcontroller
 * of the 70 element board to allow USB control via the included
 * Python script control_test.py
 *
 * Created: 3/23/2017 12:32:50 PM
 * Contributors : Lou, Yuyi Shen, Bonny Chen
 */ 

#include "AntCtrl.h"

/* Bitwise arithmetic macros */
#define bit_set(p,m) ((p) |= (1 << m))
#define bit_clear(p,m) ((p) &= ~(1 << m))

/* USB communications global variables */
volatile int usb_connected = 0;
static FILE USBSerialStream;

/* 
 *  LUFA CDC Class driver interface configuration and state information. This structure is
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

/*
 * usb_rx: Attempts to read a command sent by control_test.py from
 * the file stream associated with the USB connector.
 */
void usb_rx(void)
{
	char buf[10];
	char numbuf[10];
	char returnbuf[10];
	int i, flag;
	char *errvar;
	int pulseerror = 0;
	errvar = fgets(buf, 10, &USBSerialStream);
	// If fgets doesn't return NULL, there's a command to be read.
	if (errvar != NULL) {
		if ((buf[0] != '+') && (buf[0] != '-')) {
			fputs("Commands must start with + or -.", &USBSerialStream);
			return;
		}
		// Attempt to extract the EPM number from the command.
		i = 1;
		flag = 0;
		while (i < 10) {
			if (buf[i] == '\\') {
				flag = 1;
			}
			if (!flag) {
				numbuf[i-1] = buf[i];
			}
			i++;
		}
		errno = 0;
		long epmNumber = strtol(numbuf, NULL, 10);
		if ((errno != 0) || epmNumber == 0) {
			// If parsing the EPM number from the
			// command fails, simply return.
			fputs("Bad command", &USBSerialStream);
			return;
		}
		if (buf[0] == '+') {
			// Send a positive pulse to the indicated EPM
			pulseerror = SendPulse(1, (int)(epmNumber-1));
			if (pulseerror < 0) {
				fputs("Pulse failed", &USBSerialStream);
			} else {
				sprintf(returnbuf, "%d", (int)epmNumber);
				fputs("Positive Pulse sent to ", &USBSerialStream);
				fputs(returnbuf, &USBSerialStream);
			}
		} else if (buf[0] == '-') {
			// Send a negative pulse to the indicated EPM
			pulseerror = SendPulse(0, (int)(epmNumber-1));
			if (pulseerror < 0) {
				fputs("Pulse failed", &USBSerialStream);
			} else {
				sprintf(returnbuf, "%d", (int)epmNumber);
				fputs("Negative Pulse sent to ", &USBSerialStream);
				fputs(returnbuf, &USBSerialStream);
			}
		}
		
	}
}

int main(void)
{

	setupHardware();
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
	sei(); /*  Enable interrupts */
	GlobalInterruptEnable();
	while(1){
		if (usb_connected) {
			/*
			 * If the USB is connected, handle any incoming
			 * commands.
			 */
			usb_rx();
			CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
			USB_USBTask();
		}
	}
	
	return -1;
}

/*
 * SendPulse: Send a 100 usec pulse to the indicated EPM.
 * The pulse is positive if polarity is 1, and negative if
 * polarity is 0. Note that epmNum is indexed from 0 in this
 * function. For example, to send a positive pulse to the EPM socket
 * labeled 1 on the physical board, call SendPulse(1, 0).
 */
int SendPulse(uint8_t polarity, int epmNum)
//EPMS are indexed from 0! 
{
	if (epmNum < 0 || epmNum > 69)
		return -1;
	if (polarity) {
		/*
		Send a positive pulse to EPMx:
		1. Turn off the common MOSFETs
		and the mux-controlled MOSFETs 
		corresponding to the desired EPM.
		2. Turn on the high-side common MOSFET
		while keeping the low-side common MOSFET
		off.
		3. Turn on the low-side mux-controlled
		MOSFET corresponding to the desired EPM
		to trigger the pulse.
		4. Wait 100 usec.
		5. Cut off the pulse by turning off both
		common MOSFETs.
		6. Clean up by turning off both mux-controlled
		MOSFETs corresponding to the desired EPM.
		*/
		set_mux(epmNum, 0, 0);
		set_COM(0, 0);
		_delay_ms(1); // Zero out the FETs
		set_COM(1, 0);
		set_mux(0, 0, 1);
		_delay_us(100);
		set_COM(0, 0); // Cut off pulse
		set_mux(epmNum, 0, 0);
		_delay_ms(1);
		return 1;
	} else {
		/*
		Send a negative pulse to EPMx:
		1. Turn off the common MOSFETs
		and the mux-controlled MOSFETs 
		corresponding to the desired EPM.
		2. Turn on the high-side mux-controlled MOSFET
		corresponding to the desired EPM, while keeping
		the low-side MOSFET off.
		3. Trigger the pulse by turning on the low-side
		common MOSFET, while keeping the high-side MOSFET off.
		4. Cut off the pulse by turning off both mux-controlled
		MOSFETs corresponding to the targeted EPM.
		5. Clean up by turning off both common MOSFETs.
		*/
		set_mux(epmNum, 0, 0);
		set_COM(0, 0); //Zero out FETs
		_delay_ms(1);
		set_mux(epmNum, 1, 0);
		set_COM(0, 1);
		_delay_us(100);
		//Turn off
		set_mux(epmNum, 0, 0);
		set_COM(0, 0);
		_delay_ms(1);
		return 1;
	}


}

/*
 * set_mux(x, HI, LI): This function allows the caller
 * to control the states of the mux-controlled MOSFETs
 * associated with EPMx, where x is the number of an EPM.
 * 
 * set_mux() indexes EPMs from 0!!! For example, to control 
 * the EPM labeled 1 on the physical 70 element board, set_mux()
 * must be called with x = 0.
 * 
 * If HI is 0, set_mux() turns off the high-side MOSFET.
 * Similarly, if HI is 1, set_mux() turns on the high-side MOSFET.
 * 
 * If LI is 0, set_mux() turns off the low-side MOSFET.
 * Similarly, if LI is 1, set_mux() turns on the low-side MOSFET.
 * 
 * Note: Calling this function does not affect the common MOSFETs
 * in any way!
 */
void set_mux(int x, uint8_t HI, uint8_t LI)
// 0 <= x <= 69
// LI and HI are either 0 or 1.
{
	/* 
	Connect to the correct EPM by writing the right address
	to the multiplexer inputs!
	*/
	//0 <= CD_mux_num <= 4
	uint8_t CD_mux_num = (uint8_t)(x/16);
	//0 <= mux_io_num <= 15	
	uint8_t mux_io_num = (uint8_t)(x%16);
	//Set ISL COM on L and H to 0 initially.
	PORTC &= 0b00111111; // Set PORTC values to 0 for safety!
	//Write to PORTD and connect LIx and HIx to
	//ISL_COM_L and ISL_COM_H
	uint8_t ISL_addr_mask = 0b00000111;
	uint8_t CD_addr_mask = 0b00001111;
	uint8_t portd_val = (mux_io_num & CD_addr_mask) | ((CD_mux_num & ISL_addr_mask) << 4);
	PORTD = (PORTD & 0b10000000) | (portd_val & 0b01111111);

	/*
	Write to the mux-controlled MOSFET gate driver inputs!
	The high-side MOSFET is controlled by the value of PORTC6
	and the low-side MOSFET is controlled by PORTC7.
	*/
	if (HI && LI) {
		// Never turn on both MOSFETs on one side
		// of the H bridge!
		return; //BAD!!!
	} else if ((!HI) && LI) {
		// HI is 0 and LI is 1.
		// Turn off the upper MOSFET
		// and turn on the lower MOSFET.
		bit_clear(PORTC, PORTC6);
		_delay_us(2);
		bit_set(PORTC, PORTC7);
	} else if (HI && (!LI)) {
		// HI is 1 and LI is 0
		/*
		Turn off the lower MOSFET.
		Then, turn on the upper MOSFET.
		*/
		bit_clear(PORTC, PORTC7);
		_delay_us(2);
		bit_set(PORTC, PORTC6);
	} else {
		// HI and LI are both 0
		// Turn off both MOSFETs.
		bit_clear(PORTC, PORTC7); // Turn off the lower MOSFET first.
		_delay_us(5);
		bit_clear(PORTC, PORTC6);
	}
	return;	
}

/*
 * set_COM(HC, LC): Calling this function allows
 * the user to control the states of the common MOSFETs.
 * If HC is 1, set_COM() turns on the high-side common MOSFET.
 * Similarly, if HC is 0, set_COM() turns off the high-side
 * common MOSFET.
 * If LC is 1, set_COM() turns on the low-side common MOSFET.
 * Similarly, if LC is 0, set_COM() turns off the low-side common
 * MOSFET.
 */
void set_COM(uint8_t HC, uint8_t LC) 
{
	if (HC && LC) {
		// HC and LC are both 1
		// Never turn on both MOSFETs on one
		// side of the H-bridge simultaneously!
		return; //This is bad!
	} else if ((!HC) && LC) {
		// LC is 1, HC is 0
		// First turn off the high-side MOSFET.
		bit_clear(PORTC, PORTC4);
		_delay_us(2); // deadtime
		// Turn on the low-side MOSFET.
		bit_set(PORTC, PORTC5);
		return;
	} else if ((!LC) && HC) {
		// LC is 0, HC is 1
		// First turn off the low-side MOSFET.
		bit_clear(PORTC, PORTC5);
		_delay_us(2); // deadtime
		// Turn on the high-side MOSFET.
		bit_set(PORTC, PORTC4);
		return;
	} else {
		// LC and HC are both 0
		// First turn off the low-side MOSFET.
		bit_clear(PORTC, PORTC5);
		_delay_us(5); // deadtime
		// Turn off the high-side MOSFET.
		bit_clear(PORTC, PORTC4);
		return;
	}
	return;
}

/*
 * error(): Call this function to turn on the 70 element
 * board's blue LED. Useful for debugging.
 */
void error(void) 
{
	PORTB = 0b01000000;
	return;
}

/*
 * Blink(): Blinks the 70 element board's blue LED for a
 * given period of time. Useful for debugging.
 */
void Blink(void) 
{
	PORTB |= 0b01000000;
	// Keep the LED on for this amount of time.
	_delay_ms(300);
	// Turn off the LED.
	PORTB &= (~0b01000000);
	return;
}

/*
 * pot_value_calc(voltage): Calculates the value
 * that must be written to the 70 element board's
 * potentiometer to ensure that the onboard power
 * supply outputs a voltage close to the value passed
 * in as the argument.
 */
uint16_t pot_value_calc(float voltage) 
{
	float R2 = 67000.0;
	// 2.4 and 1.0 are arbitrary constants. Edit them
	// as needed to maximize power supply output accuracy.
	float R1 = R2/(((voltage + 2.4)/1.5) - 1.0);
	uint16_t dig_pot = (uint16_t)(255.0*R1/(5000.0));
	if (dig_pot > 255) {
		dig_pot = 255;
	} else if (dig_pot < 0) {
		dig_pot = 0;
	}
	return (255 - dig_pot);
	
}

/*
 * pot_set(voltage): Attempts to set the output of the 70
 * element board's onboard power supply to the given voltage.
 * The argument voltage must be between 25.0 and 85.0 for safety
 * purposes.
 */
int pot_set (float voltage) 
{
	uint16_t val = pot_value_calc(voltage);
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

void setupHardware(void) 
{
	/* Disable watchdog */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	/* Disable prescaler */
	clock_prescale_set(clock_div_1);
	
	/* Hardware Initialization */
	USB_Init();	
	DDRD = 0b01111111;
	DDRC = 0b11110000;
	SPI_MasterInit();
	return;
}

// Initializes SPI communications
void SPI_MasterInit(void) 
{
	DDRB = 0b01001111;
	SPCR = 0b01010011;
	PORTB |= 0b00000001;
}

// Transmits the given byte cData via the microcontroller's
// SPI interface.
void SPI_MasterTransmit(char cData) 
{
	SPDR = cData;
	while(!(SPSR & (1<<SPIF)))
	;
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
    usb_connected = 1;
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    usb_connected = 0;
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