#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <endian.h>
#include <errno.h>

#include <usb.h>
#include "opendevice.h"

#include "nrf_frames.h"

#define USB_PRODUCT "avr-nrf"
#define USB_EP_IN 2

static enum {
	MODE_FILE = 0,
	MODE_USB,
	MODE_NET
} mode;

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

static void dump_power(struct nrf_frame *pkt)
{
	struct nrf_power *pwr = &pkt->msg.power;

	pwr->value[0] = le16toh(pwr->value[0]);
	pwr->value[1] = le16toh(pwr->value[1]);
	pwr->value[2] = le16toh(pwr->value[2]);
	pwr->value[3] = le16toh(pwr->value[3]);
	pwr->vbatt = le16toh(pwr->vbatt);

	printf("power: "
	       "value=%4d,%4d,%4d,%4d vbatt=%4d",
	       pwr->value[0],
	       pwr->value[1],
	       pwr->value[2],
	       pwr->value[3],
	       pwr->vbatt & NRF_POWER_VBATT_MASK);

	if (pwr->vbatt & NRF_POWER_VBATT_CHARGING)
		putchar('+');
}

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
	       printable(bytes[11]));
}

static struct timeval last_tv;
static void print_ts(void)
{
	struct timeval tv;
	struct timeval temp_tv;
	int ret;

	ret = gettimeofday(&tv, NULL);
	if (ret < 0) {
		perror("gettimeofday");
		exit(1);
	}

	/* printf("%03d.%06d ", tv.tv_sec, tv.tv_usec); */
	timersub(&tv, &last_tv, &temp_tv);
	printf("%03ld.%06ld ",
			temp_tv.tv_sec,
			temp_tv.tv_usec);

	memcpy(&last_tv, &tv, sizeof(tv));
}

static void decode(struct nrf_frame *pkt)
{
	print_ts();
	printf("board_id=%02x, msg_id=%02x, len=%2d, seq=%02x: ",
	       pkt->board_id,
	       pkt->msg_id,
	       pkt->len,
	       pkt->seq);

	switch (pkt->msg_id) {
	case NRF_MSG_ID_POWER:
		dump_power(pkt);
		break;
	case NRF_MSG_ID_GENERIC:
	default:
		dump_generic(pkt);
	}

	printf("\n");
	fflush(stdout);
}

int main(int argc, char **argv)
{
	int fd;
	struct nrf_frame pkt;
	int ret;
	usb_dev_handle *handle = NULL;

	usb_init();

	if (argc != 2) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s: DEVICE\n", argv[0]);
		fprintf(stderr, "  %s: PORT\n", argv[0]);
		fprintf(stderr, "  %s: usb\n", argv[0]);
		fprintf(stderr, "  %s: -\n", argv[0]);
		exit(1);
	}

	if (strcmp("-", argv[1]) == 0) {
		fd = fileno(stdin);
		mode = MODE_FILE;
	} else if (atoi(argv[1]) != 0) {
		printf("net\n");
		mode = MODE_NET;
	} else if (strcmp("usb", argv[1]) == 0) {
		ret = usbOpenDevice(&handle,
				0, NULL,
				0, USB_PRODUCT,
				NULL, NULL, NULL);
		if (ret) {
			fprintf(stderr, "error: could not find USB device \"%s\"\n", USB_PRODUCT);
			exit(1);
		}

		usb_claim_interface(handle, 0);

		mode = MODE_USB;
	} else {
		fd = open_port(argv[1]);

		config_port(fd);

		mode = MODE_FILE;
	}

	printf("Start, pkt size = %ld bytes\n", sizeof(pkt));
	for (;;) {
		if (mode == MODE_USB) {
			ret = usb_bulk_read(handle, USB_EP_IN,
					(char *)&pkt, sizeof(pkt),
					10000);
			if (ret == -ETIMEDOUT ||
			    ret == -EAGAIN)
				continue;
		} else {
			ret = read(fd, &pkt, sizeof(pkt));
		}

		if (ret != sizeof(pkt)) {
			perror("read");
			exit(1);
		}

		decode(&pkt);
	}

	if (mode == MODE_USB) {
		usb_release_interface(handle, 0);
		usb_close(handle);
	} else {
		close(fd);
	}

	return 0;
}
