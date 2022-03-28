#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/ip.h> /* for ipv4 header */
#include <linux/udp.h> /* for upd header */

#include <iostream>
#include <sstream>
#include "ArgumentParserLib/ArgumentParser.hpp"

using namespace std;

#define MSG_SIZE 65535
#define HEADER_SIZE (sizeof(struct iphdr) + sizeof(struct udphdr))

size_t package_count{0}, bytes_count{0};

string to_ip(uint32_t ipi)
{
	uint8_t* bytes = (uint8_t*)&ipi;

	stringstream ss;
	ss << uint16_t(bytes[0]) << "." << uint16_t(bytes[1]) << "." << uint16_t(bytes[2]) << "." << uint16_t(bytes[3]);
	return ss.str();
}

int my_main(int argc, const char* argv[]) {
	if (argc != 4)
	{
		cerr << "Invalid number of arguments." << endl;
		cerr << "<source ip> <port> <dest ip>" << endl;
		return 1;
	}


	int raw_socket;
	//sockaddr_in sockstr;
	socklen_t socklen;

	int retval = 0; 

	char msg[MSG_SIZE];
	ssize_t msglen;

	if ((raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
		perror("socket");
		return 1;
	}

	uint16_t port = atoi(argv[2]);

	while(1)
	{
		if ((msglen = recv(raw_socket, msg, MSG_SIZE, 0)) == -1) {
			perror("recv");
			retval = 1;
			goto _go_close_socket;
		}
		
		// increase counters
		package_count++;
		bytes_count += msglen;

		if (msglen <= HEADER_SIZE)
			cerr << "Bed package: wrong header seize." << endl;
		else {
			iphdr* th = (struct iphdr*)msg;
			udphdr* udp = (udphdr*) (msg + sizeof(iphdr));

			if (to_ip(th->saddr) != argv[1])
				continue;
			if (to_ip(th->daddr) != argv[3])
				continue;
			if (ntohs(udp->dest) != port)
				continue;

			cout << "Packages received: " << package_count << " bytes received: " << bytes_count;

			cout << " proto: " << uint32_t(th->protocol) << 
				" ip source: " << to_ip(th->saddr) << 
				" ip dest: " << to_ip(th->daddr) <<
				" len: " << ntohs(th->tot_len) <<
				" msgl: " << msglen;

			cout << " port source: " << ntohs(udp->source) << " port dest: " << ntohs(udp->dest) << endl;
		}
	}

_go_close_socket:
	close(raw_socket);

	return retval;
}
int main(int argc, const char* argv[])
{
	ArgumentParser ap(argc, argv);
	auto s = ap.get<uint32_t>("--help", 123);
	cout << "S: " << s << endl;

	auto f = ap.find("--help");
	cout << "f: " << f << endl;

	return 0;
}