#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"
#include "lcd44780_LP.h"
#include "commons.h"

char PrintString[64] = {0};

unsigned char FillLed(unsigned int h,unsigned int j);

int main(void) {
	unsigned int i = 0;

	ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 400MHz / 2 / 4 (SYSCTL_SYSDIV_4) = 50MHz

	//ROM_IntMasterEnable();

    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf(" Test Print \n");

	LCD_init();

	/*LCD_command(0x80);
	LCD_buildCustomCharacters();

	LCD_write(0);
	LCD_write(1);
	LCD_write(2);
	LCD_write(3);
	LCD_write(4);
	LCD_write(5);
	LCD_write(6);
	LCD_write(7);*/

	LCD_writeText("Hello world!", 0, 0);

	ROM_SysCtlDelay(ROM_SysCtlClockGet() / 10/ 2 );

	while(1) {
		sprintf(PrintString, "%u", i);
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 10/ 2 );
		LCD_writeText(PrintString, 0, 0);
		i++;
	}

}





unsigned char FillLed(unsigned int h,unsigned int j)
{
	if (h > 7)
	{

		if (j == 0)
		{
         	//some math here
			//get the different
			return h - 8;

		}
		else
		{
			return 7;  //max on second row
		}
	}
	else
	{
		if (j == 0)
		{
			return ' ';   //leave row 0 blank, not power enough
		}
		else
		{

			return h;  //whatever the power of the led
		}

	}


}



