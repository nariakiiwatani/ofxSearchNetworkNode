#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(255);
	
	ofAddListener(node_.nodeFound, this, &ofApp::newNodeFound);
	ofAddListener(node_.unhandledMessageReceived, this, &ofApp::messageReceived);
	node_.setAllowLoopback(true);
	node_.setup(9000);
	node_.request();
	
	gui_.setup();
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
	struct Stroke {
		ofColor color;
		float width;
		ofVec2f point;
	};
	std::map<std::string, Stroke> strokes;
	for(auto &msg : log_) {
		Stroke stroke;
		stroke.color = ofColor::fromHex(msg.getArgAsInt32(3));
		stroke.width = msg.getArgAsFloat(2);
		stroke.point = ofVec2f(msg.getArgAsFloat(0), msg.getArgAsFloat(1));
		auto result = strokes.insert(std::make_pair(msg.getRemoteIp(), stroke));
		if(!result.second) {
			auto &prev_stroke = result.first->second;
			if(msg.getAddress() != "/start") {
				ofPushStyle();
				ofSetColor(prev_stroke.color);
				ofSetLineWidth(prev_stroke.width);
				ofDrawLine(prev_stroke.point, stroke.point);
				ofPopStyle();
			}
			prev_stroke = stroke;
		}
	}
	
	gui_.begin();
	
	if(ImGui::Begin("Settings")) {
		char buf[256]={0};
		strcpy(buf, node_.getName().c_str());
		if(ImGui::InputText("enter your name", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			node_.setName(buf);
		}
		strcpy(buf, ofJoinString(node_.getGroup(),",").c_str());
		if(ImGui::InputText("enter board id", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			node_.setGroup(ofSplitString(buf,","), false);
		}
		if(ImGui::Button("Enter")) {
			node_.request();
		} ImGui::SameLine();
		if(ImGui::Button("Leave")) {
			node_.disconnect();
			log_.clear();
			bundle_length_ = 0;
		}
		ImGui::ColorEdit3("color", &pen_.color.r);
		ImGui::SliderFloat("width", &pen_.width, 1, 8);
		if(ImGui::Button("clear")) {
			clear();
		}
	}
	ImGui::End();
	
	gui_.end();
}

void ofApp::messageReceived(ofxOscMessage &msg)
{
	const std::string address = msg.getAddress();
	if(address == "/clear") {
		log_.clear();
		return;
	}
	if(address == "/bundle") {
		int bundle_length = msg.getArgAsInt(0);
		if(bundle_length_ < bundle_length) {
			log_.clear();
			is_through_bundle_ = false;
		}
		else {
			is_through_bundle_ = true;
		}
		bundle_length_ = bundle_length;
		return;
	}
	if(is_through_bundle_) {
		if(--bundle_length_ <= 0) {
			is_through_bundle_ = false;
		}
	}
	else {
		log_.emplace_back(msg);
	}
}

void ofApp::startStroke(ofVec2f pos)
{
	ofxOscMessage msg;
	msg.setAddress("/start");
	msg.addFloatArg(pos.x);
	msg.addFloatArg(pos.y);
	msg.addFloatArg(pen_.width);
	msg.addInt32Arg(pen_.color.getHex());
	node_.sendMessage(msg);
	pen_.is_writing = true;
}
void ofApp::stroke(ofVec2f pos)
{
	ofxOscMessage msg;
	msg.setAddress("/stroke");
	msg.addFloatArg(pos.x);
	msg.addFloatArg(pos.y);
	msg.addFloatArg(pen_.width);
	msg.addInt32Arg(pen_.color.getHex());
	node_.sendMessage(msg);
}
void ofApp::endStroke(ofVec2f pos)
{
	stroke(pos);
	pen_.is_writing = false;
}
void ofApp::clear()
{
	ofxOscMessage msg;
	msg.setAddress("/clear");
	node_.sendMessage(msg);
}

void ofApp::newNodeFound(const std::pair<std::string, ofxSearchNetworkNode::Node> &node)
{
	if(node_.isSelfIp(node.first)) {
		return;
	}
	if(log_.empty()) {
		return;
	}
	ofxOscBundle bundle;
	ofxOscMessage first_msg;
	first_msg.setAddress("/bundle");
	first_msg.addIntArg(log_.size());
	bundle.addMessage(first_msg);
	for(auto &msg : log_) {
		bundle.addMessage(msg);
	}
	node_.sendBundle(node.first, bundle);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	if(pen_.is_writing) {
		stroke(ofVec2f(x,y));
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	if(!ImGui::GetIO().WantCaptureMouse) {
		startStroke(ofVec2f(x,y));
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	if(pen_.is_writing) {
		endStroke(ofVec2f(x,y));
	}
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
