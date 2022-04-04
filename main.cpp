#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/ip.h>  /* for ipv4 header */
#include <linux/udp.h> /* for upd header */

#include <iostream>
#include <sstream>
#include <limits>
#include "ArgumentParserLib/ArgumentParser.hpp"

using namespace std;

#define MSG_SIZE 65535
#define HEADER_SIZE (sizeof(struct iphdr) + sizeof(struct udphdr))

size_t package_count{0}, bytes_count{0};

vector<uint32_t> sources_ip_filter;
vector<uint32_t> dest_ip_filter;
vector<uint16_t> source_port_filter;
vector<uint16_t> dest_port_filter;
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
	if (source_port_filter.size() == 0)
		return true;

	for (auto f : source_port_filter)
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

void Dump() {
	int raw_socket;
	//sockaddr_in sockstr;
	socklen_t socklen{};

	int retval = 0; 

	char msg[MSG_SIZE];
	ssize_t msglen;

	if ((raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
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
			iphdr* th = (struct iphdr*)msg;
			udphdr* udp = (udphdr*) (msg + sizeof(iphdr));

			// filters
			if (!SourceIpFilter(th->saddr))
				continue;
			if (!DestIpFilter(th->daddr))
				continue;
			if (!SourcePortFilter(udp->source))
				continue;
			if (!DestPortFilter(udp->dest))
				continue;

			// increase counters
			package_count++;
			bytes_count += msglen;

			cout << "Packages received: " << package_count << " bytes received: " << bytes_count;

			cout << " proto: " << uint32_t(th->protocol) << 
				" ip source: " << ToIP(th->saddr) << 
				" ip dest: " << ToIP(th->daddr) <<
				" len: " << ntohs(th->tot_len) <<
				" msgl: " << msglen;

			cout << " port source: " << ntohs(udp->source) << " port dest: " << ntohs(udp->dest) << endl;
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
			source_port_filter.push_back(ntohs(port));

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