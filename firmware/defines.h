/* buttons */
#define BUTTON_PORT PORTD
#define BUTTON_DDR  DDRD
#define BUTTON_PIN  PIND
#define BUTTON PD0

#define button_init() do { BUTTON_PORT |= _BV(BUTTON_HWB); } while (0)
#define button_read() (!(BUTTON_PIN & _BV(BUTTON)))

/* leds */
#define LED_A_PORT PORTC
#define LED_A_DDR  DDRC
#define LED_A PC6
#define LED_B_PORT PORTB
#define LED_B_DDR DDRB
#define LED_B PB7
#define LED_C_PORT PORTC
#define LED_C_DDR DDRC
#define LED_C PC5

#define led_init() do {                                         \
	                LED_A_DDR |= _BV(LED_A);                        \
	                LED_B_DDR |= _BV(LED_B);                        \
	                LED_C_DDR |= _BV(LED_C);                        \
	        } while (0)

#define led_a_on()     LED_A_PORT |=  _BV(LED_A)
#define led_a_off()    LED_A_PORT &= ~_BV(LED_A)
#define led_a_toggle() LED_A_PORT ^=  _BV(LED_A)
#define led_b_on()     LED_B_PORT |=  _BV(LED_B)
#define led_b_off()    LED_B_PORT &= ~_BV(LED_B)
#define led_c_on()     LED_C_PORT |=  _BV(LED_C)
#define led_c_off()    LED_C_PORT &= ~_BV(LED_C)

/* Charger */
#define CHG_PORT        PORTC
#define CHG_PIN         PINC
#define CHG             PC7

#define chg_init() CHG_PORT |= _BV(CHG)
#define chg_read() (!(CHG_PIN & _BV(CHG)))

/* SPI */
#define SPI_DDR  DDRB
#define SPI_PORT PORTB
#define SPI_SS   PB0
#define SPI_SCK  PB1
#define SPI_MOSI PB2
#define SPI_MISO PB3

/* nRF24L01+ SPI */
#define NRF_DDR  DDRB
#define NRF_PORT PORTB
#define NRF_CS   SPI_SS
#define NRF_CE   PB4

#define nrf_cs_h() (NRF_PORT |=  _BV(NRF_CS))
#define nrf_cs_l() (NRF_PORT &= ~_BV(NRF_CS))
#define nrf_ce_h() (NRF_PORT |=  _BV(NRF_CE))
#define nrf_ce_l() (NRF_PORT &= ~_BV(NRF_CE))

/* KSZ8851SNL SPI */
#define ETH_DDR  DDRB
#define ETH_PORT PORTB
#define ETH_CS   PB5

#define eth_cs_h() (ETH_PORT |=  _BV(ETH_CS))
#define eth_cs_l() (ETH_PORT &= ~_BV(ETH_CS))
