/*
The MIT License (MIT)

Copyright (c) 2017 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ofxSearchNetworkNode.h"

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
void ofxSearchNetworkNode::setup(int port, const std::string &name, const std::string &group)
{
	name_ = name==""?NetworkUtils::getHostName():name;
	port_ = port;
	receiver_.setup(port);
	addToGroup(group);
}
void ofxSearchNetworkNode::addToGroup(const std::string &group)
{
	auto new_groups = ofSplitString(group, ",", true);
	group_.insert(end(group_), begin(new_groups), end(new_groups));
}

void ofxSearchNetworkNode::request(const std::string &group)
{
	for_each(begin(target_ip_), end(target_ip_), [this,&group](std::string &ip) {
		sendMessage(ip, createRequestMessage(group==""?ofJoinString(group_,","):group, is_secret_mode_?makeHash(getSelfIp(ip)):0));
	});
}
void ofxSearchNetworkNode::flush()
{
	known_nodes_.clear();
	heartbeat_send_.clear();
	heartbeat_recv_.clear();
}
void ofxSearchNetworkNode::enableSecretMode(const std::string &key)
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

void ofxSearchNetworkNode::registerNode(const std::string &ip, const std::string &name, const std::string &group, bool heartbeat_required, float heartbeat_interval)
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
void ofxSearchNetworkNode::unregisterNode(const std::string &ip, const Node &n)
{
	known_nodes_.erase(ip);
	heartbeat_send_.erase(ip);
	heartbeat_recv_.erase(ip);
	ofNotifyEvent(nodeDisconnected, make_pair(ip,n));
}
void ofxSearchNetworkNode::lostNode(const std::string &ip)
{
	auto it = known_nodes_.find(ip);
	if(it != end(known_nodes_) && !it->second.lost) {
		it->second.lost = true;
		ofNotifyEvent(nodeLost, *it);
	}
}
void ofxSearchNetworkNode::reconnectNode(const std::string &ip)
{
	auto it = known_nodes_.find(ip);
	if(it != end(known_nodes_) && !it->second.lost) {
		it->second.lost = false;
		ofNotifyEvent(nodeReconnected, *it);
	}
}

bool ofxSearchNetworkNode::isSelfIp(const std::string &ip) const
{
	return any_of(begin(self_ip_), end(self_ip_), [&ip](const NetworkUtils::IPv4Interface &me){return ip==me.ip;});
}
std::string ofxSearchNetworkNode::getSelfIp(const std::string &an_ip_in_same_netwotk)
{
	auto it = find_if(begin(self_ip_), end(self_ip_), [&an_ip_in_same_netwotk](NetworkUtils::IPv4Interface &me) {
		return me.isInSameNetwork(an_ip_in_same_netwotk);
	});
	return it != end(self_ip_) ? it->ip : "";
}


void ofxSearchNetworkNode::messageReceived(ofxOscMessage &msg)
{
	const auto &&address = ofSplitString(msg.getAddress(), "/", false);
	if(address.size() >= 2 && address[0] == "" && address[1] == prefix_) {
		const string &method = address.size()>=3?address[2]:"";
		if(method == "request") {
			string ip = msg.getRemoteIp();
			if(!allow_loopback_ && isSelfIp(ip)) {
				return;
			}
			int32_t secret_key = msg.getArgAsInt32(1);
			if(secret_key != 0 || is_secret_mode_) {
				if(!checkHash(secret_key, ip)) {
					return;
				}
			}
			vector<string> groups = ofSplitString(msg.getArgAsString(0), ",");
			if(any_of(begin(groups), end(groups), [this](string &group) {
				return ofContains(group_, group);
			})) {
				registerNode(ip,
							 msg.getArgAsString(2), msg.getArgAsString(3),
							 msg.getArgAsBool(4), msg.getArgAsFloat(5));
				sendMessage(ip, createResponseMessage(is_secret_mode_?makeHash(getSelfIp(ip)):0));
			}
		}
		else if(method == "response") {
			string ip = msg.getRemoteIp();
			int32_t secret_key = msg.getArgAsInt32(0);
			if(secret_key != 0 || is_secret_mode_) {
				if(!checkHash(secret_key, ip)) {
					return;
				}
			}
			registerNode(ip,
						 msg.getArgAsString(1), msg.getArgAsString(2),
						 msg.getArgAsBool(3), msg.getArgAsFloat(4));
		}
		else if(method == "disconnect") {
			string ip = msg.getRemoteIp();
			auto it = known_nodes_.find(ip);
			if(it == end(known_nodes_)) {
				ofLogWarning("received disconnected message from unknown node : " + ip);
				return;
			}
			unregisterNode(ip, it->second);
		}
		else if(method == "heartbeat") {
			string ip = msg.getRemoteIp();
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
ofxOscMessage ofxSearchNetworkNode::createRequestMessage(const std::string &group, HashType key) const
{
	ofxOscMessage ret;
	ret.setAddress(ofJoinString({"",prefix_,"request"},"/"));
	ret.addStringArg(group);
	ret.addInt32Arg(key);
	ret.addStringArg(name_);
	ret.addStringArg(ofJoinString(group_,","));
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
	ret.addStringArg(ofJoinString(group_,","));
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
