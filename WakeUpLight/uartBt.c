/*
 * uartBluetooth.c
 *
 *  Created on: 11-Jul-2013
 *      Author: Akhil
 */
#include <stddef.h>
#include <string.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/rom.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/interrupt.h>
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

		commandSize = CurrentCommandByte - CurrentCommandStartPointer;

		if(byte == '\n' || commandSize==UARTBT_BUFFER_SIZE) { // \r is used to indicate end-of-command
			if((CurrentCommandByte>CommandBuffer || CurrentCommandByte>(CommandBuffer + UARTBT_BUFFER_SIZE)) && *(CurrentCommandByte-1)=='\r') {
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
			*CurrentCommandByte = byte;
			CurrentCommandByte++;
		}

	}
}

void uartBt_init() {
	ROM_SysCtlPeripheralEnable(UARTBT_PORTENABLE);
	ROM_SysCtlPeripheralEnable(UARTBT_PERIPHENABLE);

	if(UARTBT_PORT == GPIO_PORTD_BASE) {
		// Enable port PD7 for UART2-TX by opening the lock and selecting the bits we want to modify in the GPIO commit register.
		HWREG(UARTBT_PORT + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
		HWREG(UARTBT_PORT + GPIO_O_CR) = 0x80;
	}

	ROM_GPIOPinConfigure(UARTBT_PINMAP_TX);
	ROM_GPIOPinConfigure(UARTBT_PINMAP_RX);

	ROM_GPIOPinTypeUART(UARTBT_PORT, UARTBT_PIN_RX | UARTBT_PIN_TX);

	// Configure the UART for 9600, 8-N-1 operation.
	ROM_UARTConfigSetExpClk(UARTBT_BASE, ROM_SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	ROM_UARTFIFODisable(UARTBT_BASE); // FIFO disabled so that short commands come through immediately
//	ROM_UARTFIFOEnable(UARTBT_BASE);
//	ROM_UARTFIFOLevelSet(UARTBT_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);
	ROM_IntEnable(UARTBT_INTERRUPT);
	ROM_UARTIntEnable(UARTBT_BASE, UART_INT_RX);
}

void uartBt_send(unsigned char *data, unsigned long size) {
	while(size--) {
		ROM_UARTCharPut(UARTBT_BASE, *data++);
	}
}

unsigned long uartBt_receive(unsigned char *data) {
	if(NewCommandArrived == true) {
		if(CurrentCommandStartPointer == CommandBuffer) {
			memcpy(data, (const void*)(CommandBuffer + UARTBT_BUFFER_SIZE), LastCommandSize);
		}
		else {
			memcpy(data, (const void*)CommandBuffer, LastCommandSize);
		}

		NewCommandArrived = false;
		return LastCommandSize;
	}
	else {
		return 0;
	}
}

static void sendCommandToBC05(char *command) {
	uartBt_send((unsigned char*)command, (unsigned long)strlen(command));
}

void uartBt_oneTimeSetup() {
	// Requires BC-05 to be in AT mode by setting the key pin high.
	char getVersion[] = "AT+VERSION?\r\n";
	char getName[] = "AT+NAME?";
	char setName[] = "AT+NAME=WakeUp Light\r\n";
	char getPassword[] = "AT+PSWD?\r\n";
	char setPassword[] = "AT+PSWD=SleepWell\r\n"; // Think again if you think this is my real password. It's not a bad one though.
	char setBaud[] = "AT+UART=115200,0,0\r\n";

	sendCommandToBC05(getVersion);

	sendCommandToBC05(getName);
	sendCommandToBC05(setName);
	sendCommandToBC05(setName); // For some reason, the datasheet's examples do this twice too. Just to be sure I guess.
	sendCommandToBC05(getName);

	sendCommandToBC05(getPassword);
	sendCommandToBC05(setPassword);
	sendCommandToBC05(setPassword);
	sendCommandToBC05(getPassword);

	sendCommandToBC05(setBaud); // You should change your baudrate in the code after this.
}
