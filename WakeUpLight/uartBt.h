/*
 * uartBt.h
 *
 *  Created on: 11-Jul-2013
 *      Author: Akhil
 */

#ifndef UARTBT_H_
#define UARTBT_H_

#include <inc/hw_types.h>
#include <driverlib/pin_map.h>

#define UARTBT_BASE			UART2_BASE
#define UARTBT_PORTENABLE	SYSCTL_PERIPH_GPIOD
#define UARTBT_PERIPHENABLE	SYSCTL_PERIPH_UART2
#define UARTBT_INTERRUPT	INT_UART2
#define UARTBT_PORT			GPIO_PORTD_BASE

#define UARTBT_PINMAP_RX	GPIO_PD6_U2RX
#define UARTBT_PINMAP_TX	GPIO_PD7_U2TX

#define	UARTBT_PIN_RX		GPIO_PIN_6
#define	UARTBT_PIN_TX		GPIO_PIN_7

#define UARTBT_MAX_COMMAND_SIZE		32

void uartBt_init(unsigned long baudrate);
void uartBt_send(unsigned char *data, unsigned long size);
unsigned long uartBt_receive(unsigned char *data);

void uartBt_oneTimeSetup();

#endif /* UARTBT_H_ */
