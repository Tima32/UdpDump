#pragma once
#include <linux/ip.h>  /* for ipv4 header */
#include <linux/udp.h> /* for upd header */
#include <linux/tcp.h>
#include <linux/if_ether.h>

//mq
#include <mqueue.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <tuple>

class Stat
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
	
	Stat();
	~Stat();
	void push(const Header* hd, size_t msg_size);
	std::tuple<Header, size_t, size_t> pop();

private:

	static constexpr size_t size{2048};
	Header buffer[size];
	size_t pos_read{ 0 }, pos_write{ 0 };
	size_t package_count{ 0 };
	size_t bytes_count{ 0 };

	std::condition_variable condition;
	std::mutex m;

	mqd_t mqdes{0};
};