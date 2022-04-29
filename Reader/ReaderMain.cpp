#include <iostream>
#include <mqueue.h>
#include <string.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h> 

using namespace std;

string server_adr{ "/UDP_DUMP_SERVER_1" };

string client_adr{ "/MY_MSGQ_" };
mqd_t msq_id{ 0 };
mq_attr attr;
char* read_buf { nullptr };

void SignalHandlerTerm( int signum )
{
	mq_unlink(client_adr.c_str());
	exit(0);
}
void RegisterSignal()
{
	// register signal SIGINT and signal handler  
	signal(SIGINT, SignalHandlerTerm);
}

void ConfigureReceiveChanel()
{
	msq_id = mq_open(client_adr.c_str(), O_RDONLY | O_NONBLOCK | O_CREAT, 0777, 0);
	if(msq_id == (mqd_t) -1)
	{
		cout << "Error on msg Q creation: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}

	// So emptying the Q to recv new messages
	ssize_t n = 0;
	if(mq_getattr(msq_id, &attr) < 0)
	{
		cout << "Error in mq_getattr " << strerror(errno)	<< endl;
		exit(EXIT_FAILURE);
	}
	read_buf = new char[attr.mq_msgsize];
}
void ReceiveStatisticsAndPrintThread()
{
	while(1)
	{
		memset(read_buf, 0, attr.mq_msgsize);
		if(auto s = mq_receive(msq_id, read_buf, attr.mq_msgsize, 0) >= 0)
			cout << read_buf;
	}
}
void SendRequestToServer()
{
	struct mq_attr attr;
	memset(&attr, 0, sizeof attr);
	attr.mq_msgsize = 8192;
	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;

	mqd_t msq_id = mq_open(server_adr.c_str(), O_RDWR | O_NONBLOCK, 0777, &attr);
	if(msq_id == (mqd_t) -1)
	{
		cout << "Error on msg Q creation: " << strerror(errno) << endl;
		exit(1);
	}

	while(1)
	{
		stringstream s;
		s << client_adr;

		if(mq_send(msq_id, s.str().c_str(), strlen(s.str().c_str()), 0) < 0)
		{
			if(errno != EAGAIN)
			{
				cout << "Error on sending msg on MsgQ " << strerror(errno);
				mq_close(msq_id);
				exit(1);
			}
		}

		sleep(1); // Easily see the received message in reader
	}

	mq_close(msq_id);
}

int main()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	client_adr += to_string(millis);
	RegisterSignal();

	ConfigureReceiveChanel();
	thread read_t{&ReceiveStatisticsAndPrintThread};

	SendRequestToServer();

	read_t.join();
	mq_close(msq_id);
}