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
#include <errno.h>
#include <util/delay.h>
#include "Descriptors.h"
#include <LUFA/Drivers/Peripheral/Serial.h>		
#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/USB/USB.h>

	/* Function Prototypes: */
// Antenna control
void usb_rx(void);
int SendPulse(uint8_t polarity, int epmNum);
void set_mux(int x, uint8_t HI, uint8_t LI);
void set_COM(uint8_t HC, uint8_t LC);
// Power control
int pot_set (float voltage);
uint16_t pot_value_calc(float voltage);
// Debug
void error(void);
void Blink(void);

// USB/Serial setup
void setupHardware(void);

void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
//SPI setup
void SPI_MasterInit(void);
void SPI_MasterTransmit(char cData);

#endif
