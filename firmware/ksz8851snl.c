/*
 * Copyright 2012 Fabio Baltieri <fabio.baltieri@gmail.com>
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
#include <util/delay.h>
#include <stdlib.h>

#include "spi.h"
#include "blink.h"

#include "ksz8851snl.h"

#define BYTE_EN_LOW	0x0c
#define BYTE_EN_HIGH	0x30

#define OP_REG_READ	0x00
#define OP_REG_WRITE	0x40
#define OP_FIFO_READ	0x80
#define OP_FIFO_WRITE	0xc0

static uint16_t rxqcr;
static uint8_t fid;

void ksz8851_write_reg(uint8_t reg, uint16_t val)
{
	eth_cs_l();

	spi_io(OP_REG_WRITE |
	       ((reg & 2) ? BYTE_EN_HIGH : BYTE_EN_LOW) |
	       reg >> 6);
	spi_io((reg << 2) & 0xf0);
	spi_io(val & 0xff);
	spi_io(val >> 8);

	eth_cs_h();
}

uint16_t ksz8851_read_reg(uint8_t reg)
{
	uint16_t ret;

	ret = 0;

	eth_cs_l();

	spi_io(OP_REG_READ |
	       ((reg & 2) ? BYTE_EN_HIGH : BYTE_EN_LOW) |
	       reg >> 6);
	spi_io((reg << 2) & 0xf0);
	ret  = spi_io(0xff);
	ret |= spi_io(0xff) << 8;

	eth_cs_h();

	return ret;
}

static void ksz8851_write_fifo(uint8_t *buf, uint16_t length)
{
	uint16_t i;

	fid = (fid + 1) & TXFR_TXFID_MASK;

	eth_cs_l();

	spi_io(OP_FIFO_WRITE);

	i = fid | TXFR_TXIC;
	spi_io(i & 0xff);
	spi_io(i >> 8);

	spi_io(length & 0xff);
	spi_io(length >> 8);

	for (i = 0; i < length; i++)
		spi_io(buf[i]);

	/* align on 4th bytes */
	while ((i++ % 4) != 0)
		spi_io(0x00);

	eth_cs_h();
}

static void ksz8851_read_fifo(uint8_t *buf, uint16_t length)
{
	uint16_t i;

	eth_cs_l();

	spi_io(OP_FIFO_READ);

	/* trash first 8 bytes */
	for (i = 0; i < 8; i++)
		spi_io(0xff);

	for (i = 0; i < length; i++)
		if (buf)
			buf[i] = spi_io(0xff);
		else
			spi_io(0xff);

	eth_cs_h();
}

static void ksz8851_soft_reset(uint8_t queue_only)
{
	if (queue_only)
		ksz8851_write_reg(KS_GRR, GRR_QMU);
	else
		ksz8851_write_reg(KS_GRR, GRR_GSR);

	_delay_ms(1);

	ksz8851_write_reg(KS_GRR, 0x00);

	_delay_ms(1);
}

static void ksz8851_set_pm(uint8_t mode)
{
	uint8_t pmecr;

	pmecr = ksz8851_read_reg(KS_PMECR);
	pmecr &= ~PMECR_PM_MASK;
	pmecr |= mode;
	ksz8851_write_reg(KS_PMECR, pmecr);
}

void ksz8851_send_packet(uint8_t *buf, uint16_t length)
{
	ksz8851_write_reg(KS_RXQCR, rxqcr | RXQCR_SDA);
	ksz8851_write_fifo(buf, length);
	ksz8851_write_reg(KS_RXQCR, rxqcr);
	ksz8851_write_reg(KS_TXQCR, TXQCR_METFE);
}

uint16_t ksz8851_read_packet(uint8_t *buf, uint16_t limit)
{
	uint16_t rxlen;
	uint16_t rxfctr;
	uint16_t ret = 0;
	uint8_t i;

	rxfctr = ksz8851_read_reg(KS_RXFC);

	rxfctr = rxfctr >> 8;
	for (i = 0; i < rxfctr; i++) {
		rxlen = ksz8851_read_reg(KS_RXFHBCR);

		ksz8851_write_reg(KS_RXFDPR, RXFDPR_RXFPAI | 0x00);
		ksz8851_write_reg(KS_RXQCR,
				rxqcr | RXQCR_SDA | RXQCR_ADRFE);

		if (rxlen > 4) {
			if (i == 0 && rxlen <= limit) {
				rxlen -= rxlen % 4;
				ksz8851_read_fifo(buf, rxlen);
				ret = rxlen;
			} else {
				rxlen -= rxlen % 4;
				ksz8851_read_fifo(NULL, rxlen);
				ret = 0;
			}
		}

		ksz8851_write_reg(KS_RXQCR, rxqcr);
	}

	return ret;
}

