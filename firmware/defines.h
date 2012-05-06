#include <stdint.h>

extern volatile uint8_t power_down;

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

/* Interrupts */
#define BUTTON_EICR	EICRA
#define BUTTON_ISC	((0 << ISC01) | (1 << ISC00))
#define BUTTON_INT	INT0

#define CHG_INT		INT4
#define CHG_EICR	EICRB
#define CHG_ISC		((0 << ISC41) | (1 << ISC40))

#define ETH_INT		INT6
#define ETH_EICR	EICRB
#define ETH_ISC		((1 << ISC61) | (0 << ISC60))

#define NRF_INT		INT7
#define NRF_EICR	EICRB
#define NRF_ISC		((1 << ISC71) | (0 << ISC70))

#define irq_init() do {					  \
		BUTTON_EICR |= BUTTON_ISC;		  \
		CHG_EICR |= CHG_ISC;			  \
		ETH_EICR |= ETH_ISC;			  \
		NRF_EICR |= NRF_ISC;			  \
		EIMSK |= _BV(BUTTON_INT) | _BV(CHG_INT) | \
			_BV(ETH_INT) | _BV(NRF_INT);	  \
	} while (0)
