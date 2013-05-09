/*
 *       lcd44780_LP.c
 *		Basic HLCD_D44780 driver for Stellaris Launchpad
 *      Version 1.00
 *      Author: NYH
 *      Reference: Robot Head to Toe Vol. 11 - pg 35-36
 *      Note: One full port must be used for this LCD. In this driver PORTB is used.
 */

//Modified by Don Le
//Added function to create custom character

// Modified by Akhil Piplani
// LCD_writeText now automatically moves on to the next row when the text doesn't in the first one.
// Using a simple lookup table for Row-Addresses instead of a switch-case as this is more modular.
// Several other small modifications


#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "lcd44780_LP.h"
#include "commons.h"

#if LCD_NB_ROWS==2
	static unsigned char RowAddresses[LCD_NB_ROWS] = {0x80, 0xC0};
#elif LCD_NB_ROWS==4
	static unsigned char RowAddresses[LCD_NB_ROWS] = {0x80, 0xC0, 0x94, 0xD4};
#else
	#error "Unexpected LCD_NB_ROWS, please modify RowAddresses in lcd44780_LP.c"
#endif

void LCD_init() {
	ROM_SysCtlPeripheralEnable(LCD_PORTENABLE);
	ROM_GPIOPinTypeGPIOOutput(LCD_PORT, 0xFF);

	// Please refer to the HLCD_D44780 datasheet for how these initializations work!
	ROM_SysCtlDelay((500e-3)*ROM_SysCtlClockGet()/3);

	ROM_GPIOPinWrite(LCD_PORT, LCD_RS,  0x00);

	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7,  0x30);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3); ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);

	ROM_SysCtlDelay((50e-3)*ROM_SysCtlClockGet()/3);

	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7,  0x30);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3);ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);

	ROM_SysCtlDelay((50e-3)*ROM_SysCtlClockGet()/3);

	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7,  0x30);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3); ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);

	ROM_SysCtlDelay((10e-3)*ROM_SysCtlClockGet()/3);

	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7,  0x20);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3); ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);

	ROM_SysCtlDelay((10e-3)*ROM_SysCtlClockGet()/3);

	LCD_command(LCD_CLEARDISPLAY);	// Clear the screen.
	LCD_command(0x06);	// Cursor moves right.
	LCD_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);	// Don't show any cursor, turn on LCD.
}

void LCD_command(unsigned char command) {
	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7, (command & 0xf0) );
	ROM_GPIOPinWrite(LCD_PORT, LCD_RS, 0x00);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3); ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);

	ROM_SysCtlDelay((100e-6)*ROM_SysCtlClockGet()/3);

	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7, (command & 0x0f) << 4 );
	ROM_GPIOPinWrite(LCD_PORT, LCD_RS, 0x00);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3); ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);

	ROM_SysCtlDelay((5e-3)*ROM_SysCtlClockGet()/3);
}

void LCD_write(unsigned char inputData) {
	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7, (inputData & 0xf0) );
	ROM_GPIOPinWrite(LCD_PORT, LCD_RS, 0x01);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);
	ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);
	ROM_SysCtlDelay((100e-6)*ROM_SysCtlClockGet()/3);

	ROM_GPIOPinWrite(LCD_PORT, LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7, (inputData & 0x0f) << 4 );
	ROM_GPIOPinWrite(LCD_PORT, LCD_RS, 0x01);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x02);
	ROM_SysCtlDelay((20e-6)*ROM_SysCtlClockGet()/3);
	ROM_GPIOPinWrite(LCD_PORT, LCD_E, 0x00);
	ROM_SysCtlDelay((5e-3)*ROM_SysCtlClockGet()/3);
}

void LCD_writeText(char* inputText,unsigned char row, unsigned char col) {
	unsigned int currentColumn = col;

	if(row >= LCD_NB_ROWS) {
		row = 0;
	}
	LCD_command(RowAddresses[row] + col);

	while(*inputText) {
		currentColumn++;
		LCD_write(*inputText++);

		if(currentColumn >= LCD_ROW_SIZE) {
			currentColumn = 0;
			row++;
			if(row >= LCD_NB_ROWS) {
				row = 0;
			}
			LCD_command(RowAddresses[row] + col);
			LCD_write(*inputText++);
		}
	}
}

void LCD_BuildCustomCharacters() {
	unsigned char pattern1[8] ={0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1f};
	unsigned char pattern2[8] ={0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x1f};
	unsigned char pattern3[8] ={0x0, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x01f, 0x1f};
	unsigned char pattern4[8] ={0x0, 0x0, 0x0, 0x0, 0x0, 0x1f, 0x1f, 0x1f};
	unsigned char pattern5[8] ={0x0, 0x0, 0x0, 0x0, 0x1f, 0x1f, 0x1f, 0x1f};
	unsigned char pattern6[8] ={0x0, 0x0, 0x0, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f};
	unsigned char pattern7[8] ={0x0, 0x0, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f};
	unsigned char pattern8[8] ={0x0, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f};

	LCD_build(0, pattern1);
	LCD_build(1, pattern2);
	LCD_build(2, pattern3);
	LCD_build(3, pattern4);
	LCD_build(4, pattern5);
	LCD_build(5, pattern6);
	LCD_build(6, pattern7);
	LCD_build(7, pattern8);
}

void LCD_build(unsigned char location, unsigned char *ptr) {
	unsigned char i;
	if(location<8){
		LCD_command(0x40 + (location*8));
		for(i=0;i<8;i++)
			LCD_write(ptr[i]);
		LCD_command(0x80);
	}

}

void LCD_writePos(unsigned char inputData, unsigned char row, unsigned char col) {
	if(row >= LCD_NB_ROWS) {
		row = 0;
	}
	LCD_command(RowAddresses[row] + col);

	LCD_write(inputData);
}

void LCD_scrollLeft() {
	LCD_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LCD_scrollRight() {
	LCD_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}
