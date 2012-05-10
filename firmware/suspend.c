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
