#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>

#define MCAST_PORT 9999

static int sock;

static char buf[1500];

int mcast_init(int port)
{
	int ret;
	struct sockaddr_in6 addr;
	int val;
	struct ipv6_mreq mreq6;
	struct ip_mreqn mreq;

	sock = socket(AF_INET6, SOCK_DGRAM, 0);
	if (socket < 0) {
		perror("socket");
		exit(1);
	}

	val = 0;
	ret = setsockopt(sock, SOL_IPV6, IPV6_V6ONLY, &val, sizeof(int));
	if (ret < 0) {
		perror("setsockopt");
		exit(1);
	}

	val = 1;
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
	if (ret < 0) {
		perror("setsockopt");
		exit(1);
	}

	memset(&mreq, 0, sizeof(mreq));
	inet_pton(AF_INET, "224.24.1.1", &mreq.imr_multiaddr);
	mreq.imr_address.s_addr = INADDR_ANY;
	mreq.imr_address.s_addr = INADDR_ANY;
	mreq.imr_ifindex = 0;
	ret = setsockopt(sock, SOL_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret < 0) {
		perror("setsockopt");
		exit(1);
	}

	memset(&mreq6, 0, sizeof(mreq6));
	inet_pton(AF_INET6, "ff02::24:1", &mreq6.ipv6mr_multiaddr);
	mreq6.ipv6mr_interface = 0;
	ret = setsockopt(sock, SOL_IPV6, IPV6_ADD_MEMBERSHIP, &mreq6, sizeof(mreq6));
	if (ret < 0) {
		perror("setsockopt");
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(MCAST_PORT);
	memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));

	ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		perror("bind");
		exit(1);
	}

	return 0;
}

#define MSG_IO          "/nrf/io"

static int check_io(char *in, int size_in, char *out, int size_out)
{
	int hlen;
	uint32_t *len;

	hlen = strlen(MSG_IO);
	hlen += 4 - (hlen % 4);

	if (size_in < hlen + 4 + 4)
		return 0;

	if (memcmp(",b\0\0", in + hlen, 4))
		return 0;

	len = (uint32_t *)(in + hlen + 4);
	*len = ntohl(*len);

	if (size_out < *len || size_in < hlen + 4 + 4 + *len)
		return 0;

	memcpy(out, in + hlen + 4 + 4, *len);

	return *len;
}

static void dbg(char *msg, struct sockaddr_in6 *addr)
{
#ifdef DEBUG
	char addrbuf[32];

	inet_ntop(AF_INET6, &addr->sin6_addr, addrbuf, sizeof(addrbuf));
	printf("%s: %s:%d\n", msg,
	       addrbuf, ntohs(addr->sin6_port));
#endif
}

int mcast_poll(char *data, int size)
{
	int ret;
	unsigned int len;
	struct sockaddr_in6 addr;

	len = sizeof(addr);
	ret = recvfrom(sock,
		       buf, sizeof(buf),
		       0,
		       (struct sockaddr *)&addr, &len);
	if (ret < 0) {
		perror("socket");
		exit(1);
	}

	if (memcmp(MSG_IO, buf, sizeof(MSG_IO)) == 0) {
		dbg("recv: io", &addr);
		return check_io(buf, ret, data, size);
	} else {
		dbg("recv: UNKNOWN", &addr);
	}

	return -EAGAIN;
}

void mcast_close(void)
{
	close(sock);
}
