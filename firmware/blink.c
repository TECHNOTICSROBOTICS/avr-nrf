/*
 * Copyright 2011 Fabio Baltieri <fabio.baltieri@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include "defines.h"

#include <avr/io.h>
#include <avr/interrupt.h>

static uint8_t led_timeout_a;
static uint8_t led_timeout_b;
static uint8_t led_timeout_c;

void blink_init(void)
{
	TIMSK0 = ( (1 << OCIE0A) );

	TCCR0A = ( (1 << WGM01) | (0 << WGM00) ); /* CTC */
	TCCR0B = ( (0 << WGM02) |
		   (1 << CS02) | (0 << CS01) | (1 << CS00) ); /* /1024 */

	OCR0A = F_CPU / 1024 / 66; /* about 66Hz */

	led_timeout_a = 0;
}

ISR(TIMER0_COMPA_vect)
{
	if (chg_read()) {
		if (led_timeout_a++ > 20) {
			led_a_toggle();
			led_timeout_a = 0;
		}
	} else {
		led_a_on();
	}

	led_timeout_b--;
	if (led_timeout_b == 0)
		led_b_off();

	led_timeout_c--;
	if (led_timeout_c == 0)
		led_c_off();
}

void blink_tx(void)
{
	led_c_on();
	led_timeout_c = 2;
}

void blink_rx(void)
{
	led_b_on();
	led_timeout_b = 2;
}
