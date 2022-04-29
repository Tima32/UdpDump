#include <arpa/inet.h> //ntohs

//mq
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#define MSG_Q_NAME "/UDP_DUMP_SERVER_1"

#include <iostream>
#include <sstream>
#include <iomanip>
#include "Stat.hpp"

using namespace std;

Stat::Stat(){}
Stat::~Stat(){}

void Stat::push(const Header* hd, size_t msg_size)
{
	m.lock();

	package_count++;
	bytes_count += msg_size;

	pos_write++;
	buffer[pos_write % size] = *hd;
	buffer_package_count[pos_write % size] = package_count;
	buffer_bytes_count[pos_write % size] = bytes_count;
	condition.notify_one();

	m.unlock();
}
std::tuple<Stat::Header, size_t, size_t> Stat::pop()
{
    std::unique_lock<std::mutex> locker(m);

	if (pos_read == pos_write)
		condition.wait(locker);

	pos_read++;
	return {buffer[pos_read % size], buffer_package_count[pos_read % size], buffer_bytes_count[pos_read % size]};
}