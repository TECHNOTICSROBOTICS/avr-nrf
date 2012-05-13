/*
 * Copyright 2011 Fabio Baltieri <fabio.baltieri@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <stdio.h>

#include "suspend.h"

static volatile uint8_t suspend_mask = 0x00;

void suspend_enable(uint8_t id)
{
	suspend_mask &= ~(1 << id);
}

void suspend_disable(uint8_t id)
{
	suspend_mask |= (1 << id);
}

uint8_t suspend_check(uint8_t id)
{
	return !!(suspend_mask & (1 << id));
}

uint8_t can_suspend(void)
{
	return (suspend_mask == 0x00);
}
