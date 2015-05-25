#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "global.h"
#include "rgb.h"
#include "usart.h"
#include "compatability.h"

volatile uint8_t rgb[3];

extern volatile uint16_t icp_hz;

void rgb_init(void) {
	LED_RED_INIT;
	LED_GREEN_INIT;
	LED_BLUE_INIT;
}

void rgb_set(uint8_t red, uint8_t green, uint8_t blue) {
	rgb[0] = red;
	rgb[1] = green;
	rgb[2] = blue;
}

void rgb_set_ch(uint8_t ch, uint8_t value) {
	// skip the value check on ch for performance
    rgb[ch] = value;
}

void rgb_rainbow(void) {
	float frequency = .3;
	uint8_t i, red, green, blue;
#ifdef RGB_DEBUG

	printf_P(PSTR("i\tred\tgreen\tblue\n"));
#endif
	for (i = 0; i < 43; ++i) {
   		red   = (uint8_t)(sin(frequency*(float)i + 0.) * 127.) + 128;
   		green = (uint8_t)(sin(frequency*(float)i + 2.) * 127.) + 128;
   		blue  = (uint8_t)(sin(frequency*(float)i + 4.) * 127.) + 128;
		rgb_set(red, green, blue);
#ifdef RGB_DEBUG
		printf_P(PSTR("%d :\t%d\t%d\t%d\n", i, red, green, blue);
#endif
		//_delay_ms(100);
		_delay_ms(50);
	}
}

void rgb_test(void) {
	printf_P(PSTR("red\n"));
	rgb_set(127, 0, 0);
	_delay_ms(5000);
	printf_P(PSTR("green\n"));
	rgb_set(0, 127, 0);
	_delay_ms(5000);
	printf_P(PSTR("blue\n"));
	rgb_set(0, 0, 127);
	_delay_ms(5000);
	printf_P(PSTR("bright white\n"));
	rgb_set(255, 255, 255);
	_delay_ms(5000);
	printf_P(PSTR("dim white\n"));
	rgb_set(50, 50, 50);
	_delay_ms(5000);
	printf_P(PSTR("off\n"));
	rgb_set(0, 0, 0);
	_delay_ms(5000);
}
