enum {
	SLEEP_USER,
	SLEEP_TIMER0,
	SLEEP_USB,
};

void suspend_enable(uint8_t id);
void suspend_disable(uint8_t id);
uint8_t can_suspend(void);
