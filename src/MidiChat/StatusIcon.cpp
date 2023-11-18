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
    
    // サウンドのレベルでアイコンを明滅
    float alpha = ofMap(level, 0.02, 0.1, 110, 255, true);
    ofSetCircleResolution(40);
    ofFill();
    ofSetColor(0);
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2);
    ofSetColor(bgColor, alpha);
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2);
    // draw outline (antialiasing)
    ofNoFill();
    ofDrawCircle(getWidth()/2, getHeight()/2, getWidth()/2);

    /*
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
     */

    // draw icon image
    if (currentIcon && currentIcon->isAllocated()) {
        ofSetColor(iconColor);
        float margin = getWidth() * 0.11;
        currentIcon->draw(margin, margin, getWidth() - margin * 2, getHeight() - margin * 2);
    }
    
    // loading indicator
    if (loadingIndicatorShowing) {
        float time = ofGetElapsedTimef() - loadingIndicatorBeginTime;

        // どのロードインジケータにするか
        // 0: ドットパターン
        // 1: ラインが複数連結したパターン
        // 2: クルクルライン
        // 3: クルクルうねうねライン
        int pattern = 2;
        
        switch (pattern) {
        case 0: {
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
        }; break;
        case 1:{
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
        }; break;
        case 2: {
            float duration = 4; // sec
            if (bpm > 0) duration = 8 * 60. / bpm; // bpmがゼロじゃない時は、それに合わせる
            float t = fmod(time, duration) / duration;
            t *= 1.4;
            float offsetTheta = time * 4;
            
            // ラインの先端の角度
            float tanh1 = -t*t + 2*t;
            float theta1 = ofMap(tanh1, 0, 1, 0, TAU);

            // ラインの後端の角度
            float tanh2 = t*t*t*t;
            float theta2 = ofMap(tanh2, 0, 1, 0, TAU);
            
            // ラインの内側の半径
            float R1 = getWidth() * 0.55;

            // ラインの外側の半径
            float R2 = getWidth() * 0.60;
            

            if (theta2 < theta1) {
                // 描画の中心(アイコンの中心)
                ofPushMatrix();
                ofTranslate(getWidth() / 2, getHeight() / 2);

                ofMesh mesh;
                int N = 30;
                for (int j=0; j <= N; ++j) {
                    float th = ofMap(j, 0, N, theta1, theta2);
                    th += offsetTheta;
                    // 内側
                    mesh.addVertex(ofVec3f(R1 * sin(th), -R1 * cos(th), 0));
                    // 外側
                    mesh.addVertex(ofVec3f(R2 * sin(th), -R2 * cos(th), 0));
                }
                mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
                mesh.drawFaces();
                
                ofPopMatrix();
            }
            
        }; break;
        case 3: {
            float duration = 4; // sec
            if (bpm > 0) duration = 8 * 60. / bpm; // bpmがゼロじゃない時は、それに合わせる
            float t = fmod(time, duration) / duration;
            t *= 1.4;
            float offsetTheta = time * 2.5;
            
            // ラインの先端の角度
            float tanh1 = -t*t + 2*t;
            float theta1 = ofMap(tanh1, 0, 1, 0, TAU);

            // ラインの後端の角度
            float tanh2 = t*t*t*t;
            float theta2 = ofMap(tanh2, 0, 1, 0, TAU);
            
            // ラインの内側の半径
            float R1 = getWidth() * 0.55;

            // ラインの外側の半径
            float R2 = getWidth() * 0.65;
            

            if (theta2 < theta1) {
                // 描画の中心(アイコンの中心)
                ofPushMatrix();
                ofTranslate(getWidth() / 2, getHeight() / 2);

                ofMesh mesh;
                int N = 100;
                for (int j=0; j <= N; ++j) {
                    float th = ofMap(j, 0, N, theta1, theta2);
                    th += offsetTheta;
                    // 波打ち
                    float wave = sin(th * 9 + offsetTheta) * getWidth() * 0.03;
                    // 内側
                    mesh.addVertex(ofVec3f((R1 + wave) * sin(th), -(R1 + wave) * cos(th), 0));
                    // 外側
                    mesh.addVertex(ofVec3f((R2 + wave) * sin(th), -(R2 + wave) * cos(th), 0));
                }
                mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
                ofSetColor(200);
                mesh.drawFaces();
                
                ofPopMatrix();
            }
            
        }; break;
        default: break;
        }
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
        // loadingIndicatorNext = true; // debug インジケータの確認用
        currentIcon = &micIcon;
        iconColor = ofColor::white;
        //bgColor = ofColor(16, 163, 127); // ChatGPT色のマイク
        bgColor = ofColor(255, 0, 0); // 赤いマイク
        break;
    case WaitingForVoice:
        currentIcon = &micIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(255, 0, 0); // 赤いマイク
        loadingIndicatorNext = true;
        break;
    case WaitingForWhisper:
        currentIcon = &chatgptIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(16, 100, 180); // 青いロゴ
        loadingIndicatorNext = true;
        break;
    case WaitingForChatGPT:
        currentIcon = &chatgptIcon;
        iconColor = ofColor::white;
        bgColor = ofColor(16, 163, 127); // 緑のロゴ
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
