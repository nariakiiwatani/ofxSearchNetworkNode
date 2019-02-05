#pragma once

#include "ofMain.h"
#include "ofxSearchNetworkNode.h"
#include "ofxImGui.h"

class ofApp : public ofBaseApp{
	
public:
	void setup();
	void update();
	void draw();
	
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
private:
	ofxSearchNetworkNode node_;
	ofxImGui::Gui gui_;
	void messageReceived(ofxOscMessage &msg);
	void notifyFileIsReady(const std::string &ip, const std::string &filepath);
	void sendRequest(const std::string &ip, unsigned int identifier, uint64_t position);
	
	struct RecvFile {
		bool is_receiving=false;
		bool isCompleted() {
			return received_size == buffer.size();
		}
		std::string name;
		ofBuffer buffer;
		uint64_t received_size;
		RecvFile(std::string name, uint64_t size):name(name),received_size(0) {
			buffer.allocate(size);
		}
	};
	struct SendFile {
		std::string destination;
		ofFile file;
		SendFile(std::string d, std::string f):destination(d),file(f){}
	};
	struct FileBox {
		bool is_open;
		std::map<uint32_t, SendFile> send_files;
		std::map<uint32_t, RecvFile> recv_files;
	};
	std::map<std::string, FileBox> boxes_;
	
	std::vector<std::string> drag_files_;
	
	uint64_t SEND_MAXSIZE;
	uint64_t RECV_MAXSIZE;
};
