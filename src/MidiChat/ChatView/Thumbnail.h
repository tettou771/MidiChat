#pragma once
#include "ofxComponentUI.h"
using namespace ofxComponent;

class Thumbnail : public ofxComponentBase {
public:
    Thumbnail(ofJson &j);
    
    void onDraw() override;
    void onMousePressedOverComponent(ofMouseEventArgs &mouse);
    ofFbo& getFbo() {return fbo;}
    
    // サムネイルがクリックされた時に発生するイベント
    static ofEvent<ofJson> selectedEvents;
    ofJson& getJson() {
        return json;
    }

private:
    ofJson json;
    ofFbo fbo;
};
