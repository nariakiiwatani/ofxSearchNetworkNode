/*
The MIT License (MIT)

Copyright (c) 2017 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "NetworkUtils.h"
#include "ofConstants.h"

#if defined(TARGET_OSX) || defined(TARGET_OF_IOS) || defined(TARGET_LINUX)
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

using namespace std;

std::string NetworkUtils::getHostName()
{
	char buf[256] = {};
	if(gethostname(buf, 256) != 0) {
		return "";
	}
	return buf;
}

std::vector<NetworkUtils::IPv4Interface> NetworkUtils::getIPv4Interface()
{
	struct ifaddrs *ifas, *ifa;
	if(getifaddrs(&ifas) != 0) {
		return {};
	}
	vector<IPv4Interface> ret;
	for(ifa = ifas; ifa != nullptr; ifa=ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			IPv4Interface result;
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

bool NetworkUtils::IPv4Interface::isInSameNetwork(const std::string &hint) const
{
	return (ip_raw&netmask_raw) == (inet_addr(hint.c_str())&netmask_raw);
}

#elif defined(TARGET_WIN32)
#include <ws2tcpip.h>
std::string NetworkUtils::getHostName(){ return ""; }
std::vector<NetworkUtils::IPv4Interface> NetworkUtils::getIPv4Interface()
{
	WSADATA ws_data;
	if(WSAStartup(WINSOCK_VERSION, &ws_data) != 0) {
		return {};
	}

	SOCKET socket = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
	if(socket == SOCKET_ERROR) {
		return {};
	}

	INTERFACE_INFO if_info[32];
	unsigned long num_bytes;
	if(WSAIoctl(socket, SIO_GET_INTERFACE_LIST, 0, 0, &if_info, sizeof(if_info), &num_bytes, 0, 0) == SOCKET_ERROR) {
		return {};
	}

	vector<IPv4Interface> ret;
	int num_if = num_bytes/sizeof(INTERFACE_INFO);
	for(int i = 0, num = num_bytes/sizeof(INTERFACE_INFO); i < num; ++i) {
		INTERFACE_INFO info = if_info[i];
		u_long flags = info.iiFlags;

		IPv4Interface result;
		char name[256];
		if(getnameinfo(&info.iiAddress.Address, sizeof(info.iiAddress.Address), name, sizeof(name), NULL, 0, NI_DGRAM) != 0) {
			continue;
		}
		result.name = name;
		auto get_address = [](sockaddr_in *ifa_addr, unsigned int &dst_raw, string &dst_str) {
			char str[INET_ADDRSTRLEN] = {};
			dst_raw = ifa_addr->sin_addr.s_addr;
			inet_ntop(AF_INET, &dst_raw, str, sizeof(str));
			dst_str = str;
		};
		get_address(&info.iiAddress.AddressIn, result.ip_raw, result.ip);
		get_address(&info.iiNetmask.AddressIn, result.netmask_raw, result.netmask);
		if((flags & IFF_BROADCAST) != 0) {
			get_address(&info.iiBroadcastAddress.AddressIn, result.broadcast_raw, result.broadcast);
		}
		else {
			result.broadcast_raw = 0;
			result.broadcast = "";
		}
		ret.push_back(result);
	}
	WSACleanup();
	return ret;
}
bool NetworkUtils::IPv4Interface::isInSameNetwork(const std::string &hint) const
{
	return (ip_raw&netmask_raw) == (inet_addr(hint.c_str())&netmask_raw);
}
#else
std::string NetworkUtils::getHostName(){ return ""; }
std::vector<NetworkUtils::IPv4Interface> NetworkUtils::getIPv4Interface() { return {}; }
bool NetworkUtils::IPv4Interface::isInSameNetwork(const std::string &hint) const { return false; }
#endif
