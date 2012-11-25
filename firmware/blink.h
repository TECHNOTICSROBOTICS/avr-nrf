#define HZ 200
extern volatile uint16_t jiffies;

void blink_init(void);
void blink_a(void);
void blink_b(void);
void blink_c(void);

#define led_nrf_rx() blink_b()
#define led_nrf_tx() blink_b()
#define led_usb_rx() blink_a()
#define led_usb_tx() blink_a()
#define led_net_rx() blink_c()
#define led_net_tx() blink_c()
