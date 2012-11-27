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

static int sock;

int net_init(int port)
{
	int ret;
	struct sockaddr_in6 addr;
	int val;

	sock = socket(AF_INET6, SOCK_DGRAM, 0);
	if (socket < 0) {
		perror("socket");
		exit(1);
	}

	val = 0;
	ret = setsockopt(sock, SOL_IPV6, IPV6_V6ONLY, &val, sizeof(int));
	if (ret < 0) {
		perror("socket");
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	memcpy(&addr.sin6_addr, &in6addr_any, sizeof(in6addr_any));

	ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		perror("socket");
		exit(1);
	}

	return 0;
}

#define MSG_DISCOVER    "/nrf/discover"
#define MSG_IO          "/nrf/io"
#define MSG_INIT        "/nrf/init"
#define MSG_BOUND       "/nrf/bound"

static void net_send_init(struct sockaddr_in6 *addr, char *buf, int size)
{
	int len;

	memset(buf, 0, size);

	memcpy(buf, MSG_INIT, sizeof(MSG_INIT));

	len = strlen(MSG_INIT);
	len += 4 - (len % 4);

	buf[len] = ',';
	len += 4;

	sendto(sock, buf, len, 0, (struct sockaddr *)addr, sizeof(*addr));
}

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

static int check_bound(char *data, int size)
{
	int hlen;
	uint32_t *len;

	hlen = strlen(MSG_BOUND);
	hlen += 4 - (hlen % 4);

	if (size < hlen + 8 + 4)
		return 0;

	if (memcmp(",iib\0\0\0\0", data + hlen, 8))
		return 0;

	len = (uint32_t *)(data + hlen + 8);
	*len = ntohl(*len);

	return *len;
}

int net_poll(char *data, int size)
{
	int ret;
	unsigned int len;
	struct sockaddr_in6 addr;
	char buf[1500];
	char addrbuf[32];

	len = sizeof(addr);
	ret = recvfrom(sock,
		       buf, sizeof(buf),
		       0,
		       (struct sockaddr *)&addr, &len);
	if (ret < 0) {
		perror("socket");
		exit(1);
	}

	inet_ntop(AF_INET6, &addr.sin6_addr, addrbuf, sizeof(addrbuf));
	printf("got %d bytes from %s:%d [%s]\n", ret,
	       addrbuf, ntohs(addr.sin6_port), buf);

	if (memcmp(MSG_DISCOVER, buf, sizeof(MSG_DISCOVER)) == 0) {
		net_send_init(&addr, buf, sizeof(buf));
	} else if (memcmp(MSG_IO, buf, sizeof(MSG_IO)) == 0) {
		return check_io(buf, ret, data, size);
	} else if (memcmp(MSG_BOUND, buf, sizeof(MSG_BOUND)) == 0) {
		check_bound(buf, ret);
	}

	return -EAGAIN;
}

void net_close(void)
{
	close(sock);
}
