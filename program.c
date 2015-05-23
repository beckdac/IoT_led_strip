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
#include <string.h>

#include "global.h"
#include "program.h"
#include "usart.h"
#include "rgb.h"
#include "compatability.h"

volatile program_state_e program_state;

extern volatile uint8_t usart_command_available;
extern volatile char usart_command[BUFFER_SIZE];

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

#define program_check_state() if (usart_command_available) program_command_available(); if (program_state != PROGRAM_RUN) goto PROGRAM_RUN_EXIT; 

#define PROGRAM_RUN_DEBUG
#undef PROGRAM_RUN_DEBUG
void program_run(void) {
	uint16_t location = PROGRAM_START;
	uint16_t i, steps, delay_in_ms;
	uint8_t rgb[3];

	// don't start running a programming if the program is being written by an external source
	// or if we are in program stop mode
	program_check_state();

	// after each eeprom read, we check to see if the system has left program run state
	// if so, the eeprom read might be invalid so we need to exit

	steps = eeprom_read_word((uint16_t *)location);
	program_check_state();
	location += sizeof(uint16_t);
#ifdef PROGRAM_RUN_DEBUG
	printf_P(PSTR("program has %d steps\n"), steps);
#endif
	for (i = 0; i < steps; ++i) {
		eeprom_read_block(&rgb, (void *)location, 3 * sizeof(uint8_t));
		program_check_state();
		location += 3 * sizeof(uint8_t);
#ifdef PROGRAM_RUN_DEBUG
		printf_P(PSTR("step %d:\t%d\t%d\t%d\n"), i, rgb[0], rgb[1], rgb[2]);
#endif	
		rgb_set(rgb[0], rgb[1], rgb[2]);

		delay_in_ms = eeprom_read_word((uint16_t *)location);
		program_check_state();
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

void program_write_steps(uint16_t steps) {
	if (program_state == PROGRAM_PROGRAMMING) {
		uint16_t location = PROGRAM_START;
		printf_P(PSTR("program steps = %" PRIu16 "\n"), steps);
		eeprom_update_word((uint16_t *)location, steps);
	} else
		printf_P(PSTR("unable to write steps: not in programming mode\n"));
}

void program_write_step(uint16_t step, uint8_t rgb[3], uint16_t delay_in_ms) {
	uint16_t location = PROGRAM_STEP_LOCATION(step);
	eeprom_update_block(rgb, (void *)location, 3 * sizeof(uint8_t));
	location += 3 * sizeof(uint8_t);
	eeprom_update_word((uint16_t *)location, delay_in_ms);
	printf_P(PSTR("program step  = %" PRIu16 "\n"), step);
	printf_P(PSTR("program rgb   = %u\t%u\t%u\n"), rgb[0], rgb[1], rgb[2]);
	printf_P(PSTR("program delay = %" PRIu16 "\n"), delay_in_ms);
}

#define PROGRAM_COMMAND_STOP "STOP"
#define PROGRAM_COMMAND_STOP_LENGTH 4
#define PROGRAM_COMMAND_PROGRAMMING_MODE "PROGRAM"
#define PROGRAM_COMMAND_PROGRAMMING_MODE_LENGTH 7
#define PROGRAM_COMMAND_RUN "RUN"
#define PROGRAM_COMMAND_RUN_LENGTH 3
#define PROGRAM_COMMAND_LENGTH "LENGTH"
#define PROGRAM_COMMAND_LENGTH_LENGTH 6
#define PROGRAM_COMMAND_STEP "STEP"
#define PROGRAM_COMMAND_STEP_LENGTH 4
#define PROGRAM_COMMAND_RGB "RGB"
#define PROGRAM_COMMAND_RGB_LENGTH 3
#define PROGRAM_COMMAND_OFF "OFF"
#define PROGRAM_COMMAND_OFF_LENGTH 3
//#define PROGRAM_COMMAND_
//#define PROGRAM_COMMAND__LENGTH

void program_command_available(void) {
	if (strncmp(usart_command, PROGRAM_COMMAND_STOP, PROGRAM_COMMAND_STOP_LENGTH) == 0) {
		program_state = PROGRAM_STOP;
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_PROGRAMMING_MODE, PROGRAM_COMMAND_PROGRAMMING_MODE_LENGTH) == 0) {
		program_state = PROGRAM_PROGRAMMING;
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_RUN, PROGRAM_COMMAND_RUN_LENGTH) == 0) {
		program_state = PROGRAM_RUN;
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_LENGTH, PROGRAM_COMMAND_LENGTH_LENGTH) == 0) {
		if (program_state == PROGRAM_PROGRAMMING) {
			printf_P(PSTR("OK\n"));
		} else {
			printf_P(PSTR("not in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_STEP, PROGRAM_COMMAND_STEP_LENGTH) == 0) {
		if (program_state == PROGRAM_PROGRAMMING) {
			printf_P(PSTR("OK\n"));
		} else {
			printf_P(PSTR("not in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_RGB, PROGRAM_COMMAND_RGB_LENGTH) == 0) {
		if (program_state == PROGRAM_STOP) {
			printf_P(PSTR("OK\n"));
		} else {
			printf_P(PSTR("currently running program!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_OFF, PROGRAM_COMMAND_OFF_LENGTH) == 0) {
		if (program_state != PROGRAM_PROGRAMMING) {
			rgb_set(0, 0, 0);
			printf_P(PSTR("OK\n"));
		} else {
		printf_P(PSTR("currently in programming mode!\nERROR\n"));
		}
	} else if (isspace(usart_command)) {
		// noop
	} else {
		printf_P(PSTR("unrecognized command\nERROR\n"));
	}
// } else if (strncmp(usart_command, PROGRAM_COMMAND_, PROGRAM_COMMAND__LENGTH) == 0) {

	usart_command_available = 0;
}
