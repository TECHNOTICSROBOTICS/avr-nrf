#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#include "net.h"
#include "../ksz8851snl.h"
#include "../blink.h"

#define NETBUF_SZ 256
static uint8_t EEMEM my_mac_ee[6] = {0x00, 0x21, 0xf3, 0x00, 0x32, 0x02};
static uint8_t EEMEM my_ip_ee[4] = {172, 16, 0, 33};
/* TODO: manage defaults and set MAC in phy */
static uint8_t my_mac[6] = {0x00, 0x21, 0xf3, 0x00, 0x88, 0x51};;
static uint8_t my_ip[4] = {172, 16, 0, 33};
static uint8_t bound = 0;
static uint8_t remote_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t remote_ip[4] = {0, 0, 0, 0};
static uint8_t buf[NETBUF_SZ];
static uint16_t net_idx;

static void eeprom_restore (void)
{
	eeprom_read_block(my_mac, my_mac_ee, ETH_ALEN);
	eeprom_read_block(my_ip, my_ip_ee, 4);
}

static void eeprom_save (void)
{
	eeprom_write_block(my_mac, my_mac_ee, ETH_ALEN);
	eeprom_write_block(my_ip, my_ip_ee, 4);
}

#define DATA_OFF (ETH_HLEN + IP_HLEN + UDP_HLEN)

static void process_udp (uint8_t * buf)
{
	struct ethhdr * eth;
	struct iphdr * ip;
	struct udphdr * udp;
	uint8_t * data;
	uint16_t size;

	eth = (struct ethhdr *)&buf[0];
	ip = (struct iphdr *)&buf[ETH_HLEN];
	udp = (struct udphdr *)&buf[ETH_HLEN + IP_HLEN];
	data = &buf[ETH_HLEN + IP_HLEN + UDP_HLEN];

	swap_ethernet(eth);
	swap_ip(ip);
	swap_udp(udp);

	if (be16(udp->source) != 9999)
		return;

	size = be16(udp->len) - UDP_HLEN;

	if (size == 12 && memcmp(data, "/init", 5) == 0) {
		memcpy(remote_ip, &ip->daddr, 4);
		memcpy(remote_mac, &eth->h_dest, ETH_ALEN);
		bound = 1;
	} else {
		return;
	}

	udp->dest = be16(9999);

	ip->ttl = IPDEFTTL;

	memset(data, 0, NETBUF_SZ - DATA_OFF);
	size = snprintf((char *)data, NETBUF_SZ - DATA_OFF,
			"/bound %d.%d.%d.%d:%d [%02x:%02x:%02x:%02x:%02x:%02x]",
			remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3],
			be16(udp->source),
			remote_mac[0], remote_mac[1], remote_mac[2],
			remote_mac[3], remote_mac[4], remote_mac[5]
		       );
	size += (4 - (size % 4));

	buf[DATA_OFF + size] = ',';
	size += 4;

	ip->tot_len = be16(IP_HLEN + UDP_HLEN + size);
	udp->len = be16(UDP_HLEN + size);

	ip->check = 0;
	ip->check = be16(checksum((uint8_t *)ip, IP_HLEN, 0));

	udp->check = 0;

	ksz8851_send_packet(buf, DATA_OFF + size);
}

static void process_periodic_udp(uint8_t * buf)
{
	struct ethhdr * eth;
	struct iphdr * ip;
	struct udphdr * udp;
	uint8_t * data;
	uint16_t size;

	eth = (struct ethhdr *)&buf[0];
	ip = (struct iphdr *)&buf[ETH_HLEN];
	udp = (struct udphdr *)&buf[ETH_HLEN + IP_HLEN];
	data = &buf[ETH_HLEN + IP_HLEN + UDP_HLEN];

	if (!bound && net_idx == 0) {
		memset(data, 0, NETBUF_SZ - DATA_OFF);
		size = snprintf((char *)data, NETBUF_SZ - DATA_OFF,
				"/discover"
			       );
		size += (4 - (size % 4));

		buf[DATA_OFF + size] = ',';
		size += 4;

		build_ethernet(eth, NULL, my_mac);
		build_ip(ip, NULL, my_ip, IPPROTO_UDP, UDP_HLEN + size);
		build_udp(udp, 9999, 9999, size);

		ksz8851_send_packet(buf, DATA_OFF + size);
	} else if (bound && net_idx == 0) {
		int32_t *i;
		memset(data, 0, NETBUF_SZ - DATA_OFF);
		size = snprintf((char *)data, NETBUF_SZ - DATA_OFF,
				"/io"
			       );
		size += (4 - (size % 4));

		memcpy(&buf[DATA_OFF + size], ",\0\0\0", 4);
		buf[DATA_OFF + size + 1] = 'i';
		size += 4;

		i = &buf[DATA_OFF + size];
		*i = be32(12345678);
		size += 4;

		build_ethernet(eth, remote_mac, my_mac);
		build_ip(ip, remote_ip, my_ip, IPPROTO_UDP, UDP_HLEN + size);
		build_udp(udp, 9999, 9999, size);

		ksz8851_send_packet(buf, DATA_OFF + size);
	}
  
	net_idx++;
	net_idx &= 0x1ff;
}

void net_poll(void)
{
	uint16_t len;

	if (ksz8851_has_data()) {
		len = ksz8851_read_packet(buf, NETBUF_SZ);
		if (!len)
			return;
    
		if (!ethernet_checkdest((struct ethhdr *)&buf[0], my_mac))
			return;

		if (get_ethertype((struct ethhdr *)&buf[0]) == be16(ETH_P_ARP)) {

			process_arp(buf, my_mac, my_ip);

		} else if (get_ethertype((struct ethhdr *)&buf[0]) == be16(ETH_P_IP)) {

			if (get_ipproto((struct iphdr *)&buf[ETH_HLEN], my_ip) == IPPROTO_ICMP)
				process_icmp(buf);
			else if (get_ipproto((struct iphdr *)&buf[ETH_HLEN], my_ip) == IPPROTO_UDP)
				process_udp(buf);
		}

	}

	process_periodic_udp(buf);
}
