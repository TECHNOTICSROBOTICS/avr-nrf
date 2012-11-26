#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>

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

char buf[128];
int net_poll(void)
{
	int ret;
	unsigned int len;
	struct sockaddr_in6 addr;

	len = sizeof(addr);
	ret = recvfrom(sock,
		       buf, 128,
		       0,
		       (struct sockaddr *)&addr, &len);
	if (ret < 0) {
		perror("socket");
		exit(1);
	}

	//TODO: wait for discover -> /nrf/discover
	//TODO: bind -> /nrf/init, check if still bound -> /nrf/bound
	//TODO: keep bound
	//TODO: receive data -> /nrf/io

	printf("ret\n");

	return 0;
}

void net_close(void)
{
}
