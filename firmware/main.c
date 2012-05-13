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
#include <avr/wdt.h>
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
#include "suspend.h"
#include "fifo.h"

static uint8_t inbuf[EP1_SIZE];
static uint8_t outbuf[EP2_SIZE];

static void debug_tx(char *msg)
{
	static uint8_t idx;

	if (!fifo_full(&rf_tx_fifo)) {
		memset(outbuf, '\0', 16);
		snprintf((char *)outbuf, 16, "%02x,%02x %s", get_board_id(), idx++, msg);
		memcpy(fifo_get_head(&rf_tx_fifo), outbuf, 16);
		fifo_push(&rf_tx_fifo);
	}

	nrf_wake_queue();
}

static void hello(void)
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
	if (button_read())
		suspend_disable(SLEEP_USER);
	else
		suspend_enable(SLEEP_USER);
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

ISR(WDT_vect)
{
	static uint8_t state;

	if (chg_read()) {
		led_a_toggle();
	} else if (!suspend_check(SLEEP_USB)) {
		if (state)
			blink_status();
		state = !state;
	} else {
		led_a_on();
	}
}

static void usb_in(void *user)
{
	if (!fifo_full(&rf_tx_fifo)) {
		memcpy(fifo_get_head(&rf_tx_fifo), (uint8_t *)user, 16);
		fifo_push(&rf_tx_fifo);
	}

	nrf_wake_queue();
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

        WDTCSR |= (1 << WDE) | (1 << WDCE);
	WDTCSR = (0 << WDCE) | (1 << WDIF) | (1 << WDIE) |
		 (1 << WDP2) | (0 << WDP1) | (1 << WDP0);

	irq_init();

	hello();
	sei();

	nrf_standby();

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

		cli();
		if (fifo_count(&rf_rx_fifo) && eps[2].state == EP_IDLE) {
			memcpy(outbuf, fifo_get_tail(&rf_rx_fifo), 16);
			usb_send(&eps[2], outbuf, 16,
				 NULL, NULL);
			fifo_pop(&rf_rx_fifo);
		}
		sei();

		cli();
		if (eps[1].state == EP_IDLE)
			usb_recv(&eps[1], inbuf, 16, usb_in, inbuf);
		sei();

		cli();
		if (suspend_check(SLEEP_USB))
			nrf_rx_enable();
		else
			nrf_rx_disable();
		sei();

		if (can_suspend())
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		else
			set_sleep_mode(SLEEP_MODE_IDLE);
		sleep_mode();
	}
}
