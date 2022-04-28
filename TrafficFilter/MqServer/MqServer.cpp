#include <signal.h>
#include <string.h>
#include <iostream>
#include "MqServer.hpp"
#include "../Sender/Sender.hpp"

using namespace std;

#define MSG_Q_NAME "/UDP_DUMP_SERVER_1"

MqServer::MqServer()
{
    ms = this;
}
void MqServer::run()
{
    registerClientRequestCallback(true);
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
void MqServer::ClientRequestCallback(union sigval sv)
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
	ms->registerClientRequestCallback(false);
}
void MqServer::openClientMqAndSendStatistics(const char* name)
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


	Sender::last_statistics_m.lock();
	if(mq_send(msq_id, Sender::last_statistics.c_str(), strlen(Sender::last_statistics.c_str()), 0) < 0)
	{
		if(errno != EAGAIN)
			cout << "Error on sending msg on MsgQ " << strerror(errno);
	}
	Sender::last_statistics_m.unlock();
	mq_close(msq_id);
}

MqServer* MqServer::ms{nullptr};