#include <stdint.h>

#define NRF_MSG_ID_GENERIC	0x00

struct nrf_frame {
	uint8_t board_id;
	uint8_t msg_id;
	uint8_t seq;
	uint8_t flags;
	union {
		uint8_t generic[12];
	} msg;
};
