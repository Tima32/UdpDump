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
#include <iomanip>
#include <sstream>
#include <limits>
#include <algorithm>
#include <thread>
#include "ArgumentParserLib/ArgumentParser.hpp"
#include "StatisticsOutput/StatisticsOutput.hpp"

using namespace std;

#define MSG_SIZE 65535
#define HEADER_SIZE (sizeof(struct iphdr) + sizeof(struct udphdr))

StatisticsOutput so;

vector<uint32_t> sources_ip_filter;
vector<uint32_t> dest_ip_filter;

vector<uint16_t> sources_port_filter;
vector<uint16_t> dest_port_filter;

struct mac
{
	unsigned char arr[ETH_ALEN];
};
vector<mac> sources_mac_filter;
vector<mac> dest_mac_filter;

string interface;
vector<uint8_t> protocols_filter;

bool SourceIpFilter(uint32_t ip)
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
bool DestIpFilter(uint32_t ip)
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
bool SourcePortFilter(uint16_t port)
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
bool DestPortFilter(uint16_t port)
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
bool SourceMacFilter(uint8_t h_source[ETH_ALEN])
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
bool DestMacFilter(uint8_t h_source[ETH_ALEN])
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
bool ProtocolsFilter(uint8_t proto)
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

bool SetPromisc(const char* ifname, bool enable)
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
bool SetPromiscAll(bool enable)
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

void PrintHelp()
{
	constexpr char const* str{
		"Usage: dump [options]\n"
		"Options:\n"
		"	--interface <arg>	  Sellect interface.\n"
		"	--src-ip    <arg>	  Sets the source ip.        (multiple)\n"
		"	--dst-ip    <arg>	  Sets the destination ip.   (multiple)\n"
		"	--src-port  <arg>	  Sets the source port.      (multiple)\n"
		"	--dst-port  <arg>	  Sets the destination port. (multiple)\n"
		"	--src-mac   <arg>	  Sets the source mac.       (multiple)\n"
		"	--dst-mac   <arg>	  Sets the destination mac.  (multiple)\n"
		"   -t                    TCP filter.\n"
		"   -u                    UDP filter.\n"
		"   --no-print-statistics Disables output of statistics to the console.\n"
	};
	cout << str;
}
void ArgumentParsing(int argc, const char* argv[])
{
	ArgumentParser ap(argc, argv);

	//help
	auto help_param_pos = ap.find("--help");
	auto help_param_short_pos = ap.find("-h");
	if (help_param_pos != -1 || help_param_short_pos != -1)
	{
		PrintHelp();
		exit(0);
	}

	try
	{
		// --src-ip
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--src-ip", i);
			if (i == -1)
				break;

			const auto& ip_s = ap.at(i + 1);

			in_addr_t a = inet_addr(ip_s.c_str());
			if (a == INADDR_NONE)
			{
				cerr << "Error: bed ip in param " << i + 1 << endl;
				exit(1);
			}
			sources_ip_filter.push_back(a);

			cout << "Set filter source ip: " << ip_s << endl;
		}

		// --dest-ip
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--dest-ip", i);
			if (i == -1)
				break;

			const auto& ip_s = ap.at(i + 1);

			in_addr_t a = inet_addr(ip_s.c_str());
			if (a == INADDR_NONE)
			{
				cerr << "Error: bed ip in param " << i + 1 << endl;
				exit(1);
			}
			dest_ip_filter.push_back(a);

			cout << "Set filter destination ip: " << ip_s << endl;
		}

		// --src-port
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--src-port", i);
			if (i == -1)
				break;

			const auto& port_s = ap.at(i + 1);

			stringstream ss;
			ss << port_s;
			uint16_t port;
			ss >> port;
			sources_port_filter.push_back(ntohs(port));

			cout << "Set filter source port: " << port_s << endl;
		}

		// --dest-port
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--dest-port", i);
			if (i == -1)
				break;

			const auto& port_s = ap.at(i + 1);

			stringstream ss;
			ss << port_s;
			uint16_t port;
			ss >> port;
			dest_port_filter.push_back(ntohs(port));

			cout << "Set filter destination port: " << port_s << endl;
		}

		// --src-mac
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--src-mac", i);
			if (i == -1)
				break;

			auto mac_s = ap.at(i + 1);
			std::replace(mac_s.begin(), mac_s.end(), ':', ' ');
			cout << "mac: " << mac_s << endl;

			stringstream ss;
			ss.str(mac_s);
			ss >> std::hex;
			mac m;
			for (size_t i = 0; i < ETH_ALEN; ++i)
			{
				uint16_t byte{};
				ss >> byte;
				m.arr[i] = uint8_t(byte);
			}
			sources_mac_filter.push_back(m);
		}

		// --dest-mac
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--dest-mac", i);
			if (i == -1)
				break;

			auto mac_s = ap.at(i + 1);
			std::replace(mac_s.begin(), mac_s.end(), ':', ' ');
			cout << "mac: " << mac_s << endl;

			stringstream ss;
			ss.str(mac_s);
			ss >> std::hex;
			mac m;
			for (size_t i = 0; i < ETH_ALEN; ++i)
			{
				uint16_t byte{};
				ss >> byte;
				m.arr[i] = uint8_t(byte);
			}
			dest_mac_filter.push_back(m);
		}

		// --interface
		interface = ap.get<string>("--interface", "");
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
		
		// -t
		auto tcp = ap.find("-t");
		if (tcp != -1)
			protocols_filter.push_back(6);

		// -u
		auto udp = ap.find("-u");
		if (udp != -1)
			protocols_filter.push_back(17);

		// -no-print-statistics
		auto nps = ap.find("-no-print-statistics");
		if (nps != -1)
			so.setPrintStatistics(false);

	}
	catch (const std::out_of_range& e)
	{
		std::cerr << "Error: no command argument" << '\n';
		exit(1);
	}
	catch(const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		exit(1);
	}
}

void Dump() {
	int raw_socket;

	int retval = 0; 

	char msg[MSG_SIZE];
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
		if ((msglen = recv(raw_socket, msg, MSG_SIZE, 0)) == -1) {
			perror("recv");
			retval = 1;
			goto _go_close_socket;
		}

		if (msglen <= HEADER_SIZE)
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

int main(int argc, const char* argv[])
{
	ArgumentParsing(argc, argv);
	Dump();

	return 0;
}