/*
The MIT License (MIT)

Copyright (c) 2017 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "NetworkUtils.h"
#include "ofConstants.h"

using namespace std;

#if defined(TARGET_OSX) || defined(TARGET_OF_IOS) || defined(TARGET_LINUX)
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>

string NetworkUtils::getHostName()
{
	char buf[256] = {};
	if(gethostname(buf, 256) != 0) {
		return "";
	}
	return buf;
}

vector<NetworkUtils::IPv4Interface> NetworkUtils::getIPv4Interface()
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

bool NetworkUtils::IPv4Interface::isInSameNetwork(const string &hint) const
{
	return (ip_raw&netmask_raw) == (inet_addr(hint.c_str())&netmask_raw);
}

#elif defined(TARGET_WIN32)
#include <ws2tcpip.h>
#include <iphlpapi.h>
string NetworkUtils::getHostName()
{
	char buf[256] = {};
	if(gethostname(buf, 256) != 0) {
		return "";
	}
	return buf;
}

vector<NetworkUtils::IPv4Interface> NetworkUtils::getIPv4Interface()
{
	PIP_ADAPTER_ADDRESSES if_info = nullptr;
	ULONG data_size=0;
	ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
	if(GetAdaptersAddresses(AF_INET, flags, NULL, NULL, &data_size) == ERROR_BUFFER_OVERFLOW) {
		if_info = (PIP_ADAPTER_ADDRESSES)malloc(data_size);
		if(if_info) {
			if(GetAdaptersAddresses(AF_INET, flags, NULL, if_info, &data_size) != ERROR_SUCCESS) {
				free(if_info);
				return {};
			}
		}
	}
	vector<IPv4Interface> ret;
	while(if_info) {
		IPv4Interface result;
		result.name = if_info->AdapterName;
		ULONG addr = ((sockaddr_in*)(if_info->FirstUnicastAddress->Address.lpSockaddr))->sin_addr.s_addr;
		ULONG mask;
		ConvertLengthToIpv4Mask(if_info->FirstUnicastAddress->OnLinkPrefixLength, &mask);
		auto get_address = [](ULONG addr, unsigned int &dst_raw, string &dst_str) {
			char str[INET_ADDRSTRLEN] = {};
			dst_raw = addr;
			inet_ntop(AF_INET, &dst_raw, str, sizeof(str));
			dst_str = str;
		};
		get_address(addr, result.ip_raw, result.ip);
		get_address(mask, result.netmask_raw, result.netmask);
		get_address(addr|~mask, result.broadcast_raw, result.broadcast);
		ret.push_back(result);
		if_info = if_info->Next;
	}
	return ret;
}
bool NetworkUtils::IPv4Interface::isInSameNetwork(const string &hint) const
{
	return (ip_raw&netmask_raw) == (inet_addr(hint.c_str())&netmask_raw);
}
#else
string NetworkUtils::getHostName(){ return ""; }
vector<NetworkUtils::IPv4Interface> NetworkUtils::getIPv4Interface() { return {}; }
bool NetworkUtils::IPv4Interface::isInSameNetwork(const string &hint) const { return false; }
#endif
