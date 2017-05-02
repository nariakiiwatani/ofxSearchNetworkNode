/*
The MIT License (MIT)

Copyright (c) 2017 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "IPAddress.h"
#include "ofConstants.h"

#if defined(TARGET_OSX) || defined(TARGET_OF_IOS) || defined(TARGET_LINUX)
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

using namespace std;

std::vector<IPAddress::IPv4> IPAddress::getv4()
{
	struct ifaddrs *ifas, *ifa;
	if(getifaddrs(&ifas) != 0) {
		return {};
	}
	vector<IPv4> ret;
	for(ifa = ifas; ifa != nullptr; ifa=ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			IPv4 result;
			result.name = ifa->ifa_name;
			auto get_address = [](sockaddr_in *ifa_addr, unsigned int &dst_raw, string &dst_str) {
				char str[INET_ADDRSTRLEN] = {};
				dst_raw = ifa_addr->sin_addr.s_addr;
				inet_ntop(AF_INET, &dst_raw, str, sizeof(str));
				dst_str = str;
			};
			get_address((struct sockaddr_in *)ifa->ifa_addr, result.ip_raw, result.ip);
			get_address((struct sockaddr_in *)ifa->ifa_netmask, result.netmask_raw, result.netmask);
			if((ifa->ifa_flags & IFF_BROADCAST) != 0) {
				get_address((struct sockaddr_in *)ifa->ifa_dstaddr, result.broadcast_raw, result.broadcast);
			}
			else {
				result.broadcast_raw = 0;
				result.broadcast = "";
			}
			ret.push_back(result);
		}
	}
	freeifaddrs(ifas);
	return ret;
}

bool IPAddress::IPv4::isInSameNetwork(const std::string &hint) const
{
	return (ip_raw&netmask_raw) == (inet_addr(hint.c_str())&netmask_raw);
}

#else
std::vector<IPAddress::IPv4> IPAddress::getv4() { return {}; }
bool IPAddress::IPv4::isInSameNetwork(const std::string &hint) const { return false; }
#endif
