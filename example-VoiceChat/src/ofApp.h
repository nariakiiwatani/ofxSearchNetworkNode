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
	ofSoundStream stream_;
	std::map<std::string, ofSoundBuffer> buffer_;
	
	void messageReceived(ofxOscMessage &msg);
	
	void audioIn(ofSoundBuffer &buffer);
	void audioOut(ofSoundBuffer &buffer);
	
	mutex audio_mutex_, node_mutex_;
	ofSoundBuffer last_buffer_, buffer_to_send_;
	ofPolyline waveform_;
	float rms_;
};
