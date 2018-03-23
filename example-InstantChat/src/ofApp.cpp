#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
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
	gui_.begin();
	
	if(ImGui::Begin("Settings")) {
		char buf[256]={0};
		strcpy(buf, node_.getName().c_str());
		if(ImGui::InputText("enter your name", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			node_.setName(buf);
		}
		strcpy(buf, node_.getGroup().c_str());
		if(ImGui::InputText("enter room id", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			node_.setGroup(buf, false);
		}
		if(ImGui::Button("Enter")) {
			node_.request();
		} ImGui::SameLine();
		if(ImGui::Button("Leave")) {
			node_.disconnect();
		}
		ImGui::InputInt("Log length", &log_length_);
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
		static char message[1024]={0};
		if(ImGui::InputText("message", message, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
			ofxOscMessage msg;
			msg.setAddress("/message");
			msg.addStringArg(message);
			node_.sendMessage(msg);
			message[0] = 0;
		}
		for(auto &l : log_) {
			std::string name = "unknown";
			const auto &member = members.find(l.ip);
			if(member != end(members)) {
				name = member->second.name;
			}
			ImGui::Text("%s(%s):%s", name.c_str(), l.ip.c_str(), l.message.c_str());
		}
		
	}
	ImGui::End();
	
	gui_.end();
}

void ofApp::messageReceived(ofxOscMessage &msg)
{
	while(log_.size() > log_length_) {
		log_.pop_back();
	}
	MessageLog log;
	log.ip = msg.getRemoteIp();
	log.message = msg.getArgAsString(0);
	log_.emplace_front(log);
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

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

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
