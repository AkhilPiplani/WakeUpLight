/*
 * alarm.h
 *
 *  Created on: 15-Jul-2013
 *      Author: Akhil
 */

#ifndef ALARM_H_
#define ALARM_H_

#define ALARM_SAMPLERATE_TIMER				TIMER2_BASE
#define ALARM_SAMPLERATE_TIMER_INTERRUPT	INT_TIMER2A
#define ALARM_SAMPLERATE_TIMERENABLE		SYSCTL_PERIPH_TIMER2

#define ALARM_PWM_TIMER						TIMER3_BASE
#define ALARM_PWM_TIMERENABLE				SYSCTL_PERIPH_TIMER3
#define ALARM_PWM_PORTENABLE				SYSCTL_PERIPH_GPIOB
#define ALARM_PWM_PORT						GPIO_PORTB_BASE
#define ALARM_PWM_PIN						GPIO_PIN_2
#define ALARM_PINMAP_PWM					GPIO_PB2_T3CCP0

void alarm_init();
void alarm_stop();

#endif /* ALARM_H_ */
