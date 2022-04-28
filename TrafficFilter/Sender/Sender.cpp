#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include "Sender.hpp"
#include "../MqServer/MqServer.hpp"

using namespace std;

Sender::~Sender()
{
	done = false;
	print_thread.join();
}

void Sender::setStatisticsBuffer(Stat& s)
{
	statistics = &s;
}
void Sender::run()
{
	print_thread = thread(&Sender::print, this);
}
void Sender::allowPrintStatistics(bool print)
{
	print_to_console = print;
}

//private
void Sender::print()
{
	while (!done)
	{
		const auto stat = statistics->pop();
		Stat::Header h;
		size_t package_count, bytes_count;
		std::tie(h, package_count, bytes_count) = stat;

		const uint8_t* msg = reinterpret_cast<uint8_t*>(&h);

		stringstream ss;
			
		ethhdr* eth = (ethhdr*)msg;
		iphdr* ip = (struct iphdr*)(msg + sizeof(ethhdr));
		udphdr* udp = (udphdr*)(msg + sizeof(ethhdr) + sizeof(iphdr));
		tcphdr* tcp = (tcphdr*)(msg + sizeof(ethhdr) + sizeof(iphdr));

		ss << "Packages received: " << package_count << " bytes received: " << bytes_count << '\t';

		ss << hex;
		for (size_t i = 0; i < 6; ++i)
		{
			ss << setfill('0') << setw(2) << uint16_t(eth->h_source[i]);
			if (i != 5)
				ss << ':';
		}
		ss << " -> ";
		for (size_t i = 0; i < 6; ++i)
		{
			ss << setfill('0') << setw(2) << uint16_t(eth->h_dest[i]);
			if (i != 5)
				ss << ':';
		}
		ss << dec;

		ss << "\tproto: " << uint32_t(ip->protocol) <<
			setfill(' ') <<
			"\tip s: " << setw(15) << ToIP(ip->saddr) <<
			"\tip d: " << setw(15) << ToIP(ip->daddr) <<
			setfill('0') <<
			"\tip l: " << setw(5) << ntohs(ip->tot_len);

		ss << "\tport s: " << setw(5) << ntohs(udp->source) << "\tport d: " << setw(5) << ntohs(udp->dest) << endl;

		printToConsole(ss.str());
		last_statistics_m.lock();
		last_statistics = ss.str();
		last_statistics_m.unlock();
	}
}
void Sender::printToConsole(const std::string& str)
{
	if (!print_to_console)
		return;
	cout << str;
}
string Sender::ToIP(uint32_t ipi)
{
	uint8_t* bytes = (uint8_t*)&ipi;

	stringstream ss;
	ss << uint16_t(bytes[0]) << "." << uint16_t(bytes[1]) << "." << uint16_t(bytes[2]) << "." << uint16_t(bytes[3]);
	return ss.str();
}

std::mutex Sender::last_statistics_m;
std::string Sender::last_statistics;