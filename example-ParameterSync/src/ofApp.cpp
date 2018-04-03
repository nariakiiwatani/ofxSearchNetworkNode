#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	node_.setAllowLoopback(true);
	node_.setup(9000);
	node_.request();
	
	gui_.setup();
	
	my_value_.setNode(node_);
	my_value_.setup(10000, "my value");
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
		strcpy(buf, ofJoinString(node_.getGroup(),",").c_str());
		if(ImGui::InputText("enter room id", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
			node_.setGroup(ofSplitString(buf,","), false);
		}
		if(ImGui::Button("Enter")) {
			node_.request();
		} ImGui::SameLine();
		if(ImGui::Button("Leave")) {
			node_.disconnect();
		}
		float value = my_value_.get();
		if(ImGui::SliderFloat("my value", &value, 0, 1)) {
			my_value_.set(value);
		}
	}
	ImGui::End();
	
	if(ImGui::Begin("Chat room")) {
		const auto &members = node_.getNodes();
		if(ImGui::TreeNode("Members", "Members(%lu)", members.size())) {
			for(const auto &member : members) {
				ImGui::Text("%s(%s)", member.second.name.c_str(), member.first.c_str());
				ImGui::SliderFloat("value", &my_value_.getRemote(member.first), 0, 1);
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
	
	gui_.end();
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
