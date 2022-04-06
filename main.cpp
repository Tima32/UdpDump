#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
//#include <linux/if_packet.h>
#include <unistd.h>

#include <linux/ip.h>  /* for ipv4 header */
#include <linux/udp.h> /* for upd header */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <algorithm>
#include "ArgumentParserLib/ArgumentParser.hpp"

using namespace std;

#define MSG_SIZE 65535
#define HEADER_SIZE (sizeof(struct iphdr) + sizeof(struct udphdr))

size_t package_count{0}, bytes_count{0};

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

string ToIP(uint32_t ipi)
{
	uint8_t* bytes = (uint8_t*)&ipi;

	stringstream ss;
	ss << uint16_t(bytes[0]) << "." << uint16_t(bytes[1]) << "." << uint16_t(bytes[2]) << "." << uint16_t(bytes[3]);
	return ss.str();
}

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

void Dump() {
	int raw_socket;
	//sockaddr_in sockstr;
	socklen_t socklen{};

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

			// filters
			if (!SourceIpFilter(ip->saddr))
				continue;
			if (!DestIpFilter(ip->daddr))
				continue;

			if (!SourcePortFilter(udp->source))
				continue;
			if (!DestPortFilter(udp->dest))
				continue;

			if (!SourceMacFilter(eth->h_source))
				continue;
			if (!DestMacFilter(eth->h_dest))
				continue;

			// increase counters
			package_count++;
			bytes_count += msglen;

			cout << "Packages received: " << package_count << " bytes received: " << bytes_count << '\t';

			printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x -> ",
				eth->h_source[0], eth->h_source[1], eth->h_source[2],
				eth->h_source[3], eth->h_source[4], eth->h_source[5]);

			printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\t",
				eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
				eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);

			cout << "\tproto: " << uint32_t(ip->protocol) << 
				"\tip s: " << setw(15) << ToIP(ip->saddr) <<
				"\tip d: " << setw(15) << ToIP(ip->daddr) <<
				"\tip l: " << setw(5) << ntohs(ip->tot_len) <<
				"\tmsgl: " << setw(5) << msglen;

			cout << "\tport s: " << setw(5) << ntohs(udp->source) << "\tport d: " << setw(5) << ntohs(udp->dest) << endl;
		}
	}

_go_close_socket:
	close(raw_socket);

	return;
}

void PrintHelp()
{
	constexpr char const* str{
		"Usage: dump [options]\n"
		"Options:\n"
		"	--interface <arg>    Sellect interface.\n"
		"	--src-ip <arg>       Sets the source ip.        (multiple)\n"
		"	--dest-ip <arg>      Sets the destination ip.   (multiple)\n"
		"	--src-port <arg>     Sets the source port.      (multiple)\n"
		"	--dest-port <arg>    Sets the destination port. (multiple)\n"
		"	--src-mac <arg>      Sets the source mac.       (multiple)\n"
		"	--dest-mac <arg>     Sets the destination mac.  (multiple)\n"
	};
	cout << str;
}
int main(int argc, const char* argv[])
{
	ArgumentParser ap(argc, argv);

	//help
	auto help_param_pos = ap.find("--help");
	auto help_param_short_pos = ap.find("-h");
	if (help_param_pos != -1 || help_param_short_pos != -1)
	{
		PrintHelp();
		return 0;
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
				return 1;
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
				return 1;
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

		interface = ap.get<string>("--interface", "");
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
	
	Dump();

	return 0;
}