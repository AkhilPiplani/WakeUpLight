/*
 * lcd44780_LP.h
 *		Header file for the lcd44780_LP.c
 *		Version 1.00
 *      Author: NYH
 *      Reference: Robot Head to Toe Vol. 11 - pg 35-36
 */
 
#ifndef LCD44780_LP_H_
#define LCD44780_LP_H_

// commands
	#define LCD_CLEARDISPLAY	0x01
	#define LCD_RETURNHOME		0x02
	#define LCD_ENTRYMODESET	0x04
	#define LCD_DISPLAYCONTROL	0x08
	#define LCD_CURSORSHIFT		0x10
	#define LCD_FUNCTIONSET		0x20
	#define LCD_SETCGRAMADDR	0x40
	#define LCD_SETDDRAMADDR	0x80

	// flags for display entry mode
	#define LCD_ENTRYRIGHT		0x00
	#define LCD_ENTRYLEFT		0x02
	#define LCD_ENTRYSHIFTINCREMENT	0x01
	#define LCD_ENTRYSHIFTDECREMENT	0x00

	// flags for display on/off control
	#define LCD_DISPLAYON		0x04
	#define LCD_DISPLAYOFF		0x00
	#define LCD_CURSORON		0x02
	#define LCD_CURSOROFF		0x00
	#define LCD_BLINKON			0x01
	#define LCD_BLINKOFF		0x00

	// flags for display/cursor shift
	#define LCD_DISPLAYMOVE		0x08
	#define LCD_CURSORMOVE		0x00
	#define LCD_MOVERIGHT		0x04
	#define LCD_MOVELEFT		0x00

	// flags for function set
	#define LCD_8BITMODE		0x10
	#define LCD_4BITMODE		0x00
	#define LCD_2LINE			0x08
	#define LCD_1LINE			0x00
	#define LCD_5x10DOTS		0x04
	#define LCD_5x8DOTS			0x00



#define LCD_PORT		GPIO_PORTB_BASE
#define LCD_PORTENABLE	SYSCTL_PERIPH_GPIOB
#define LCD_RS			GPIO_PIN_0
#define LCD_E			GPIO_PIN_1
#define LCD_D4			GPIO_PIN_4
#define LCD_D5			GPIO_PIN_5
#define LCD_D6			GPIO_PIN_6
#define LCD_D7			GPIO_PIN_7

#define LCD_ROW_SIZE	8
#define LCD_NB_ROWS		2

void lcd_init();
void lcd_command(unsigned char);
void lcd_write(unsigned char);
void lcd_writeText(char*,unsigned char, unsigned char);
void lcd_writePos(unsigned char,unsigned char, unsigned char );
void lcd_scrollLeft();
void lcd_scrollRight();
void lcd_build(unsigned char location, unsigned char *ptr);
void lcd_BuildCustomCharacters();
void lcd_writeData(unsigned char inputData);

#endif /* LCD44780_LP_H_ */
