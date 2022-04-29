#include <mqueue.h>

class Sender;

class MqServer
{
public:
	MqServer();
	~MqServer();

	void run();

private:
	void registerClientRequestCallback(bool unlink);
	void openClientMqAndSendStatistics(const char* name);
	static void ClientRequestCallback(union sigval sv);

	mqd_t mqdes{0};

	static MqServer* ms;
};