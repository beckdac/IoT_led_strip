#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "global.h"
#include "program.h"
#include "usart.h"
#include "rgb.h"
#include "compatability.h"

volatile program_state_e program_state;

void program_init(void) {
	uint16_t preamble;

	preamble = eeprom_read_word((const uint16_t *)PROGRAM_PREAMBLE_LOCATION);
	if (preamble != PROGRAM_PREAMBLE) {
		program_setup_default();
		eeprom_update_word((uint16_t *)PROGRAM_PREAMBLE_LOCATION, PROGRAM_PREAMBLE);
	}

	program_state = PROGRAM_RUN;
}

void program_setup_default(void) {
	uint16_t location = PROGRAM_START;
	float frequency = .3;
	uint8_t i, rgb[3];
	uint16_t steps = 43, delay_in_ms = 100;

	program_state = PROGRAM_PROGRAMMING;

	printf_P(PSTR("init eeprom ... "));
	eeprom_update_word((uint16_t *)location, steps);
	location += sizeof(uint16_t);
	for (i = 0; i < steps; ++i) {
   		rgb[0] = (uint8_t)(sin(frequency*(float)i + 0.) * 127.) + 128;
   		rgb[1] = (uint8_t)(sin(frequency*(float)i + 2.) * 127.) + 128;
   		rgb[2] = (uint8_t)(sin(frequency*(float)i + 4.) * 127.) + 128;
		eeprom_update_block(&rgb, (void *)location, 3 * sizeof(uint8_t));
		location += 3 * sizeof(uint8_t);

		eeprom_update_word((uint16_t *)location, delay_in_ms);
		location += sizeof(uint16_t);
	}
	printf_P(PSTR("done!\n"));
	
	program_state = PROGRAM_RUN;
}

#define PROGRAM_RUN_DEBUG
#undef PROGRAM_RUN_DEBUG
void program_run(void) {
	uint16_t location = PROGRAM_START;
	uint16_t i, steps, delay_in_ms;
	uint8_t rgb[3];

	// don't start running a programming if the program is being written by an external source
	// or if we are in program stop mode
	if (program_state != PROGRAM_RUN) goto PROGRAM_RUN_EXIT;

	// after each eeprom read, we check to see if the system has left program run state
	// if so, the eeprom read might be invalid so we need to exit

	steps = eeprom_read_word((uint16_t *)location);
	if (program_state != PROGRAM_RUN) goto PROGRAM_RUN_EXIT;
	location += sizeof(uint16_t);
#ifdef PROGRAM_RUN_DEBUG
	printf_P(PSTR("program has %d steps\n"), steps);
#endif
	for (i = 0; i < steps; ++i) {
		eeprom_read_block(&rgb, (void *)location, 3 * sizeof(uint8_t));
		if (program_state != PROGRAM_RUN) goto PROGRAM_RUN_EXIT;
		location += 3 * sizeof(uint8_t);
#ifdef PROGRAM_RUN_DEBUG
		printf_P(PSTR("step %d:\t%d\t%d\t%d\n"), i, rgb[0], rgb[1], rgb[2]);
#endif	
		rgb_set(rgb[0], rgb[1], rgb[2]);

		delay_in_ms = eeprom_read_word((uint16_t *)location);
		if (program_state != PROGRAM_RUN) goto PROGRAM_RUN_EXIT;
		location += sizeof(uint16_t);
#ifdef PROGRAM_RUN_DEBUG
		printf_P(PSTR("step %d delay:\t%d\n"), i, delay_in_ms);
#endif
		// _delay_ms must be used on a constant (not a variable)
		while(delay_in_ms--) _delay_ms(1);

		if (program_state != PROGRAM_RUN) {
PROGRAM_RUN_EXIT:
#ifdef PROGRAM_RUN_DEBUG
			printf_P(PSTR("system is not in run state, exiting run program\n"));
#endif
			return;
		}
	}
#ifdef PROGRAM_RUN_DEBUG
	printf_P(PSTR("program complete\n"));
#endif
}
