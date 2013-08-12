/*
 * sound.h
 *
 *  Created on: 15-Jul-2013
 *      Author: Akhil
 */

#ifndef SOUND_H_
#define SOUND_H_

#define SOUND_SAMPLERATE_TIMER				TIMER2_BASE
#define SOUND_SAMPLERATE_TIMER_INTERRUPT	INT_TIMER2A
#define SOUND_SAMPLERATE_TIMERENABLE		SYSCTL_PERIPH_TIMER2

#define SOUND_PWM_TIMER						TIMER3_BASE
#define SOUND_PWM_TIMERENABLE				SYSCTL_PERIPH_TIMER3
#define SOUND_PWM_PORTENABLE				SYSCTL_PERIPH_GPIOB
#define SOUND_PWM_PORT						GPIO_PORTB_BASE
#define SOUND_PWM_PIN						GPIO_PIN_2
#define SOUND_PINMAP_PWM					GPIO_PB2_T3CCP0

#define SOUND_TIMERS_SYNC					(TIMER_2A_SYNC | TIMER_3A_SYNC)

void sound_init();
void sound_stop();

#endif /* SOUND_H_ */
