/*
The MIT License (MIT)

Copyright (c) 2018 nariakiiwatani

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "ofParameter.h"
#include "ofxSearchNetworkNode.h"
#include "ofxPubSubOsc.h"

template<typename T>
class ofxSNNParamSync : public ofParameter<T>
{
public:
	virtual ~ofxSNNParamSync() {
		if(searcher_) {
			removeListener();
		}
	}
	void setNode(ofxSearchNetworkNode &search) {
		if(searcher_) {
			removeListener();
		}
		searcher_ = &search;
		addListener();
	}
	void setup(int port, const std::string &name) {
		name_ = name;
		port_ = port;
	}
	
	const T& getRemote(const std::string &ip) const { return remotes_[ip].value; }
	T& getRemote(const std::string &ip) { return remotes_[ip].value; }
protected:
	void addListener() {
		ofAddListener(searcher_->nodeFound, this, &ofxSNNParamSync::nodeConnected);
		ofAddListener(searcher_->nodeDisconnected, this, &ofxSNNParamSync::nodeDisconnected);
	}
	void removeListener() {
		ofRemoveListener(searcher_->nodeFound, this, &ofxSNNParamSync::nodeConnected);
		ofRemoveListener(searcher_->nodeDisconnected, this, &ofxSNNParamSync::nodeDisconnected);
	}
	void nodeConnected(const std::pair<std::string,ofxSearchNetworkNode::Node> &node) {
		const std::string &ip = node.first;
		auto &remote = remotes_[ip];
		remote.publisher = ofxPublishOsc(ip, port_, getAddress(), *this);
		remote.subscriber = ofxSubscribeOsc(port_, getAddress(), remote.value);
	}
	void nodeDisconnected(const std::pair<std::string,ofxSearchNetworkNode::Node> &node) {
		const std::string &ip = node.first;
		auto &remote = remotes_[ip];
		ofxUnpublishOsc(remote.publisher);
		ofxUnsubscribeOsc(remote.subscriber);
		remotes_.erase(ip);
	}

	std::string getAddress() const {
		return "/ofxSNNParamSync/" + name_ + "/set";
	}
	ofxSearchNetworkNode *searcher_=nullptr;
	std::string name_;
	int port_;
	struct Remote {
		ofxOscPublisherIdentifier publisher;
		ofxOscSubscriberIdentifier subscriber;
		T value;
	};
	std::map<std::string, Remote> remotes_;
};

