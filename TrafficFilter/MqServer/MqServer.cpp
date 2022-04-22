#include <signal.h>
#include <iostream>
#include "MqServer.hpp"

using namespace std;

#define MSG_Q_NAME "/UDP_DUMP_SERVER_1"

static MqServer* ms{nullptr};
MqServer::MqServer()
{
    ms = this;
}
void MqServer::run()
{
    registerClientRequestCallback(true);
}
void MqServer::setSender(const Sender& s)
{
    sender = &s;
}

//private
void MqServer::registerClientRequestCallback(bool unlink)
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
void ClientRequestCallback(union sigval sv)
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
	ms->openClientMqAndSendStatistics(buf);


    free(buf);

	//Требуется перерегистрация калбека
	RegisterClientRequestCallback(false);
}
void MqServer::openClientMqAndSendStatistics(const char* name)
{

}

