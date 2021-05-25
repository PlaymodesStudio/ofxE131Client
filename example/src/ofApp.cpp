#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	node.setup("192.168.1.22", "255.255.255.0");
	node.setUseFramerate(false);
	node.setSourceName("ofxE131ClientExample");
}

//--------------------------------------------------------------
void ofApp::update(){
	vector<u_char> data = {static_cast<unsigned char>(round(ofRandom(0, 1)) * 255),
		static_cast<unsigned char>(round(ofRandom(0, 1)) * 255),
		static_cast<unsigned char>(round(ofRandom(0, 1)) * 255)};
	node.setChannels(1, data.data(), data.size(), 1);
	node.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

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
