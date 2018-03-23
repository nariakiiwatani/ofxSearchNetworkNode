#include "ofApp.h"

using namespace std;

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
	
	if(ImGui::Begin("Hosts")) {
		const auto &members = node_.getNodes();
		for(const auto &member : members) {
			const auto &ip = member.first;
			const auto &name = member.second.name;
			auto &box_info = boxes_[ip];
			ImGui::Checkbox(name.c_str(), &box_info.is_open); ImGui::SameLine();
			ImGui::Text("(%s)", ip.c_str());
			if(box_info.is_open) {
				if(ImGui::Begin(name.c_str(), &box_info.is_open)) {
					if(ImGui::CollapsingHeader("Received Files")) {
						for(auto &info : box_info.recv_files) {
							ImGui::PushID(&info);
							ImGui::Text("%s", info.second.name.c_str()); ImGui::SameLine();
							if(info.second.isCompleted()) {
								if(ImGui::Button("save")) {
									auto result = ofSystemSaveDialog(info.second.name, "");
									if(result.bSuccess) {
										ofBufferToFile(result.getPath(), info.second.buffer);
									}
								}
							}
							else if(info.second.is_receiving) {
								if(ImGui::Button("cancel")) {
									info.second.is_receiving = false;
								}
								else {
									ImGui::Text("%llu/%ld", info.second.received_size, info.second.buffer.size());
									sendRequest(ip, info.first, info.second.received_size);
								}
							}
							else {
								if(ImGui::Button("download")) {
									info.second.is_receiving = true;
								}
							}
							ImGui::PopID();
						}
					}
					if(ImGui::CollapsingHeader("Send Files")) {
						for(auto &info : box_info.send_files) {
							ImGui::Text("%s", info.second.file.path().c_str());
						}
					}
					if(ImGui::IsWindowHovered()) {
						if(!drag_files_.empty()) {
							for(auto &path : drag_files_) {
								ofFile file(path);
								if(file.isFile()) {
									notifyFileIsReady(ip, path);
								}
							}
						}
					}
				}
				ImGui::End();
			}
		}
	}
	ImGui::End();
	gui_.end();
	
	drag_files_.clear();
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
	uint32_t crc32(const std::string &str) {
		return crc32(str.c_str(), str.length());
	}
}

void ofApp::notifyFileIsReady(const std::string &ip, const std::string &filepath)
{
	SendFile info(ip, filepath);
	if(!info.file.exists()) {
		return;
	}
	auto it = boxes_.find(ip);
	if(it == end(boxes_)) { return; }
	auto &send_files = it->second.send_files;
	uint32_t hash = crc32(ip+filepath);
	send_files.insert(std::make_pair(hash, info));
	ofxOscMessage msg;
	msg.setAddress("/file/info");
	msg.addInt32Arg(hash);
	msg.addInt64Arg(info.file.getSize());
	msg.addStringArg(ofFilePath::getFileName(filepath));
	node_.sendMessage(ip, msg);
}

void ofApp::sendRequest(const std::string &ip, unsigned int identifier, uint64_t position)
{
	ofxOscMessage msg;
	msg.setAddress("/file/request");
	msg.addInt32Arg(identifier);
	msg.addInt64Arg(position);
	msg.addInt64Arg(RECV_MAXSIZE);
	node_.sendMessage(ip, msg);
}

void ofApp::messageReceived(ofxOscMessage &msg)
{
	const std::string &address = msg.getAddress();
	if(address == "/file/info") {
		const std::string &ip = msg.getRemoteIp();
		auto &box = boxes_[ip];
		RecvFile file(msg.getArgAsString(2), msg.getArgAsInt64(1));
		auto identifier = msg.getArgAsInt32(0);
		box.recv_files.insert(std::make_pair(identifier, file));
	}
	else if(address == "/file/request"){
		std::string ip = msg.getRemoteIp();
		auto it = boxes_.find(ip);
		if(it == end(boxes_)) { return; }
		auto &send_files = it->second.send_files;
		uint32_t identifier = msg.getArgAsInt32(0);
		auto it2 = send_files.find(identifier);
		if(it2 == end(send_files)) {
			return;
		}
		auto &info = it2->second;
		uint64_t position = msg.getArgAsInt64(1);
		uint64_t maxsize = min<uint64_t>(msg.getArgAsInt64(2), SEND_MAXSIZE);
		uint64_t size = min<uint64_t>(maxsize, info.file.getSize()-position);
		ofBuffer buffer;
		info.file.seekg(position, ios_base::beg);
		buffer.allocate(size);
		info.file.read(buffer.getData(), buffer.size());
		ofxOscMessage msg;
		msg.setAddress("/file/data");
		msg.addInt32Arg(identifier);
		msg.addInt64Arg(position);
		msg.addBlobArg(buffer);
		node_.sendMessage(info.destination, msg);
	}
	else if(address == "/file/data") {
		std::string ip = msg.getRemoteIp();
		uint32_t identifier = msg.getArgAsInt32(0);
		uint64_t position = msg.getArgAsInt64(1);
		const ofBuffer &data = msg.getArgAsBlob(2);
		auto it = boxes_.find(ip);
		if(it == end(boxes_)) { return; }
		auto it2 = it->second.recv_files.find(identifier);
		if(it2 == end(it->second.recv_files)) { return; }
		auto &info = it2->second;
		if(!info.is_receiving) { return; }
		if(info.isCompleted()) { return; }
		auto *ptr = info.buffer.getData();
		auto size = data.size();
		assert(position+size <= info.buffer.size());
		memcpy(ptr+position, data.getData(), size);
		info.received_size = position+size;
	}
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
	drag_files_ = dragInfo.files;
}
