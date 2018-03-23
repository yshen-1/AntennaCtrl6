#ifndef _ANTCTRL_H
#define _ANTCTRL_H
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>
#include "Descriptors.h"		
#include <LUFA/Version.h>
#include <LUFA/Drivers/Peripheral/Serial.h>		
#include <LUFA/Drivers/Misc/RingBuffer.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>
// #include <LUFA/Drivers/Peripheral/SPI.h>

	/* Function Prototypes: */
// Antenna control
uint8_t SendPulse(uint8_t polarity, uint16_t epmNum);
uint16_t set_antenna(uint16_t epmNum);

// Power control
int pot_set (uint16_t val);

// Debug
void error(void);
void Blink(void);

// USB/Serial setup
void setupHardware(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

//SPI setup
void SPI_MasterInit(void);
void SPI_MasterTransmit(char cData);

#endif
