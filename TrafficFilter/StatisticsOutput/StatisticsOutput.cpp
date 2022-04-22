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
#include "StatisticsOutput.hpp"

using namespace std;

static string ToIP(uint32_t ipi)
{
	uint8_t* bytes = (uint8_t*)&ipi;

	stringstream ss;
	ss << uint16_t(bytes[0]) << "." << uint16_t(bytes[1]) << "." << uint16_t(bytes[2]) << "." << uint16_t(bytes[3]);
	return ss.str();
}

static void RegisterClientRequestCallback(bool unlink = true);
static string last_statistics;
static void OpenClientMqAndSendStatistics(const char* name)
{
	struct mq_attr attr;
	memset(&attr, 0, sizeof attr);
	attr.mq_msgsize = 8192;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;

	mqd_t msq_id = mq_open(name, O_RDWR | O_CREAT | O_NONBLOCK,
                         0777, &attr);
	if(msq_id == (mqd_t) -1)
	{
		cout << "Error on msg Q creation: " << strerror(errno) << endl;
		exit(1);
	}

	if(mq_send(msq_id, last_statistics.c_str(), strlen(last_statistics.c_str()), 0) < 0)
	{
		if(errno != EAGAIN)
			cout << "Error on sending msg on MsgQ " << strerror(errno);
	}
	mq_close(msq_id);
}

static void ClientRequestCallback(union sigval sv)
{
	struct mq_attr attr;
    ssize_t nr;
    char *buf;
    mqd_t mqdes = *((mqd_t *) sv.sival_ptr);

    /* Determine max. msg size; allocate buffer to receive msg */

    if (mq_getattr(mqdes, &attr) == -1) {
        fprintf(stderr, "mq_getattr: %s\n", strerror(errno));
	}

    buf = (char*)malloc(attr.mq_msgsize);
    if (buf == NULL)
       cout << "malloc" << endl;

    nr = mq_receive(mqdes, buf, attr.mq_msgsize, NULL);
    if (nr == -1)
       cout << "mq_receive" << endl;

	buf[nr] = 0;
	OpenClientMqAndSendStatistics(buf);


    free(buf);

	//Требуется перерегистрация калбека
	RegisterClientRequestCallback(false);
}
static mqd_t mqdes{0};
static void RegisterClientRequestCallback(bool unlink)
{
	sigevent sev{0};
	struct mq_attr attr = 
    {
        .mq_maxmsg = 1,
        .mq_msgsize = 256
    };

	if (unlink)
	{
		mq_unlink(MSG_Q_NAME);
		mqdes = mq_open(MSG_Q_NAME, O_RDONLY| O_CREAT | O_EXCL, S_IRWXU, &attr);
	}

	if (mqdes == (mqd_t)-1)
	{
		cout << "Error mq_open" << endl;
		perror("mq_open");
		exit(1);
	}

	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = ClientRequestCallback;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = &mqdes;
	if (mq_notify(mqdes, &sev) == -1)
	{
		cout << "Error mq_notify" << endl;
		exit(1);
	}
}

StatisticsOutput::StatisticsOutput()
{
	RegisterClientRequestCallback();
}
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
			last_statistics = ss.str();
		}
	}
}
void StatisticsOutput::printToConsole(const string& str)
{
	if (!print_to_console)
		return;
	cout << str;
}