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
	// rgb_governor is a governor measure
	uint8_t rgb_governor;
#if 0
	if (icp_hz < 20)
		rgb_governor = 255;
	else {
		float tmp = (255. - (((float)icp_hz - 20.) * 3.2));
		rgb_governor = (uint8_t)(tmp < 0 ? 0 : tmp);
	}
#else
	rgb_governor = 255;
#endif

	rgb[0] = (red < rgb_governor ? red : rgb_governor);
	rgb[1] = (green < rgb_governor ? green : rgb_governor);
	rgb[2] = (blue < rgb_governor ? blue : rgb_governor);
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
