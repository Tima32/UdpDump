#include "Sender/Sender.hpp"
#include "Stat/Stat.hpp"
#include "Filter/Filter.hpp"

using namespace std;

void ArgumentParsing(int argc, const char* argv[], Filter& f, Sender& s);

int main(int argc, const char* argv[])
{
	Sender sender;
	Stat stat;
	Filter filter;

	sender.setStatisticsBuffer(stat);
	ArgumentParsing(argc, argv, filter, sender);

	sender.run();//does not block the thread.
	filter.run();//block the thread.
	

	return 0;
}