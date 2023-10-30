#include "StatusIcon.h"

void StatusIcon::onStart() {
    // load image
    micIcon.load("icons/mic.png");
    chatgptIcon.load("icons/ChatGPT.png");
    errorIcon.load("icons/error.png");
    level = 0;

    setStatus(status);
}

void StatusIcon::onDraw() {
    // circle
    ofSetColor(bgColor);
    ofFill();
    ofSetCircleResolution(40);
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2);
    // draw outline (antialiasing)
    ofNoFill();
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2);

    // audio level
    if (isRecording) {
        ofSetColor(ofColor::fromHsb(0 * 255, 0.8 * 255, level * 10 * 255));
    } else{
        ofSetColor(ofColor::fromHsb(0.25 * 255, 0.8 * 255, level * 10 * 255));
    }
    ofNoFill();
    ofSetLineWidth(4);
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2 + 2);
    ofSetLineWidth(1);

    // draw icon image
    if (currentIcon && currentIcon->isAllocated()) {
        ofSetColor(iconColor);
        float margin = getWidth() * 0.14;
        currentIcon->draw(margin, margin, getWidth() - margin * 2, getHeight() - margin * 2);
    }
    
    // loading indicator
    if (loadingIndicatorShowing) {
        float time = ofGetElapsedTimef() - loadingIndicatorBeginTime;
        const int dotNum = 10;
        
        float duration = 5;
        float t = fmod(time, duration) / duration;
        // ドットの半径
        float rMax = 5;
        // ドットの軌道の半径
        float R = getWidth() * 0.65;
        // ドットが出てくる角度
        float theta1 = ofMap(t, 0, 0.3, 0, TAU);
        // ドットが消える角度
        float theta2 = ofMap(t, 0.4, 0.75, 0, TAU);
        ofVec2f center(getWidth() / 2, getHeight() / 2);
        // ドットの色
        ofSetColor(200);
        ofFill();
        
        for (int i=0; i<dotNum; ++i) {
            float dotTheta = (float)i / dotNum * TAU;
            if (theta2 <= dotTheta && dotTheta <= theta1) {
                float s = MIN(1, MIN(abs(dotTheta - theta2), abs(dotTheta - theta1)) / TAU * 6);
                if (s > 0) {
                    float r = s * rMax;
                    ofVec2f p = center;
                    p.x += R * sin(dotTheta);
                    p.y += -R * cos(dotTheta);
                    ofDrawCircle(p, r);
                }
            }
        }
        
        /*
        for (int i=0; i<dotNum; ++i) {
            float dotTheta = (float)(i + 1) / dotNum * TAU;
            if (theta2 <= dotTheta && dotTheta <= theta1) {
                float s = MIN(1, MIN(abs(dotTheta - theta2), abs(dotTheta - theta1)) / TAU * 5);
                if (s > 0) {
                    int N = MAX(2, s * 5);
                    float drawTheta = s * TAU / dotNum;
                    ofBeginShape();
                    for (int j=0; j <= N; ++j) {
                        float th = -drawTheta * j / N;
                        ofVec2f p = center;
                        p.x += R * sin(dotTheta + th);
                        p.y += -R * cos(dotTheta + th);
                        ofVertex(p);
                    }
                    ofEndShape(OF_PRIMITIVE_LINES);
                }
            }
        }
         */
    }
}

void StatusIcon::setStatus(MidiChatStatus next) {
    bool loadingIndicatorNext = false;
    
    switch (next) {
    case Stop:
        currentIcon = &micIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(100);
        break;
    case Recording:
        currentIcon = &micIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(255, 0, 0); // 赤いマイク
        break;
    case RecordingToChatGPT:
        currentIcon = &micIcon;
        iconColor = ofColor::white;
        //bgColor = ofColor(16, 163, 127); // ChatGPT色のマイク
        bgColor = ofColor(255, 0, 0); // 赤いマイク
        break;
    case WaitingForWhisper:
        currentIcon = &chatgptIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(10, 120, 100);
        loadingIndicatorNext = true;
        break;
    case WaitingForChatGPT:
        currentIcon = &chatgptIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(16, 163, 127);
        loadingIndicatorNext = true;
        break;
    case Error:
        currentIcon = &errorIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(255, 0, 0);
        break;
    }

    // loadingIndicatorの状態が変わったときだけ、タイミング（クルクルの位相）をリセットする
    if (loadingIndicatorNext && !loadingIndicatorShowing) {
        loadingIndicatorBeginTime = ofGetElapsedTimef();
    }
    loadingIndicatorShowing = loadingIndicatorNext;
    status = next;
}

void StatusIcon::setWhisper(ofxWhisper *w) {
    ofAddListener(w->audioEvents, this, &StatusIcon::whisperAudioEvent);
}

void StatusIcon::whisperAudioEvent(ofxWhisper::AudioEventArgs &args) {
    level = args.audioLevel;
    isRecording = args.isRecording;
}
