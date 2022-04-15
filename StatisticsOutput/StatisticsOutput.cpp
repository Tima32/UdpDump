#include <arpa/inet.h> //ntohs
#include <iostream>
#include <sstream>
#include <iomanip>
#include "StatisticsOutput.hpp"

using namespace std;

static string ToIP(uint32_t ipi)
{
	uint8_t* bytes = (uint8_t*)&ipi;

	stringstream ss;
	ss << uint16_t(bytes[0]) << "." << uint16_t(bytes[1]) << "." << uint16_t(bytes[2]) << "." << uint16_t(bytes[3]);
	return ss.str();
}

StatisticsOutput::StatisticsOutput()
{}
StatisticsOutput::~StatisticsOutput()
{
	done = true;
	print_thread.join();
}

void StatisticsOutput::push(const Header* hd, size_t msg_size)
{
	m.lock();

	package_count++;
	bytes_count += msg_size;

	pos_write++;
	buffer[pos_write % size] = *hd;
	condition.notify_one();

	m.unlock();
}
StatisticsOutput::Header StatisticsOutput::pop()
{
	pos_read++;
	return buffer[pos_read % size];
}

void StatisticsOutput::setPrintStatistics(bool print)
{
	print_to_console = print;
}

//private
void StatisticsOutput::print()
{
	while (!done)
	{
		std::unique_lock<std::mutex> locker(m);
		condition.wait(locker);

		while ((pos_read % size) != (pos_write % size))
		{
			auto h = pop();
			const uint8_t* msg = reinterpret_cast<uint8_t*>(&h);

			printToConsole(msg);
		}
	}
}
void StatisticsOutput::printToConsole(const uint8_t* msg)
{
	if (!print_to_console)
		return;

	ethhdr* eth = (ethhdr*)msg;
	iphdr* ip = (struct iphdr*)(msg + sizeof(ethhdr));
	udphdr* udp = (udphdr*)(msg + sizeof(ethhdr) + sizeof(iphdr));
	tcphdr* tcp = (tcphdr*)(msg + sizeof(ethhdr) + sizeof(iphdr));

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
		"\tip l: " << setw(5) << ntohs(ip->tot_len);

	cout << "\tport s: " << setw(5) << ntohs(udp->source) << "\tport d: " << setw(5) << ntohs(udp->dest) << endl;
}