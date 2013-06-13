/*
 * lights.h
 *
 *  Created on: 18-May-2013
 *      Author: Akhil
 */

#ifndef LIGHTS_H_
#define LIGHTS_H_

#include <inc/hw_ints.h>
#include <inc/hw_types.h>

#define LIGHTS_PORT			GPIO_PORTE_BASE
#define LIGHTS_TIMER		TIMER0_BASE
#define LIGHTS_PORTENABLE	SYSCTL_PERIPH_GPIOE
#define LIGHTS_TIMERENABLE	SYSCTL_PERIPH_TIMER0

#define LIGHTS_0CROSSING_IN			GPIO_PIN_1
#define LIGHTS_DIMMER_OUT			GPIO_PIN_2

#define LIGHTS_0CROSSING_INTERRUPT	INT_GPIOE
#define LIGHTS_TIMER_INTERRUPT		INT_TIMER0A

#define LIGHTS_MAX_BRIGHTNESS		(ROM_SysCtlClockGet()/100)

extern unsigned long lights_MaxBrightness;

void lights_init();
void lights_setBrightness(unsigned long brightness);
void lights_printACfrequencyOnLCD();

#endif /* LIGHTS_H_ */
