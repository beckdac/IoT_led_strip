#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <avr/wdt.h>

#include "global.h"
#include "rgb.h"
#include "timer1.h"

volatile uint16_t icp_hz, icp_events;

ISR(TIMER1_CAPT_vect){ 
	++icp_events;
}

ISR(TIMER1_COMPA_vect) {
	icp_hz = icp_events;
	icp_events = 0;
}

void timer1_init() {
	// normal timer counter mode
	TCCR1A = 0;
	// input capture edge select on positive edge
	TCCR1B = (1 << ICES1);
	// clear timer compare mode (CTC)
	TCCR1B |= (1 << CS12);

	// set CTC for 1 Hz
	OCR1A = TIMER1_CTC_TOP;
	// set timer 1 prescale factor to 256
	TCCR1B |= (1 << WGM12);

	timer1_start();
}

void timer1_stop() {
	TIMSK1 &= ~(1 << ICIE1);
}

void timer1_start() {
	icp_hz = 0;
	icp_events = 0;

	// enable timer 1 input capture interrupt and CTC interrupt
	TIMSK1 |= (1 << ICIE1) | (1 << OCIE1A);
}
