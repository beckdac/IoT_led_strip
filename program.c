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

extern volatile uint16_t icp_hz, icp_events;
extern usart_t usart;

uint16_t program_icp_hz_enable;

void program_init(void) {
	uint16_t preamble;

	preamble = eeprom_read_word((const uint16_t *)PROGRAM_PREAMBLE_LOCATION);
	if (preamble != PROGRAM_PREAMBLE) {
		eeprom_update_word((uint16_t *)PROGRAM_PREAMBLE_LOCATION, PROGRAM_PREAMBLE);
		eeprom_update_word((uint16_t *)PROGRAM_ICP_HZ_ENABLE_LOCATION, PROGRAM_DEFAULT_ICP_HZ_ENABLE);
		program_setup_default();
	}
	program_icp_hz_enable = eeprom_read_word((const uint16_t *)PROGRAM_ICP_HZ_ENABLE_LOCATION);

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

#define program_check_state() if (usart_command_available) if (program_process_command_and_invalidate()) goto PROGRAM_RUN_EXIT; if (program_state != PROGRAM_RUN) goto PROGRAM_RUN_EXIT; if (icp_hz > program_icp_hz_enable) { rgb_set(0, 0, 0); goto PROGRAM_RUN_EXIT; }

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

#define PROGRAM_COMMAND_RESET "RESET"
#define PROGRAM_COMMAND_RESET_LENGTH 5
#define PROGRAM_COMMAND_FACTORY "FACTORY"
#define PROGRAM_COMMAND_FACTORY_LENGTH 7
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
#define PROGRAM_COMMAND_RED "RED"
#define PROGRAM_COMMAND_RED_LENGTH 3
#define PROGRAM_COMMAND_GREEN "GREEN"
#define PROGRAM_COMMAND_GREEN_LENGTH 5
#define PROGRAM_COMMAND_BLUE "BLUE"
#define PROGRAM_COMMAND_BLUE_LENGTH 4
#define PROGRAM_COMMAND_OFF "OFF"
#define PROGRAM_COMMAND_OFF_LENGTH 3
#define PROGRAM_COMMAND_LHZ "LHZ"
#define PROGRAM_COMMAND_LHZ_LENGTH 3
#define PROGRAM_COMMAND_LHZEN "LHZEN"
#define PROGRAM_COMMAND_LHZEN_LENGTH 5
#define PROGRAM_COMMAND_DUMP "DUMP"
#define PROGRAM_COMMAND_DUMP_LENGTH 4
#define PROGRAM_COMMAND_STATUS "STATUS"
#define PROGRAM_COMMAND_STATUS_LENGTH 6
//#define PROGRAM_COMMAND_
//#define PROGRAM_COMMAND__LENGTH

uint8_t program_process_command_and_invalidate(void) {
	char *buf = usart_command, *endptr = NULL;
	uint8_t invalidate_program = 0;

	printf_P(PSTR("% "));
	if (buf[0] == '\0' || buf[0] == '\n' || buf[0] == '\r' || buf[0] == ' ' || buf[0] == '\t') {
		usart_command_available = 0;
		return;
	}
	
	if (strncmp(usart_command, PROGRAM_COMMAND_RESET, PROGRAM_COMMAND_RESET_LENGTH) == 0) {
		printf_P(PSTR("OK\n"));
		// call the watchdog timer to reset in 15ms
		wdt_enable(WDTO_15MS);
		while (1) {};
	} else if (strncmp(usart_command, PROGRAM_COMMAND_FACTORY, PROGRAM_COMMAND_FACTORY_LENGTH) == 0) {
		printf_P(PSTR("resetting to default program\n"));
		program_setup_default();
		printf_P(PSTR("OK\n"));
		invalidate_program = 1;
	} else if (strncmp(usart_command, PROGRAM_COMMAND_STOP, PROGRAM_COMMAND_STOP_LENGTH) == 0) {
		program_state = PROGRAM_STOP;
		invalidate_program = 1;
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_PROGRAMMING_MODE, PROGRAM_COMMAND_PROGRAMMING_MODE_LENGTH) == 0) {
		program_state = PROGRAM_PROGRAMMING;
		rgb_set(0, 0, 0);
		invalidate_program = 1;
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_RUN, PROGRAM_COMMAND_RUN_LENGTH) == 0) {
		program_state = PROGRAM_RUN;
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_LENGTH, PROGRAM_COMMAND_LENGTH_LENGTH) == 0) {
		if (program_state == PROGRAM_PROGRAMMING) {
			buf = &usart_command[PROGRAM_COMMAND_LENGTH_LENGTH];
			uint16_t steps = strtoul(buf, &endptr, 10);
			if (*buf != endptr) {
				uint16_t location = PROGRAM_START;
				printf_P(PSTR("program steps = %" PRIu16 "\n"), steps);
				eeprom_update_word((uint16_t *)location, steps);
				printf_P(PSTR("OK\n"));
			} else {
				printf_P(PSTR("invalid program length\nERROR\n"));
			}
		} else {
			printf_P(PSTR("not in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_STEP, PROGRAM_COMMAND_STEP_LENGTH) == 0) {
		if (program_state == PROGRAM_PROGRAMMING) {
			buf = &usart_command[PROGRAM_COMMAND_STEP_LENGTH];
			uint8_t rgb[3], parse_error = 0;
			uint16_t step, delay_in_ms;
			step = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse STEP value\n"));
				parse_error = 1;
			}
			buf = endptr;
			rgb[0] = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse RED value\n"));
				parse_error = 1;
			}
			buf = endptr;
			rgb[1] = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse GREEN value\n"));
				parse_error = 1;
			}
			buf = endptr;
			rgb[2] = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse BLUE value\n"));
				parse_error = 1;
			}
			buf = endptr;
			delay_in_ms = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse DELAY_IN_MS value\n"));
				parse_error = 1;
			} else if (delay_in_ms > 4000) {
				printf_P(PSTR("delay_in_ms must be less than 4000 ms\n"));
				parse_error = 1;
			}
			if (!parse_error) {
    			uint16_t location = PROGRAM_STEP_LOCATION(step);
				rgb_set(rgb[0], rgb[1], rgb[2]);
    			eeprom_update_block(rgb, (void *)location, 3 * sizeof(uint8_t));
    			location += 3 * sizeof(uint8_t);
    			eeprom_update_word((uint16_t *)location, delay_in_ms);
    			printf_P(PSTR("program step  = %" PRIu16 "\n"), step);
    			printf_P(PSTR("program rgb   = %u\t%u\t%u\n"), rgb[0], rgb[1], rgb[2]);
    			printf_P(PSTR("program delay = %" PRIu16 "\n"), delay_in_ms);
				printf_P(PSTR("OK\n"));
			} else {
				printf_P(PSTR("ERROR\n"));
			}
		} else {
			printf_P(PSTR("currently running program!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_OFF, PROGRAM_COMMAND_OFF_LENGTH) == 0) {
		if (program_state != PROGRAM_PROGRAMMING) {
			program_state = PROGRAM_STOP;
			rgb_set(0, 0, 0);
			invalidate_program = 1;
			printf_P(PSTR("OK\n"));
		} else {
			printf_P(PSTR("not in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_RGB, PROGRAM_COMMAND_RGB_LENGTH) == 0) {
		if (program_state != PROGRAM_PROGRAMMING) {
			buf = &usart_command[PROGRAM_COMMAND_RGB_LENGTH];
			uint8_t rgb[3], parse_error = 0;
			rgb[0] = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse RED value\n"));
				parse_error = 1;
			}
			buf = endptr;
			rgb[1] = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse GREEN value\n"));
				parse_error = 1;
			}
			rgb[2] = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse BLUE value\n"));
				parse_error = 1;
			}
			if (!parse_error) {
				program_state = PROGRAM_STOP;
				rgb_set(rgb[0], rgb[1], rgb[2]);
				invalidate_program = 1;
				printf_P(PSTR("OK\n"));
			} else {
				printf_P(PSTR("ERROR\n"));
			}
		} else {
			printf_P(PSTR("currently in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_RED, PROGRAM_COMMAND_RED_LENGTH) == 0) {
		if (program_state != PROGRAM_PROGRAMMING) {
			buf = &usart_command[PROGRAM_COMMAND_RED_LENGTH];
			uint8_t red, parse_error = 0;
			red = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse RED value\n"));
				parse_error = 1;
			}
			if (!parse_error) {
				program_state = PROGRAM_STOP;
				rgb_set_ch(0, red);
				invalidate_program = 1;
				printf_P(PSTR("OK\n"));
			} else {
				printf_P(PSTR("ERROR\n"));
			}
		} else {
			printf_P(PSTR("currently in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_GREEN, PROGRAM_COMMAND_GREEN_LENGTH) == 0) {
		if (program_state != PROGRAM_PROGRAMMING) {
			buf = &usart_command[PROGRAM_COMMAND_GREEN_LENGTH];
			uint8_t green, parse_error = 0;
			green = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse GREEN value\n"));
				parse_error = 1;
			}
			if (!parse_error) {
				program_state = PROGRAM_STOP;
				rgb_set_ch(1, green);
				invalidate_program = 1;
				printf_P(PSTR("OK\n"));
			} else {
				printf_P(PSTR("ERROR\n"));
			}
		} else {
			printf_P(PSTR("currently in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_BLUE, PROGRAM_COMMAND_BLUE_LENGTH) == 0) {
		if (program_state != PROGRAM_PROGRAMMING) {
			buf = &usart_command[PROGRAM_COMMAND_BLUE_LENGTH];
			uint8_t blue, parse_error = 0;
			blue = strtoul(buf, &endptr, 10);
			if (*buf == endptr) {
				printf_P(PSTR("unable to parse BLUE value\n"));
				parse_error = 1;
			}
			if (!parse_error) {
				program_state = PROGRAM_STOP;
				rgb_set_ch(2, blue);
				invalidate_program = 1;
				printf_P(PSTR("OK\n"));
			} else {
				printf_P(PSTR("ERROR\n"));
			}
		} else {
			printf_P(PSTR("currently in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_OFF, PROGRAM_COMMAND_OFF_LENGTH) == 0) {
		if (program_state != PROGRAM_PROGRAMMING) {
			program_state = PROGRAM_STOP;
			rgb_set(0, 0, 0);
			invalidate_program = 1;
			printf_P(PSTR("OK\n"));
		} else {
		printf_P(PSTR("currently in programming mode!\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_LHZEN, PROGRAM_COMMAND_LHZEN_LENGTH) == 0) {
		buf = &usart_command[PROGRAM_COMMAND_LHZEN_LENGTH];
		uint16_t lhz_enable = strtoul(buf, &endptr, 10);
		if (*buf != endptr) {
			uint16_t location = PROGRAM_ICP_HZ_ENABLE_LOCATION;
			printf_P(PSTR("light frequency HZ enable threshold = %" PRIu16 "\n"), lhz_enable);
			eeprom_update_word((uint16_t *)location, lhz_enable);
			program_icp_hz_enable = lhz_enable;
			invalidate_program = 1;
			printf_P(PSTR("OK\n"));
		} else {
			printf_P(PSTR("invalid light frequency Hz enable threshold\nERROR\n"));
		}
	} else if (strncmp(usart_command, PROGRAM_COMMAND_LHZ, PROGRAM_COMMAND_LHZ_LENGTH) == 0) {
		printf_P(PSTR("light frequency:\t%d\n"), icp_hz);
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_DUMP, PROGRAM_COMMAND_DUMP_LENGTH) == 0) {
		uint16_t location = PROGRAM_START;
		uint16_t i, steps, delay_in_ms;
		uint8_t rgb[3];
		printf_P(PSTR("\nPROGRAM\n"));
		steps = eeprom_read_word((uint16_t *)location);
		location += sizeof(uint16_t);
		printf_P(PSTR("LENGTH %d\n"), steps);
		for (i = 0; i < steps; ++i) {
			eeprom_read_block(&rgb, (void *)location, 3 * sizeof(uint8_t));
			location += 3 * sizeof(uint8_t);
			delay_in_ms = eeprom_read_word((uint16_t *)location);
			location += sizeof(uint16_t);
			printf_P(PSTR("STEP\t%d\t%d\t%d\t%d\t%d\n"), i, rgb[0], rgb[1], rgb[2], delay_in_ms);
		}
		printf_P(PSTR("RUN\n\n"));
		printf_P(PSTR("OK\n"));
	} else if (strncmp(usart_command, PROGRAM_COMMAND_STATUS, PROGRAM_COMMAND_STATUS_LENGTH) == 0) {
//		printf_P(PSTR(""),);
		printf_P(PSTR("state: %s\n"), (program_state == PROGRAM_STOP ? "stop" : \
			(program_state == PROGRAM_RUN ? "run" : \
			(program_state == PROGRAM_PROGRAMMING ? "program" : "unknown"))));
		printf_P(PSTR("command overflows: %d\n"), usart.command_overflows);
		printf_P(PSTR("RX buffer overflows: %d\n"), usart.RX_buffer.overflows);
		printf_P(PSTR("TX buffer overflows: %d\n"), usart.TX_buffer.overflows);
		printf_P(PSTR("light frequency:\t%d\n"), icp_hz);
		printf_P(PSTR("light frequency enable threshold:\t%d\n"), program_icp_hz_enable);
		printf_P(PSTR("OK\n"));
	} else {
		printf_P(PSTR("unrecognized command\nERROR\n"));
	}
// } else if (strncmp(usart_command, PROGRAM_COMMAND_, PROGRAM_COMMAND__LENGTH) == 0) {

	usart_command_available = 0;

	return invalidate_program;
}
