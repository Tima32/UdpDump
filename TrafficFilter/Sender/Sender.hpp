#include <thread>
#include <mutex>

#include "../Stat/Stat.hpp"
#include "../MqServer/MqServer.hpp"

class Sender
{
public:
	~Sender();

	void setStatisticsBuffer(Stat& s);
	void run();
	void allowPrintStatistics(bool print);

private:
	void print();
	void printToConsole(const std::string& str);
	std::string ToIP(uint32_t ipi);

	volatile bool print_to_console{ true };

	bool done { false };
	std::thread print_thread;
	Stat* statistics{ nullptr };

	mutable std::mutex last_statistics_m;
	std::string last_statistics;
	friend MqServer;
};