#pragma once

#include "ofxiOS.h"
#include "ofxSearchNetworkNode.h"
#include "ofxImGui.h"

class ofApp : public ofxiOSApp {
	
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void touchDown(ofTouchEventArgs & touch);
	void touchMoved(ofTouchEventArgs & touch);
	void touchUp(ofTouchEventArgs & touch);
	void touchDoubleTap(ofTouchEventArgs & touch);
	void touchCancelled(ofTouchEventArgs & touch);
	
	void lostFocus();
	void gotFocus();
	void gotMemoryWarning();
	void deviceOrientationChanged(int newOrientation);
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