uint8_t ksz8851_has_data(void)
{
	uint16_t isr;

	isr = ksz8851_read_reg(KS_ISR);

	if (isr & IRQ_RXI) {
		ksz8851_write_reg(KS_ISR, isr);
		return 1;
	} else {
		ksz8851_write_reg(KS_ISR, isr);
		return 0;
	}
}

void ksz8851_irq(void)
{
	uint16_t isr;

	isr = ksz8851_read_reg(KS_ISR);

	if (isr & IRQ_LCI)
		;

	if (isr & IRQ_LDI)
		;

	if (isr & IRQ_RXPSI)
		;

	if (isr & IRQ_TXI)
		blink_tx();

	if (isr & IRQ_RXI)
		blink_rx();

	if (isr & IRQ_SPIBEI)
		;

	ksz8851_write_reg(KS_ISR, isr);

	if (isr & IRQ_RXI)
		;
}

void ksz8851_init(void)
{
	fid = 0;

	eth_cs_h();

	ksz8851_soft_reset(0);

	/* set MAC address */
	ksz8851_write_reg(KS_MARH, 0x0021);
	ksz8851_write_reg(KS_MARM, 0xf300);
	ksz8851_write_reg(KS_MARL, 0x8851);

	/* PM NORMAL */
	ksz8851_set_pm(PMECR_PM_NORMAL);

	/* QMU reset */
	ksz8851_soft_reset(1);

	/* TX parameters */
	ksz8851_write_reg(KS_TXCR, (TXCR_TXE | /* enable transmit process */
				    TXCR_TXPE | /* pad to min length */
				    TXCR_TXCRC | /* add CRC */
				    TXCR_TXFCE)); /* enable flow control */

	/* auto-increment tx data, reset tx pointer */
        ksz8851_write_reg(KS_TXFDPR, TXFDPR_TXFPAI);

        /* RX parameters */
        ksz8851_write_reg(KS_RXCR1, (RXCR1_RXPAFMA | /*  from mac filter */
				     RXCR1_RXFCE | /* enable flow control */
				     RXCR1_RXBE | /* broadcast enable */
				     RXCR1_RXUE | /* unicast enable */
				     RXCR1_RXE)); /* enable rx block */

	/* transfer entire frames out in one go */
        ksz8851_write_reg(KS_RXCR2, RXCR2_SRDBL_FRAME);

	/* set receive counter timeouts */
        ksz8851_write_reg(KS_RXDTTR, 1000); /* 1ms after first frame to IRQ */
        ksz8851_write_reg(KS_RXDBCTR, 4096); /* >4Kbytes in buffer to IRQ */
        ksz8851_write_reg(KS_RXFCTR, 10);  /* 10 frames to IRQ */

	rxqcr = RXQCR_RXFCTE |  /* IRQ on frame count exceeded */
		RXQCR_RXDBCTE | /* IRQ on byte count exceeded */
		RXQCR_RXDTTE;  /* IRQ on time exceeded */

	ksz8851_write_reg(KS_RXQCR, rxqcr);

	ksz8851_write_reg(KS_ISR, (IRQ_LCI |      /* Link Change */
				   IRQ_TXI |      /* TX done */
				   IRQ_RXI |      /* RX done */
				   IRQ_SPIBEI |   /* SPI bus error */
				   IRQ_TXPSI |    /* TX process stop */
				   IRQ_RXPSI));

	ksz8851_write_reg(KS_IER, (IRQ_LCI |      /* Link Change */
				   IRQ_TXI |      /* TX done */
				   IRQ_RXI |      /* RX done */
				   IRQ_SPIBEI |   /* SPI bus error */
				   IRQ_TXPSI |    /* TX process stop */
				   IRQ_RXPSI));
}
