#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){	
	ofAddListener(node_.unhandledMessageReceived, this, &ofApp::messageReceived);
	node_.setAllowLoopback(true);
	node_.setup(9000);
	node_.request();
	
//	gui_.setup();
	
	ofBackground(ofColor::black);
	
	stream_.setup(this, 1, 1, 44100, 256, 16);
}

//--------------------------------------------------------------
void ofApp::update(){
	ofSoundBuffer buf;
	{
		unique_lock<mutex> lock(audio_mutex_);
		buf = last_buffer_;
		
		int send_size = 256;
		int buffers = 2;
		while(buffer_to_send_.size() >= send_size*buffers) {
			ofxOscMessage msg;
			msg.setAddress("/audio");
			auto &data = buffer_to_send_.getBuffer();
			ofBuffer blob;
			blob.append(reinterpret_cast<char*>(data.data()), send_size*sizeof(float));
			data.erase(begin(data), begin(data)+send_size);
			
			msg.addInt32Arg(send_size);
			msg.addInt32Arg(buffer_to_send_.getNumChannels());
			msg.addInt32Arg(buffer_to_send_.getSampleRate());
			msg.addBlobArg(blob);
			
			unique_lock<mutex> lock(node_mutex_);
			node_.sendMessage(msg);
		}
	}
	
	waveform_.clear();
	for(size_t i = 0; i < buf.getNumFrames(); i++) {
		float sample = buf.getSample(i, 0);
		float x = ofMap(i, 0, buf.getNumFrames(), 0, ofGetWidth());
		float y = ofMap(sample, -1, 1, 0, ofGetHeight());
		waveform_.addVertex(x, y);
	}
	
	rms_ = buf.getRMSAmplitude();
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofPushStyle();
	ofSetColor(ofColor::white);
	ofSetLineWidth(1 + (rms_ * 30.));
	waveform_.draw();
	ofPopStyle();
	
	/*
	gui_.begin();
	
	if(ImGui::Begin("Settings")) {
		char buf[256]={0};
		strcpy(buf, node_.getName().c_str());
		if(ImGui::InputText("enter your name", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			node_.setName(buf);
		}
		strcpy(buf, ofJoinString(node_.getGroup(),",").c_str());
		if(ImGui::InputText("enter room id", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			node_.setGroup(ofSplitString(buf,","), false);
		}
		if(ImGui::Button("Enter")) {
			unique_lock<mutex> lock(node_mutex_);
			node_.request();
		} ImGui::SameLine();
		if(ImGui::Button("Leave")) {
			unique_lock<mutex> lock(node_mutex_);
			node_.disconnect();
		}
	}
	ImGui::End();
	
	if(ImGui::Begin("Chat room")) {
		const auto &members = node_.getNodes();
		if(ImGui::TreeNode("Members", "Members(%lu)", members.size())) {
			for(const auto &member : members) {
				ImGui::Text("%s(%s)", member.second.name.c_str(), member.first.c_str());
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
	
	gui_.end();
	 */
}

void ofApp::messageReceived(ofxOscMessage &msg)
{
	int frames = msg.getArgAsInt32(0);
	int channels = msg.getArgAsInt32(1);
	int sample_rate = msg.getArgAsInt32(2);
	ofBuffer incoming = std::move(msg.getArgAsBlob(3));
	float *data = reinterpret_cast<float*>(incoming.getData());
	
	ofSoundBuffer buf;
	buf.copyFrom(data, frames, channels, sample_rate);
	
	unique_lock<mutex> lock(audio_mutex_);
	buffer_[msg.getRemoteIp()].append(buf);
}

void ofApp::audioIn(ofSoundBuffer &buffer)
{
	if(buffer.trimSilence()) {
		unique_lock<mutex> lock(audio_mutex_);
		buffer_to_send_.append(buffer);
	}
}
void ofApp::audioOut(ofSoundBuffer &buffer)
{
	buffer.set(0);
	unique_lock<mutex> lock(audio_mutex_);
	for(auto &b : buffer_) {
		auto &buf = b.second;
		if(buf.getNumFrames() < buffer.getNumFrames()) {
			continue;
		}
		int frames_to_copy = buffer.getNumFrames();
		int channels = buffer.getNumChannels();
		buf.addTo(buffer, frames_to_copy, channels, 0);
		auto &data = buf.getBuffer();
		data.erase(begin(data), begin(data)+buffer.size());
	}
	
	last_buffer_ = buffer;
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void ofApp::touchMoved(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void ofApp::touchUp(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void ofApp::touchDoubleTap(ofTouchEventArgs & touch){

}

//--------------------------------------------------------------
void ofApp::touchCancelled(ofTouchEventArgs & touch){
    
}

//--------------------------------------------------------------
void ofApp::lostFocus(){

}

//--------------------------------------------------------------
void ofApp::gotFocus(){

}

//--------------------------------------------------------------
void ofApp::gotMemoryWarning(){

}

//--------------------------------------------------------------
void ofApp::deviceOrientationChanged(int newOrientation){

}
