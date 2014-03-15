WakeUpLight
===========
A wake up light using a table lamp and a Stellaris Launchpad.
The project is described here: http://beagaroo.wordpress.com

The project can be compiled in eclipse CDT with the  GNU ARM plugin (http://sourceforge.net/projects/gnuarmeclipse/) installed along with the GCC ARM Embedded compiler toolchain (https://launchpad.net/gcc-arm-embedded) . 

lights: Controls an AC dimmer circuit to set the brightness of a lamp attached to it.

sound: Uses PWM to play back a sound stored in a header file.

time: Used to get and set time form the RTC of the microcontroller and set alarms.

uartBt: Uart connection to the HC-05 module. The incoming commands are expected to end with a newline and carriage return "\n\r".

buttons: Functions to poll the snooze / alarm-off button.

lcd44780_LP: Controls an LCD over GPIO. Not used in the project at the end.

syscalls: Lowest layer to newlib for printf and malloc etc.

LM4F.ld and LM4F_startup.c: Linker script to tell the GCC linker what goes where and startup code that runs before main.


