/*
 * ui.c
 *
 *  Created on: 18-May-2013
 *      Author: Akhil
 */

/*
 * Main menu:
 * enter=lights-on, up=brighter, down=darker
 * left/right = time/alarm
 * Set-Time: up/down=inc/dec, right=next
 */

#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include "lcd44780_LP.h"
#include "buttons.h"
#include "lights.h"
#include "time.h"
#include "ui.h"

typedef enum _UIstates {
	time_uistate = 0,
	menu_uistate = 1,
	timeSet_uistate = 2,
	alarmSet_uistate = 3,
	lights_uistate = 4
} UIstates;

UIstates UIstate = time_uistate;

typedef enum _MenuStates {
	time_menustate = 0,
	alarm_menustate = 1
} MenuStates;
MenuStates MainMenuState = time_menustate;

char *MainMenuStrings[] = {"Time", "Alarm"};

tBoolean ButtonStates[BUTTONS_NB_BUTTONS];

void ui_init() {
	lcd_init();
    if(buttons_init(ButtonStates) != 0) {
    	while(1);
    }

    ui_run();
}

void ui_run() {
	buttons_poll(ButtonStates);

	switch(UIstate) {
	case time_uistate:
		if(ButtonStates[enter_button] != 0) {
			UIstate = lights_uistate;
			return;
		}
		else if(ButtonStates[right_button]!=0 || ButtonStates[left_button]!=0) {
			UIstate = menu_uistate;
			return;
		}
		else {
			time_printCurrentOnLCD();
			break;
		}

	case menu_uistate:
		if(ButtonStates[right_button] != 0) {
			MainMenuState = alarm_menustate;
		}
		else if(ButtonStates[left_button] != 0) {
			MainMenuState = time_menustate;
		}
		else if(ButtonStates[enter_button] != 0) {
			UIstate = timeSet_uistate + MainMenuState;
			MainMenuState = time_menustate;
			break;
		}
		lcd_writeText(MainMenuStrings[MainMenuState], 0, 1); // Does this blank previous characters I'm not overwriting?
		break;

	case timeSet_uistate:
		break;

	case alarmSet_uistate:
		break;

	case lights_uistate:
		break;

	default:
		break;
	}

	buttons_poll(ButtonStates);
}
