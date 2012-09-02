#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "nrf_frames.h"

static int open_port(char * path)
{
	int ret;
	int fd;

	fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		perror("open");
		exit(-1);
	}

	ret = fcntl(fd, F_SETFL, O_RDWR);
	if (ret < 0) {
		perror("fcntl");
		exit(-1);
	}

	return fd;
}

static int config_port(int fd)
{
	struct termios tp;
	int ret;

	ret = tcgetattr(fd, &tp);
	if (ret < 0)
		perror("tcgetattr");

	cfmakeraw(&tp);

	cfsetspeed(&tp, B38400);

	ret = tcsetattr(fd, TCSANOW, &tp);
	if (ret < 0)
		perror("tcsetattr");

	ret = tcflush(fd, TCIOFLUSH);
	if (ret < 0)
		perror("tcflush");

	return ret;
}

#define printable(x) (x > ' ' && x < '~' ? x : '.')

static void dump_generic(struct nrf_frame *pkt)
{
	char *bytes;

	bytes = (char *)pkt->msg.generic;

	printf("generic: "
	       "%02hhx %02hhx %02hhx %02hhx  "
	       "%02hhx %02hhx %02hhx %02hhx  "
	       "%02hhx %02hhx %02hhx %02hhx  "
	       "|%c%c%c%c%c%c%c%c%c%c%c%c|",

	       bytes[0],
	       bytes[1],
	       bytes[2],
	       bytes[3],
	       bytes[4],
	       bytes[5],
	       bytes[6],
	       bytes[7],
	       bytes[8],
	       bytes[9],
	       bytes[10],
	       bytes[11],

	       printable(bytes[0]),
	       printable(bytes[1]),
	       printable(bytes[2]),
	       printable(bytes[3]),
	       printable(bytes[4]),
	       printable(bytes[5]),
	       printable(bytes[6]),
	       printable(bytes[7]),
	       printable(bytes[8]),
	       printable(bytes[9]),
	       printable(bytes[10]),
	       printable(bytes[11]) );
}

static void decode(struct nrf_frame *pkt)
{
	printf("board_id=%02x, msg_id=%02x, seq=%02x, flags=%02x: ",
	       pkt->board_id,
	       pkt->msg_id,
	       pkt->seq,
	       pkt->flags);

	switch (pkt->msg_id) {
	case NRF_MSG_ID_GENERIC:
	default:
		dump_generic(pkt);
	}

	printf("\n");
}

int main(int argc, char **argv)
{
	int fd;
	struct nrf_frame pkt;
	int ret;

	if (argc != 2) {
		fprintf(stderr, "%s: PORT\n", argv[0]);
		exit(1);
	}

	fd = open_port(argv[1]);

	config_port(fd);

	printf("Start, pkt size = %d bytes\n", sizeof(pkt));
	for (;;) {
		ret = read(fd, &pkt, sizeof(pkt));
		if (ret != sizeof(pkt)) {
			perror("read");
			exit(1);
		}

		decode(&pkt);
	}

	close(fd);

	return 0;
}
