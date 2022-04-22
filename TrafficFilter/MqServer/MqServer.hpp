#include <mqueue.h>

class Sender;

class MqServer
{
public:
	MqServer();

	void run();
	void setSender(const Sender& s);

private:
	void registerClientRequestCallback(bool unlink);
	void openClientMqAndSendStatistics(const char* name);

	mqd_t mqdes{0};
	const Sender* sender{nullptr};
};