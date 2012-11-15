#define HZ 200
extern volatile uint16_t jiffies;

void blink_init(void);
void blink_status(void);
void blink_tx(void);
void blink_rx(void);
