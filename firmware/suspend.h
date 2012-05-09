enum {
	SLEEP_TIMER0
};

void suspend_enable(uint8_t id);
void suspend_disable(uint8_t id);
uint8_t can_suspend(void);
