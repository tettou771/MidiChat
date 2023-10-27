#include "AudioLevelMonitor.h"

AudioLevelMonitor::AudioLevelMonitor(ofxWhisper *w) {
    setWhisper(w);
}

void AudioLevelMonitor::onStart() {
    
}

void AudioLevelMonitor::onDraw() {
    if (!whisper) return;
    
    bool isRecording = whisper->isRecording();
    float l = whisper->getAudioLevel();
    float t = whisper->getRrStartThreshold();
    
    auto r = getRect();
    
    // bg
    ofFill();
    ofSetColor(0);
    ofDrawRectangle(0, 0, r.width, r.height);

    // level bar with color
    isRecording ? ofSetColor(200, 0, 0) : ofSetColor(0, 200, 0);
    ofDrawRectangle(0, 0, r.width * l, r.height);

    // threshold
    ofSetColor(200, 200, 0);
    ofDrawLine(r.width * t, 0, r.width * t, r.height);
}

void AudioLevelMonitor::setWhisper(ofxWhisper *w) {
    ofAddListener(w->audioEvents, this, &AudioLevelMonitor::whisperAudioEvent);
    whisper = w;
}

void AudioLevelMonitor::whisperAudioEvent(ofxWhisper::AudioEventArgs &args) {
    
}

void AudioLevelMonitor::onMousePressedOverComponent(ofMouseEventArgs &mouse) {
    mouseEvent(mouse);
}

void AudioLevelMonitor::onMouseDraggedOverComponent(ofMouseEventArgs &mouse) {
    mouseEvent(mouse);
}

void AudioLevelMonitor::mouseEvent(ofMouseEventArgs &mouse) {
    if (!isMouseInside()) return;
    float t = ofMap(getMouseX(), 0, getWidth(), 0, 1.0);
    whisper->setRrStartThreshold(t);
    whisper->setRrEndThreshold(t);
}
