#include "Sender/Sender.hpp"
#include "Stat/Stat.hpp"
#include "Filter/Filter.hpp"
#include "MqServer/MqServer.hpp"

using namespace std;

void ArgumentParsing(int argc, const char* argv[], Filter& f, Sender& s);
void RegisterSignal();

int main(int argc, const char* argv[])
{
	RegisterSignal();

	Sender sender;
	Stat stat;
	Filter filter;
	MqServer mq_server;

	sender.setStatisticsBuffer(stat);
	filter.setStatisticsOutput(stat);
	ArgumentParsing(argc, argv, filter, sender);

	sender.run();//does not block the thread.
	mq_server.run();//does not block the thread.
	filter.run();//block the thread.
	

	return 0;
}