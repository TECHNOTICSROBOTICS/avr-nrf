/*
 * Copyright 2011 Fabio Baltieri <fabio.baltieri@gmail.com>
 *
 * Based on the original ben-wpan code written by
 *   Werner Almesberger, Copyright 2008-2011
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "defines.h"

#include "usb.h"
#include "board.h"
#include "descr.h"
#include "ep0.h"
#include "spi.h"
#include "nrf24l01p.h"
#include "ksz8851snl.h"
#include "blink.h"

void hello(void)
{
	uint8_t i;

	_delay_ms(100);

	for (i = 0; i < 5; i++) {
		led_c_on();
		led_b_on();

		_delay_ms(50);

		led_c_off();
		led_b_off();

		_delay_ms(50);
	}
}

ISR(INT0_vect)
{
}
ISR(INT4_vect)
{
}
ISR(INT6_vect)
{
}
ISR(INT7_vect)
{
	nrf_irq();
}

int main(void)
{
	board_init();
	led_init();
	spi_init();
	blink_init();
	chg_init();

	_delay_ms(10);

	led_a_on();

	/* IO init */
	NRF_DDR |= _BV(NRF_CS) | _BV(NRF_CE);
	ETH_DDR |= _BV(ETH_CS);
	nrf_cs_h();
	nrf_ce_l();
	eth_cs_h();

	nrf_init();
	ksz8851_init();

	usb_init();
	ep0_init();

	user_get_descriptor = strings_get_descr;

	/* move interrupt vectors to 0 */
	MCUCR = 1 << IVCE;
	MCUCR = 0;

	irq_init();

	sei();

	hello();

	nrf_powerup();
	nrf_mode_rx();

	while (1) {
#if 0
		cli();
		if (nrf_read_reg(RPD))
			blink_rx();
		sei();
#endif

#if 0
		cli();
		ksz8851_irq();
		sei();
#endif

		set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();
	}
}
