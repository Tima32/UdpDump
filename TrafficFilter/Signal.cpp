#include <csignal>
#include <iostream>
#include <mqueue.h>

using namespace std;

static void SignalHandlerTerm( int signum )
{
	mq_unlink("/UDP_DUMP_SERVER_1");
	exit(0);
}

void RegisterSignal()
{
	// register signal SIGINT and signal handler  
	signal(SIGINT, SignalHandlerTerm);
}