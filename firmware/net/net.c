#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "net.h"
#include "../ksz8851snl.h"

/* common functions */

uint16_t checksum(uint8_t *buf, uint16_t len, uint8_t type)
{
	uint32_t sum = type;

	/* build the sum of 16bit words */
	while (len > 1){
		sum += 0xFFFF & (*buf << 8 | *(buf + 1));
		buf += 2;
		len -= 2;
	}
	/* if there is a byte left then add it (padded with zero) */
	if (len) {
		sum += (0xFF & *buf) << 8;
	}
	/* now calculate the sum over the bytes in the sum */
	/* until the result is only 16bit long */
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	/* build 1's complement: */
	return ((uint16_t) sum ^ 0xFFFF);
}

uint16_t be16(uint16_t x)
{
	return (x >> 8) | ((x & 0xff) << 8);
}

uint32_t be32(uint32_t x)
{
	return ( ((x & 0xff000000) >> 24) |
		 ((x & 0x00ff0000) >> 8)  |
		 ((x & 0x0000ff00) << 8)  |
		 ((x & 0x000000ff) << 24) );
}

/* Ehernet layer */

void build_ethernet(struct ethhdr * hdr, uint8_t * dst, uint8_t * src)
{
	if (dst == NULL)
		memset(&hdr->h_dest, 0xff, ETH_ALEN);
	else
		memcpy(&hdr->h_dest, dst, ETH_ALEN);

	memcpy(&hdr->h_source, src, ETH_ALEN);

	hdr->h_proto = be16(ETH_P_IP);
}

void reply_ethernet(struct ethhdr * hdr, uint8_t * src)
{
	memcpy(&hdr->h_dest, &hdr->h_source, ETH_ALEN);
	memcpy(&hdr->h_source, src, ETH_ALEN);
}

void swap_ethernet(struct ethhdr * hdr)
{
	uint8_t tmp[ETH_ALEN];
	memcpy(tmp, &hdr->h_source, ETH_ALEN);
	memcpy(&hdr->h_source, &hdr->h_dest, ETH_ALEN);
	memcpy(&hdr->h_dest, tmp, ETH_ALEN);
}

uint8_t ethernet_checkdest(struct ethhdr * hdr, uint8_t * mac)
{
	return 1;
}

uint16_t get_ethertype(struct ethhdr * hdr)
{
	return hdr->h_proto;
}

/* ARP layer */

void process_arp(uint8_t * buf, uint8_t * mac, uint8_t * ip)
{
	struct arphdr * arp;

	arp = (struct arphdr *)&buf[ETH_HLEN];

	/* check arp request is correct */

	if (arp->ar_hrd != be16(ARPHRD_ETHER))
		return;

	if (arp->ar_pro != be16(ETH_P_IP))
		return;

	if (arp->ar_hln != 6)
		return;

	if (arp->ar_pln != 4)
		return;

	if (arp->ar_op != be16(ARPOP_REQUEST))
		return;

	if (memcmp(arp->ar_tip, ip, 4) != 0)
		return;

	/* send a reply */

	reply_ethernet((struct ethhdr *)&buf[0], mac);

	arp->ar_op = be16(ARPOP_REPLY);

	memcpy(&arp->ar_tip, &arp->ar_sip, 4);
	memcpy(&arp->ar_tha, &arp->ar_sha, ETH_ALEN);

	memcpy(&arp->ar_sip, ip, 4);
	memcpy(&arp->ar_sha, mac, ETH_ALEN);

	ksz8851_send_packet(buf, ETH_HLEN + ARP_HLEN);
}

/* IP layer */

void build_ip(struct iphdr * hdr, uint8_t * dst, uint8_t * src,
	       uint8_t protocol, uint16_t len)
{
	hdr->version = IPVERSION;
	hdr->ihl = IP_HLEN/4;
	hdr->frag_off = 0x40;
	hdr->ttl = IPDEFTTL;
	hdr->check = 0;
	hdr->tot_len = be16(IP_HLEN + len);
	hdr->protocol = protocol;

	if (dst == NULL)
		memset(&hdr->daddr, 0xff, 4);
	else
		memcpy(&hdr->daddr, dst, 4);

	memcpy(&hdr->saddr, src, 4);
	hdr->check = be16(checksum((uint8_t *)hdr, IP_HLEN, 0));
}

uint8_t get_ipproto(struct iphdr * hdr, uint8_t * ip)
{
	if (hdr->version != IPVERSION)
		return -1;

	if (hdr->ihl != IP_HLEN/4)
		return -1;

	if (checksum((uint8_t *)hdr, IP_HLEN, 0) != 0)
		return -1;

	if (memcmp(&hdr->daddr, ip, 4) != 0)
		return -1;

	return hdr->protocol;
}

void swap_ip(struct iphdr * hdr)
{
	uint8_t tmp[4];

	memcpy(tmp, &hdr->saddr, 4);
	memcpy(&hdr->saddr, &hdr->daddr, 4);
	memcpy(&hdr->daddr, tmp, 4);
}

/* ICMP layer */

void process_icmp(uint8_t * buf)
{
	struct ethhdr * eth;
	struct iphdr * ip;
	struct icmphdr * icmp;

	eth = (struct ethhdr *)&buf[0];
	ip = (struct iphdr *)&buf[ETH_HLEN];
	icmp = (struct icmphdr *)&buf[ETH_HLEN + IP_HLEN];

	if (icmp->type != ICMP_ECHO)
		return;

	swap_ethernet(eth);
	swap_ip(ip);

	ip->ttl = IPDEFTTL;

	ip->check = 0;
	ip->check = be16(checksum((uint8_t *)ip, IP_HLEN, 0));

	icmp->type = ICMP_ECHOREPLY;

	icmp->checksum = 0;
	icmp->checksum = be16(checksum((uint8_t *)icmp,
				       be16(ip->tot_len) - IP_HLEN, 0));

	ksz8851_send_packet(buf, ETH_HLEN + be16(ip->tot_len));
}

/* UDP layer */

void build_udp(struct udphdr * hdr, uint16_t dst, uint16_t src, uint16_t len)
{
	hdr->source = be16(src);
	hdr->dest = be16(dst);
	hdr->len = be16(UDP_HLEN + len);
	hdr->check = 0;
	/* hdr->check = be16(checksum((uint8_t *)hdr,
	   UDP_HLEN + len, IPPROTO_UDP)); */
}

void swap_udp(struct udphdr * hdr)
{
	uint8_t tmp[2];

	memcpy(tmp, &hdr->source, 2);
	memcpy(&hdr->source, &hdr->dest, 2);
	memcpy(&hdr->dest, tmp, 2);
}
