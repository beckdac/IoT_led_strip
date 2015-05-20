#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <avr/wdt.h>

#include "global.h"
#include "rgb.h"
#include "timer0.h"

volatile uint8_t timer0_counter;
extern volatile uint8_t rgb[3];

ISR(TIMER0_OVF_vect) {
	if (timer0_counter == rgb[0])
		LED_RED_LOW;
	if (timer0_counter == rgb[1])
		LED_GREEN_LOW;
	if (timer0_counter == rgb[2])
		LED_BLUE_LOW;

	if (timer0_counter == 255) {
		if (rgb[0] != 0)
			LED_RED_HIGH;
		if (rgb[1] != 0)
			LED_GREEN_HIGH;
		if (rgb[2] != 0)
			LED_BLUE_HIGH;
		timer0_counter = 0;
	} else
		timer0_counter++;
}

void timer0_init() {
	timer0_counter = 0;

	rgb_init();

	// set timer 0 prescale factor to 64
	TCCR0B |= (1 << CS00);
	//TCCR0B |= (1 << CS01);

	timer0_start();
}

void timer0_stop() {
	timer0_counter = 0;
	TIMSK0 &= ~(1 << TOIE0);
}

void timer0_start() {
	// enable timer 0 overflow interrupt
	TIMSK0 |= (1 << TOIE0);
}
