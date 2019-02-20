/*
The MIT License (MIT)

Copyright (c) 2018 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ofxSearchNetworkNode.h"
#include "ofAppRunner.h"

using namespace std;

ofxSearchNetworkNode::ofxSearchNetworkNode()
:prefix_("ofxSearchNetworkNode")
,is_sleep_(true)
{
	self_ip_ = NetworkUtils::getIPv4Interface();
	for_each(begin(self_ip_), end(self_ip_), [this](const NetworkUtils::IPv4Interface &ip) {
		if(ip.broadcast != "") {
			target_ip_.push_back(ip.broadcast);
		}
	});
	setName(NetworkUtils::getHostName());
	awake();
}
void ofxSearchNetworkNode::sleep()
{
	if(!is_sleep_) {
		ofRemoveListener(ofEvents().update, this, &ofxSearchNetworkNode::update);
		is_sleep_ = true;
	}
}
void ofxSearchNetworkNode::awake()
{
	if(is_sleep_) {
		ofAddListener(ofEvents().update, this, &ofxSearchNetworkNode::update);
		is_sleep_ = false;
	}
}
ofxSearchNetworkNode::~ofxSearchNetworkNode()
{
	disconnect();
	sleep();
}
void ofxSearchNetworkNode::setup(int port)
{
	port_ = port;
	receiver_.setup(port);
}

void ofxSearchNetworkNode::setName(const string &name)
{
	name_ = name;
}

void ofxSearchNetworkNode::addToGroup(const string &group)
{
	addToGroup(vector<string>{group});
}
void ofxSearchNetworkNode::addToGroup(const vector<string> &group)
{
	group_.insert(end(group_), begin(group), end(group));
}
void ofxSearchNetworkNode::setGroup(const string &group, bool refresh)
{
	setGroup(vector<string>{group}, refresh);
}
void ofxSearchNetworkNode::setGroup(const vector<string> &group, bool refresh)
{
	group_.clear();
	addToGroup(group);
	if(refresh) {
		disconnect();
		request();
	}
}

void ofxSearchNetworkNode::request()
{
	request(group_);
}
void ofxSearchNetworkNode::request(const vector<string> &group)
{
	for_each(begin(target_ip_), end(target_ip_), [this,&group](string &ip) {
		sendMessage(ip, createRequestMessage(group, is_secret_mode_?makeHash(getSelfIp(ip)):0));
	});
}
void ofxSearchNetworkNode::requestTo(const std::string &ip)
{
	sendMessage(ip, createRequestMessage(group_, is_secret_mode_?makeHash(getSelfIp(ip)):0));
}
void ofxSearchNetworkNode::disconnectFrom(const std::string &ip)
{
	auto it = known_nodes_.find(ip);
	if(it == end(known_nodes_)) {
		ofLogWarning("can't disconnect from unknown node : " + ip);
		return;
	}
	unregisterNode(ip, it->second);
	sendMessage(ip, createDisconnectMessage());
}
void ofxSearchNetworkNode::flush()
{
	known_nodes_.clear();
	heartbeat_send_.clear();
	heartbeat_recv_.clear();
}
void ofxSearchNetworkNode::enableSecretMode(const string &key)
{
	is_secret_mode_ = true;
	secret_key_ = key;
}

void ofxSearchNetworkNode::update(ofEventArgs&)
{
	while(receiver_.hasWaitingMessages()) {
		ofxOscMessage msg;
		receiver_.getNextMessage(msg);
		messageReceived(msg);
	}
	
	float frame_time = ofGetLastFrameTime();
	for_each(begin(heartbeat_send_), end(heartbeat_send_), [this,frame_time](pair<const string,TimerArgs> &h) {
		TimerArgs &timer = h.second;
		timer.timer += frame_time;
		if(timer.timer >= timer.limit) {
			sendMessage(h.first, createHeartbeatMessage());
			timer.timer -= timer.limit;
		}
	});
	if(need_heartbeat_) {
		for_each(begin(heartbeat_recv_), end(heartbeat_recv_), [this,frame_time](pair<const string,TimerArgs> &h) {
			TimerArgs &timer = h.second;
			timer.timer += frame_time;
			if(timer.timer >= timer.limit) {
				lostNode(h.first);
			}
		});
	}
}

vector<string> ofxSearchNetworkNode::getGroups(const ofxOscMessage &msg, int &index) const
{
	vector<string> ret;
	int count = msg.getArgAsInt32(index++);
	ret.reserve(count);
	int last_index = index+count;
	for(; index < last_index; ++index) {
		ret.push_back(msg.getArgAsString(index));
	}
	return ret;
}
void ofxSearchNetworkNode::setGroupsTo(ofxOscMessage &msg, const vector<string> &groups) const
{
	msg.addInt32Arg(groups.size());
	for(auto &g : groups) {
		msg.addStringArg(g);
	}
}

void ofxSearchNetworkNode::registerNode(const string &ip, const string &name, const vector<string> &group, bool heartbeat_required, float heartbeat_interval)
{
	Node n{name, group, false};
	auto result = known_nodes_.insert(make_pair(ip, n));
	if(result.second) {
		ofNotifyEvent(nodeFound, *result.first);
	}
	else {
		if(result.first->second.lost) {
			result.first->second = n;
			ofNotifyEvent(nodeReconnected, *result.first);
		}
		else {
			if(!result.first->second.isSame(n)) {
				result.first->second = n;
				ofNotifyEvent(nodePropertyChanged, *result.first);
			}
		}
	}
	
	if(need_heartbeat_) {
		heartbeat_recv_[ip] = TimerArgs{0, heartbeat_timeout_};
	}
	if(heartbeat_required) {
		heartbeat_send_[ip] = TimerArgs{0, heartbeat_interval};
		sendMessage(ip, createHeartbeatMessage());
	}
}
void ofxSearchNetworkNode::unregisterNode(const string &ip, const Node &n)
{
	Node cache = n;
	known_nodes_.erase(ip);
	heartbeat_send_.erase(ip);
	heartbeat_recv_.erase(ip);
	ofNotifyEvent(nodeDisconnected, make_pair(ip,cache));
}
void ofxSearchNetworkNode::lostNode(const string &ip)
{
	auto it = known_nodes_.find(ip);
	if(it != end(known_nodes_) && !it->second.lost) {
		it->second.lost = true;
		ofNotifyEvent(nodeLost, *it);
	}
}
void ofxSearchNetworkNode::reconnectNode(const string &ip)
{
	auto it = known_nodes_.find(ip);
	if(it != end(known_nodes_) && it->second.lost) {
		it->second.lost = false;
		ofNotifyEvent(nodeReconnected, *it);
	}
}

bool ofxSearchNetworkNode::isSelfIp(const string &ip) const
{
	return any_of(begin(self_ip_), end(self_ip_), [&ip](const NetworkUtils::IPv4Interface &me){return ip==me.ip;});
}
string ofxSearchNetworkNode::getSelfIp(const string &an_ip_in_same_netwotk) const
{
	auto it = find_if(begin(self_ip_), end(self_ip_), [&an_ip_in_same_netwotk](const NetworkUtils::IPv4Interface &me) {
		return me.isInSameNetwork(an_ip_in_same_netwotk);
	});
	return it != end(self_ip_) ? it->ip : "";
}
string ofxSearchNetworkNode::getSelfIpForInterface(const string &interface_name) const
{
	auto it = find_if(begin(self_ip_), end(self_ip_), [&interface_name](const NetworkUtils::IPv4Interface &me) {
		return me.name == interface_name;
	});
	return it != end(self_ip_) ? it->ip : "";
}


void ofxSearchNetworkNode::messageReceived(ofxOscMessage &msg)
{
	const auto &&address = ofSplitString(msg.getAddress(), "/", false);
	if(address.size() >= 2 && address[0] == "" && address[1] == prefix_) {
		const string &method = address.size()>=3?address[2]:"";
		if(method == "request") {
			string ip = msg.getRemoteHost();
			if(!allow_loopback_ && isSelfIp(ip)) {
				return;
			}
			int index = 0;
			vector<string> groups = getGroups(msg, index);
			int32_t secret_key = msg.getArgAsInt32(index++);
			if(secret_key != 0 || is_secret_mode_) {
				if(!checkHash(secret_key, ip)) {
					return;
				}
			}
			if((group_.empty() && groups.empty()) || any_of(begin(groups), end(groups), [this](string &group) {
				return ofContains(group_, group);
			})) {
				string name = msg.getArgAsString(index++);
				vector<string> group = getGroups(msg, index);
				bool heartbeat = msg.getArgAsBool(index++);
				float heartbeat_interval = msg.getArgAsFloat(index++);
				registerNode(ip, name, group, heartbeat, heartbeat_interval);
				sendMessage(ip, createResponseMessage(is_secret_mode_?makeHash(getSelfIp(ip)):0));
			}
		}
		else if(method == "response") {
			string ip = msg.getRemoteHost();
			int32_t secret_key = msg.getArgAsInt32(0);
			if(secret_key != 0 || is_secret_mode_) {
				if(!checkHash(secret_key, ip)) {
					return;
				}
			}
			int index = 1;
			string name = msg.getArgAsString(index++);
			vector<string> group = getGroups(msg, index);
			bool heartbeat = msg.getArgAsBool(index++);
			float heartbeat_interval = msg.getArgAsFloat(index++);
			registerNode(ip, name, group, heartbeat, heartbeat_interval);
		}
		else if(method == "disconnect") {
			string ip = msg.getRemoteHost();
			auto it = known_nodes_.find(ip);
			if(it == end(known_nodes_)) {
				ofLogWarning("received disconnected message from unknown node : " + ip);
				return;
			}
			unregisterNode(ip, it->second);
		}
		else if(method == "heartbeat") {
			string ip = msg.getRemoteHost();
			auto it = heartbeat_recv_.find(ip);
			if(it == end(heartbeat_recv_)) {
				ofLogWarning("received heartbeat message from unknown node : " + ip);
				return;
			}
			it->second.timer = 0;
			auto node = known_nodes_.find(ip);
			if(node != end(known_nodes_) && node->second.lost) {
				reconnectNode(ip);
			}
		}
	}
	else {
		ofNotifyEvent(unhandledMessageReceived, msg, this);
	}
}
ofxOscMessage ofxSearchNetworkNode::createRequestMessage(const vector<string> &group, HashType key) const
{
	ofxOscMessage ret;
	ret.setAddress(ofJoinString({"",prefix_,"request"},"/"));
	setGroupsTo(ret, group);
	ret.addInt32Arg(key);
	ret.addStringArg(name_);
	setGroupsTo(ret, group_);
	ret.addBoolArg(need_heartbeat_);
	ret.addFloatArg(heartbeat_request_interval_);
	return move(ret);
}
ofxOscMessage ofxSearchNetworkNode::createResponseMessage(HashType key) const
{
	ofxOscMessage ret;
	ret.setAddress(ofJoinString({"",prefix_,"response"},"/"));
	ret.addInt32Arg(key);
	ret.addStringArg(name_);
	setGroupsTo(ret, group_);
	ret.addBoolArg(need_heartbeat_);
	ret.addFloatArg(heartbeat_request_interval_);
	return move(ret);
}
ofxOscMessage ofxSearchNetworkNode::createDisconnectMessage() const
{
	ofxOscMessage ret;
	ret.setAddress(ofJoinString({"",prefix_,"disconnect"},"/"));
	return move(ret);
}
ofxOscMessage ofxSearchNetworkNode::createHeartbeatMessage() const
{
	ofxOscMessage ret;
	ret.setAddress(ofJoinString({"",prefix_,"heartbeat"},"/"));
	return move(ret);
}
void ofxSearchNetworkNode::disconnect()
{
	sendMessage(createDisconnectMessage());
	flush();
}

void ofxSearchNetworkNode::sendMessage(const string &ip, ofxOscMessage msg) {
	ofxOscSender sender;
	sender.setup(ip, port_);
	sender.sendMessage(msg);
}
void ofxSearchNetworkNode::sendMessage(ofxOscMessage msg) {
	for_each(begin(known_nodes_), end(known_nodes_), [this,&msg](const pair<string,Node> &p) {
		sendMessage(p.first, msg);
	});
}

void ofxSearchNetworkNode::sendBundle(const string &ip, ofxOscBundle bundle) {
	ofxOscSender sender;
	sender.setup(ip, port_);
	sender.sendBundle(bundle);
}
void ofxSearchNetworkNode::sendBundle(ofxOscBundle bundle) {
	for_each(begin(known_nodes_), end(known_nodes_), [this,&bundle](const pair<string,Node> &p) {
		sendBundle(p.first, bundle);
	});
}

namespace {
	uint32_t crc_table[256];
	bool crc_table_created=false;
	void createCRC32Table() {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int j = 0; j < 8; j++) {
				c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
			}
			crc_table[i] = c;
		}
		crc_table_created = true;
	};
	uint32_t crc32(const char *buf, size_t len) {
		if(!crc_table_created) { createCRC32Table(); }
		uint32_t c = 0xFFFFFFFF;
		for (size_t i = 0; i < len; i++) {
			c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
		}
		return c ^ 0xFFFFFFFF;
	};
}
ofxSearchNetworkNode::HashType ofxSearchNetworkNode::makeHash(const string &self_ip) const
{
	string src = secret_key_ + self_ip;
	return crc32(src.c_str(), src.length());
}
bool ofxSearchNetworkNode::checkHash(HashType hash, const string &remote_ip) const
{
	return hash == makeHash(remote_ip);
}
