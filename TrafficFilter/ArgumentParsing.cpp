#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <algorithm>
#include <ArgumentParser.hpp>
#include "Filter/Filter.hpp"
#include "Sender/Sender.hpp"

using namespace std;

static void PrintHelp()
{
	constexpr char const* str{
		"Usage: dump [options]\n"
		"Options:\n"
		"	--interface <arg>	  Sellect interface.\n"
		"	--src-ip    <arg>	  Sets the source ip.        (multiple)\n"
		"	--dst-ip    <arg>	  Sets the destination ip.   (multiple)\n"
		"	--src-port  <arg>	  Sets the source port.      (multiple)\n"
		"	--dst-port  <arg>	  Sets the destination port. (multiple)\n"
		"	--src-mac   <arg>	  Sets the source mac.       (multiple)\n"
		"	--dst-mac   <arg>	  Sets the destination mac.  (multiple)\n"
		"   -t                    TCP filter.\n"
		"   -u                    UDP filter.\n"
		"   --no-print-statistics Disables output of statistics to the console.\n"
	};
	cout << str;
}
void ArgumentParsing(int argc, const char* argv[], Filter& f, Sender& s)
{
	ArgumentParser ap(argc, argv);

	//help
	auto help_param_pos = ap.find("--help");
	auto help_param_short_pos = ap.find("-h");
	if (help_param_pos != -1 || help_param_short_pos != -1)
	{
		PrintHelp();
		exit(0);
	}

	try
	{
		// --src-ip
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--src-ip", i);
			if (i == -1)
				break;

			const auto& ip_s = ap.at(i + 1);

			in_addr_t a = inet_addr(ip_s.c_str());
			if (a == INADDR_NONE)
			{
				cerr << "Error: bed ip in param " << i + 1 << endl;
				exit(1);
			}
			f.sources_ip_filter.push_back(a);

			cout << "Set filter source ip: " << ip_s << endl;
		}

		// --dest-ip
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--dest-ip", i);
			if (i == -1)
				break;

			const auto& ip_s = ap.at(i + 1);

			in_addr_t a = inet_addr(ip_s.c_str());
			if (a == INADDR_NONE)
			{
				cerr << "Error: bed ip in param " << i + 1 << endl;
				exit(1);
			}
			f.dest_ip_filter.push_back(a);

			cout << "Set filter destination ip: " << ip_s << endl;
		}

		// --src-port
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--src-port", i);
			if (i == -1)
				break;

			const auto& port_s = ap.at(i + 1);

			stringstream ss;
			ss << port_s;
			uint16_t port;
			ss >> port;
			f.sources_port_filter.push_back(ntohs(port));

			cout << "Set filter source port: " << port_s << endl;
		}

		// --dest-port
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--dest-port", i);
			if (i == -1)
				break;

			const auto& port_s = ap.at(i + 1);

			stringstream ss;
			ss << port_s;
			uint16_t port;
			ss >> port;
			f.dest_port_filter.push_back(ntohs(port));

			cout << "Set filter destination port: " << port_s << endl;
		}

		// --src-mac
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--src-mac", i);
			if (i == -1)
				break;

			auto mac_s = ap.at(i + 1);
			std::replace(mac_s.begin(), mac_s.end(), ':', ' ');
			cout << "mac: " << mac_s << endl;

			stringstream ss;
			ss.str(mac_s);
			ss >> std::hex;
			Filter::mac m;
			for (size_t i = 0; i < ETH_ALEN; ++i)
			{
				uint16_t byte{};
				ss >> byte;
				m.arr[i] = uint8_t(byte);
			}
			f.sources_mac_filter.push_back(m);
		}

		// --dest-mac
		for (size_t i = 0; i < ap.size(); ++i)
		{
			i = ap.find("--dest-mac", i);
			if (i == -1)
				break;

			auto mac_s = ap.at(i + 1);
			std::replace(mac_s.begin(), mac_s.end(), ':', ' ');
			cout << "mac: " << mac_s << endl;

			stringstream ss;
			ss.str(mac_s);
			ss >> std::hex;
			Filter::mac m;
			for (size_t i = 0; i < ETH_ALEN; ++i)
			{
				uint16_t byte{};
				ss >> byte;
				m.arr[i] = uint8_t(byte);
			}
			f.dest_mac_filter.push_back(m);
		}

		// --interface
		f.interface = ap.get<string>("--interface", "");
		
		// -t
		auto tcp = ap.find("-t");
		if (tcp != -1)
			f.protocols_filter.push_back(6);

		// -u
		auto udp = ap.find("-u");
		if (udp != -1)
			f.protocols_filter.push_back(17);

		// -no-print-statistics
		auto nps = ap.find("-no-print-statistics");
		if (nps != -1)
			s.allowPrintStatistics(false);

	}
	catch (const std::out_of_range& e)
	{
		std::cerr << "Error: no command argument" << '\n';
		exit(1);
	}
	catch(const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
		exit(1);
	}
}
