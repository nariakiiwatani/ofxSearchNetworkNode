/*
The MIT License (MIT)

Copyright (c) 2017 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "ofEvents.h"
#include "ofxOsc.h"
#include "IPAddress.h"

class ofxSearchNetworkNode
{
public:
	ofxSearchNetworkNode();
	virtual ~ofxSearchNetworkNode();
	void setup(int port, const std::string &name, const std::string &group="");
	void setTargetIp(const std::string &ip) { target_ip_ = ofSplitString(ip,",",true); }
	void setAllowLoopback(bool allow) { allow_loopback_ = allow; }
	void setPrefix(const std::string &prefix) { prefix_ = prefix; }
	
	ofEvent<ofxOscMessage> unhandledMessageReceived;
	
	struct Node {
		std::string name;
		std::string group;
		bool lost;
		bool isSame(const Node &node) {
			return this->name==node.name && this->group==node.group;
		}
	};
	
	void request(const std::string &group="");
	void flush();
	void disconnect();
	
	void sleep();
	void awake();
	void setSleep(bool s) { s?sleep():awake(); }
	bool isSleep() const { return is_sleep_; }
	
	ofEvent<const pair<std::string,Node>> nodeFound;
	ofEvent<const pair<std::string,Node>> nodePropertyChanged;
	ofEvent<const pair<std::string,Node>> nodeDisconnected;
	ofEvent<const pair<std::string,Node>> nodeLost;
	ofEvent<const pair<std::string,Node>> nodeReconnected;
	
	const std::map<std::string, Node>& getNodes() const { return known_nodes_; }
	bool isSelfIp(const std::string &ip) const;
	
	void setRequestHeartbeat(bool heartbeat, float request_interval=1, float timeout=3) {
		need_heartbeat_=heartbeat;
		heartbeat_request_interval_ = request_interval;
		heartbeat_timeout_=timeout;
	}
	
	void enableSecretMode(const std::string &key);
	void disableSecretMode() { is_secret_mode_=false; }
	
private:
	void update(ofEventArgs&);
	void registerNode(const std::string &ip, const std::string &name, const std::string &group, bool heartbeat_required, float heartbeat_interval);
	void unregisterNode(const std::string &ip, const Node &n);
	void lostNode(const std::string &ip);
	void reconnectNode(const std::string &ip);
	void messageReceived(ofxOscMessage &msg);
	void sendMessage(const string &ip, ofxOscMessage msg) {
		ofxOscSender sender;
		sender.setup(ip, port_);
		sender.sendMessage(msg);
	}
	void sendMessage(ofxOscMessage msg) {
		for_each(begin(known_nodes_), end(known_nodes_), [this,&msg](const pair<string,Node> &p) {
			sendMessage(p.first, msg);
		});
	}
	
	using HashType = std::uint32_t;
	HashType makeHash(const std::string &self_ip) const;
	bool checkHash(HashType hash, const std::string &remote_ip) const;
	ofxOscMessage createRequestMessage(const std::string &group, HashType key) const;
	ofxOscMessage createResponseMessage(HashType key) const;
	ofxOscMessage createDisconnectMessage() const;
	ofxOscMessage createHeartbeatMessage() const;
	
	std::string name_;
	std::vector<std::string> group_;
	std::vector<std::string> target_ip_;
	
	int port_;
	bool allow_loopback_=false;
	bool is_sleep_=false;
	ofxOscReceiver receiver_;
	std::string prefix_;
	
	std::map<std::string, Node> known_nodes_;
	std::vector<IPAddress::IPv4> self_ip_;
	
	bool need_heartbeat_=true;
	float heartbeat_request_interval_=1;
	float heartbeat_timeout_=3;
	struct TimerArgs {
		float timer;
		float limit;
	};
	std::map<std::string, TimerArgs> heartbeat_send_;
	std::map<std::string, TimerArgs> heartbeat_recv_;
	
	bool is_secret_mode_=false;
	std::string secret_key_;
	std::string getSelfIp(const std::string &hint);
};
