#include <vector>
#include <string>
#include <stdint.h>
#include <linux/if_ether.h>
#include "../Stat/Stat.hpp"

class Filter
{
public:
	Filter();
	~Filter();

	void setStatisticsOutput(Stat& s);
	void run();

	std::vector<uint32_t> sources_ip_filter;
	std::vector<uint32_t> dest_ip_filter;

	std::vector<uint16_t> sources_port_filter;
	std::vector<uint16_t> dest_port_filter;

	struct mac
	{
		unsigned char arr[ETH_ALEN];
	};
	std::vector<mac> sources_mac_filter;
	std::vector<mac> dest_mac_filter;

	std::string interface;
	std::vector<uint8_t> protocols_filter;

private:
	bool SourceIpFilter(uint32_t ip);
	bool DestIpFilter(uint32_t ip);
	bool SourcePortFilter(uint16_t port);
	bool DestPortFilter(uint16_t port);
	bool SourceMacFilter(uint8_t h_source[ETH_ALEN]);
	bool DestMacFilter(uint8_t h_source[ETH_ALEN]);
	bool ProtocolsFilter(uint8_t proto);

	bool SetPromisc(const char* ifname, bool enable);
	bool SetPromiscAll(bool enable);

	Stat* so;
};
