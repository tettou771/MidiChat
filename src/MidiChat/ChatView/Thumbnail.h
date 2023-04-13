#pragma once
#include "ofxComponentUI.h"
using namespace ofxComponent;

class Thumbnail : public ofxComponentBase {
public:
    Thumbnail(string &sequenceStr);
    
    void onDraw() override;
    void onMousePressedOverComponent(ofMouseEventArgs &mouse);
    ofFbo& getFbo() {return fbo;}
    
    // サムネイルがクリックされた時に発生するイベント
    static ofEvent<string> selectedEvents;
    string& getSequence() {
        return sequenceStr;
    }

private:
    string sequenceStr;
    ofFbo fbo;
};
