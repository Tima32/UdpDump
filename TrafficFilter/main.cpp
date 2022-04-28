#include "Sender/Sender.hpp"
#include "Stat/Stat.hpp"
#include "Filter/Filter.hpp"
#include "MqServer/MqServer.hpp"

using namespace std;

void ArgumentParsing(int argc, const char* argv[], Filter& f, Sender& s);

int main(int argc, const char* argv[])
{
	Sender sender;
	Stat stat;
	Filter filter;
	MqServer mq_server;

	sender.setStatisticsBuffer(stat);
	filter.setStatisticsOutput(stat);
	ArgumentParsing(argc, argv, filter, sender);

	sender.run();//does not block the thread.
	mq_server.run();
	filter.run();//block the thread.
	

	return 0;
}