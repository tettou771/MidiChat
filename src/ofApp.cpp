#include "ofApp.h"
#include "MidiChat/MidiChat.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("MIDI Chat");
    ofLogToConsole();
    
#ifdef TARGET_OS_MAC
    ofSetDataPathRoot("../Resources/data");
#endif
    
    componentManager = make_shared<ofxComponentManager>();
    
    auto midiChat = make_shared<MidiChat>();
    componentManager->addChild(midiChat);
    componentManager->setup();
}

//--------------------------------------------------------------
void ofApp::update(){

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
