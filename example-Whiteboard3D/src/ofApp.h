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
	struct Pen {
		ofFloatColor color=ofColor::black;
		float width=4;
		bool is_writing=false;
	} pen_;
	ofEasyCam camera_;
	ofVec3f getCurrentWorldPosition();

	std::deque<ofxOscMessage> log_;
	bool is_through_bundle_=false;
	int bundle_length_=0;
	void messageReceived(ofxOscMessage &msg);
	void newNodeFound(const std::pair<std::string, ofxSearchNetworkNode::Node> &node);
	
	// create message
	void startStroke(ofVec3f pos);
	void stroke(ofVec3f pos);
	void endStroke(ofVec3f pos);
	void clear();
};
