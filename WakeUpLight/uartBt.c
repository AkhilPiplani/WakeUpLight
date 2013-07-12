/*
 * uartBluetooth.c
 *
 *  Created on: 11-Jul-2013
 *      Author: Akhil
 */
#include <stddef.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/rom.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include "uartBt.h"

#define UARTBT_BUFFER_SIZE	32

static volatile unsigned char CommandBuffer[UARTBT_BUFFER_SIZE*2] = {0}; // Double buffer for holding commands
static volatile unsigned char *CurrentCommandStartPointer = CommandBuffer;
static volatile unsigned char *CurrentCommandByte = CommandBuffer;
static volatile unsigned long LastCommandSize = 0;
static tBoolean NewCommandArrived = false;

void ISR_uartBt(void) {
	unsigned long status;
	unsigned long commandSize;
	char byte;

	status = UARTIntStatus(UARTBT_BASE, true);
	UARTIntClear(UARTBT_BASE, status);

	// Loop while there are characters in the receive FIFO.
	while(UARTCharsAvail(UARTBT_BASE)) {
		// Read the next character from the UART and write it back to the UART.
		byte = UARTCharGetNonBlocking(UARTBT_BASE);
		UARTCharPutNonBlocking(UARTBT_BASE, byte); // Comment me later

		commandSize = CurrentCommandByte - CurrentCommandStartPointer;

		if(byte == '\r' || commandSize==UARTBT_BUFFER_SIZE) { // \r is used to indicate end-of-command
			if(*CurrentCommandByte == '\n') {
				LastCommandSize = commandSize - 1;
			}
			else {
				LastCommandSize = commandSize;
			}

			// Select the next buffer
			if(CurrentCommandStartPointer == CommandBuffer) {
				CurrentCommandByte = CommandBuffer + UARTBT_BUFFER_SIZE;
			}
			else {
				CurrentCommandByte = CommandBuffer;
			}

			CurrentCommandStartPointer = CurrentCommandByte;

			if(commandSize != UARTBT_BUFFER_SIZE) { // If the command is too long, ignore it
				NewCommandArrived = true;
			}
		}
		else {
			CurrentCommandByte++;
		}

		// Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
		SysCtlDelay(SysCtlClockGet() / (1000 * 3)); // Comment me later
	}
}

void uartBt_init() {
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_GPIOPinConfigure(UARTBT_PINMAP_RX);
	ROM_GPIOPinConfigure(UARTBT_PINMAP_TX);
	ROM_GPIOPinTypeUART(UARTBT_PORT, UARTBT_PIN_RX | UARTBT_PIN_TX);

	// Configure the UART for 9600, 8-N-1 operation.
	ROM_UARTConfigSetExpClk(UARTBT_BASE, ROM_SysCtlClockGet(), 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	ROM_UARTIntEnable(UARTBT_BASE, UART_INT_RX | UART_INT_RT);
}

void uartBt_send(unsigned char *data, unsigned long size) {
	while(size--) {
		ROM_UARTCharPutNonBlocking(UARTBT_BASE, *data++);
	}
}

tBoolean uartBt_receive(volatile unsigned char **data, unsigned long *size) {
	if(NewCommandArrived == true) {
		*size = LastCommandSize;

		if(CurrentCommandStartPointer == CommandBuffer) {
			*data = CommandBuffer + UARTBT_BUFFER_SIZE;
		}
		else {
			*data = CommandBuffer;
		}

		NewCommandArrived = false;
		return true;
	}
	else {
		*size = 0;
		return false;
	}
}
