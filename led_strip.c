#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usart.h"
#include "compatability.h"
#include "timer0.h"
#include "rgb.h"
#include "program.h"
#include "timer1.h"

extern volatile program_state_e program_state;
extern volatile uint16_t icp_hz;

int main(void) {
	// disable the watchdog timer, this is necessary if the timer was used to reset the device
	MCUSR &= ~(1<<WDRF);
	wdt_disable();

	usart_init(BAUD_TO_UBRR(USART_BAUD));
	printf_P(PSTR("hello!\n"));

	// led program execution
	timer0_init();
	program_init();
	rgb_set(0, 0, 0);

	// TSL230R light to frequency chip
	timer1_init();

	// turn on interrupts
	sei();

	while (1) {
		program_run();
	}
}
