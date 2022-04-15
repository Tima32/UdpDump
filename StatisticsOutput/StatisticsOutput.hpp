#pragma once
#include <linux/ip.h>  /* for ipv4 header */
#include <linux/udp.h> /* for upd header */
#include <linux/tcp.h>
#include <linux/if_ether.h>

#include <thread>
#include <mutex>
#include <condition_variable>

class StatisticsOutput
{
public:
#pragma pack(push, 1)
	struct Header
	{
		ethhdr eth;
		iphdr ip;
		union
		{
			udphdr udp;
			tcphdr tcp;
		};
	};
#pragma pack(pop)
	
	StatisticsOutput();
	~StatisticsOutput();
	void push(const Header* hd, size_t msg_size);
	Header pop();

	void setPrintStatistics(bool print);

private:
	void print();
	void printToConsole(const uint8_t* msg);

	static constexpr size_t size{2048};
	Header buffer[size];
	size_t pos_read{ 0 }, pos_write{ 0 };
	size_t package_count{ 0 };
	size_t bytes_count{ 0 };
	bool done{ false };
	bool print_to_console{ true };

	std::condition_variable condition;
	std::mutex m;
	std::thread print_thread{&StatisticsOutput::print, this};
};