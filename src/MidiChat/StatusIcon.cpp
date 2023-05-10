#include "StatusIcon.h"

void StatusIcon::onStart() {
    // load image
    micIcon.load("icons/mic.png");
    chatgptIcon.load("icons/ChatGPT.png");
    errorIcon.load("icons/error.png");
    
    setStatus(WaitingForUser);
}

void StatusIcon::onDraw() {
    // circle
    ofSetColor(bgColor);
    ofFill();
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2);
    // draw outline (antialiasing)
    ofNoFill();
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2);

    // draw icon image
    if (currentIcon && currentIcon->isAllocated()) {
        ofSetColor(iconColor);
        float margin = getWidth() * 0.14;
        currentIcon->draw(margin, margin, getWidth() - margin * 2, getHeight() - margin * 2);
    }
}

void StatusIcon::setStatus(MidiChatStatus next) {
    switch (next) {
    case WaitingForUser:
        currentIcon = &micIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(100);
        break;
    case Recording:
        currentIcon = &micIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(255, 0, 0);
        break;
    case WaitingForWhisper:
        currentIcon = &chatgptIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(10, 80, 180);
        break;
    case WaitingForChatGPT:
        currentIcon = &chatgptIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(16, 163, 127);
        break;
    case Error:
        currentIcon = &errorIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(255, 0, 0);
        break;
    }

    statusChangedTime = ofGetElapsedTimef();
    status = next;
}
