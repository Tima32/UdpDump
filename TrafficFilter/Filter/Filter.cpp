#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>

#include <linux/ip.h>  /* for ipv4 header */
#include <linux/udp.h> /* for upd header */
#include <linux/tcp.h>

#include <iostream>

#include "Filter.hpp"

using namespace std;

static constexpr size_t msg_size { 65535 };
static constexpr size_t header_size { sizeof(ethhdr) + sizeof(iphdr) + sizeof(udphdr) };

//public
Filter::Filter(){};
Filter::~Filter(){};

void Filter::run()
{
    if (interface.size())
	{
		if (!SetPromisc(interface.c_str(), true))
		{
			cerr << "Error active promisc mode." << endl;
			exit(1);
		}
	}
	else 
	{
		if (!SetPromiscAll(true))
		{
			cerr << "Error active promisc mode." << endl;
			exit(1);
		}
	}

    int raw_socket;

	int retval = 0; 

	char msg[msg_size];
	ssize_t msglen;

	if ((raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("socket");
		return;
	}

	if (interface.size() &&
		setsockopt(raw_socket, SOL_SOCKET, SO_BINDTODEVICE, interface.data(), interface.size()))
	{
		perror("setsockopt");
		retval = 1;
		goto _go_close_socket;
	}
		

	while(1)
	{
		if ((msglen = recv(raw_socket, msg, msg_size, 0)) == -1) {
			perror("recv");
			retval = 1;
			goto _go_close_socket;
		}

		if (msglen <= header_size)
			cerr << "Bed package: wrong header seize." << endl;
		else {
			ethhdr* eth = (ethhdr*)        msg;
			iphdr*  ip =  (struct iphdr*) (msg + sizeof(ethhdr));
			udphdr* udp = (udphdr*)       (msg + sizeof(ethhdr) + sizeof(iphdr));
			tcphdr* tcp = (tcphdr*)       (msg + sizeof(ethhdr) + sizeof(iphdr));
			

			// filters
			if (!SourceIpFilter(ip->saddr))
				continue;
			if (!DestIpFilter(ip->daddr))
				continue;

			if (!SourcePortFilter(udp->source))
				continue;
			if (!DestPortFilter(udp->dest))
				continue;

			// For TCP and UDP, the first fields are ports.
			if (!SourceMacFilter(eth->h_source))
				continue;
			if (!DestMacFilter(eth->h_dest))
				continue;

			if (!ProtocolsFilter(ip->protocol))
				continue;

			so.push(reinterpret_cast<StatisticsOutput::Header*>(msg), msglen);
		}
	}

_go_close_socket:
	close(raw_socket);

	return;
    }

//private
bool Filter::SourceIpFilter(uint32_t ip)
{
	if (sources_ip_filter.size() == 0)
		return true;

	for (auto f : sources_ip_filter)
	{
		if (f == ip)
			return true;
	}

	return false;
}
bool Filter::DestIpFilter(uint32_t ip)
{
	if (dest_ip_filter.size() == 0)
		return true;

	for (auto f : dest_ip_filter)
	{
		if (f == ip)
			return true;
	}

	return false;
}
bool Filter::SourcePortFilter(uint16_t port)
{
	if (sources_port_filter.size() == 0)
		return true;

	for (auto f : sources_port_filter)
	{
		if (f == port)
			return true;
	}

	return false;
}
bool Filter::DestPortFilter(uint16_t port)
{
	if (dest_port_filter.size() == 0)
		return true;

	for (auto f : dest_port_filter)
	{
		if (f == port)
			return true;
	}

	return false;
}
bool Filter::SourceMacFilter(uint8_t h_source[ETH_ALEN])
{
	if (sources_mac_filter.size() == 0)
		return true;

	for (auto f : sources_mac_filter)
	{
		bool compare{ true };
		for (size_t i = 0; i < ETH_ALEN; ++i)
		{
			if (h_source[i] != f.arr[i])
			{
				compare = false;
				break;
			}
		}
		if (compare)
			return true;
		
	}

	return false;
}
bool Filter::DestMacFilter(uint8_t h_source[ETH_ALEN])
{
	if (dest_mac_filter.size() == 0)
		return true;

	for (auto f : dest_mac_filter)
	{
		bool compare{ true };
		for (size_t i = 0; i < ETH_ALEN; ++i)
		{
			if (h_source[i] != f.arr[i])
			{
				compare = false;
				break;
			}
		}
		if (compare)
			return true;

	}

	return false;
}
bool Filter::ProtocolsFilter(uint8_t proto)
{
	if (protocols_filter.size() == 0)
		return true;

	for (auto f : protocols_filter)
	{
		if (f == proto)
			return true;
	}

	return false;
}

bool Filter::SetPromisc(const char* ifname, bool enable)
{
	packet_mreq mreq = { 0 };
	int sfd;
	int action;

	if ((sfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("unable to open socket");
		return false;
	}

	mreq.mr_ifindex = if_nametoindex(ifname);
	mreq.mr_type = PACKET_MR_PROMISC;

	if (mreq.mr_ifindex == 0) {
		perror("unable to get interface index");
		return false;
	}

	if (enable)
		action = PACKET_ADD_MEMBERSHIP;
	else
		action = PACKET_DROP_MEMBERSHIP;

	if (setsockopt(sfd, SOL_PACKET, action, &mreq, sizeof(mreq)) != 0) {
		perror("unable to enter promiscouous mode");
		return false;
	}

	close(sfd);
	return true;
}
bool Filter::SetPromiscAll(bool enable)
{
	struct ifaddrs* addrs, * tmp;

	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET)
		{
			bool status = SetPromisc(tmp->ifa_name, enable);
			if (status == false && enable == true)
			{
				SetPromiscAll(false);
				return false;
			}
		}

		tmp = tmp->ifa_next;
	}

	freeifaddrs(addrs);
	return true;
}