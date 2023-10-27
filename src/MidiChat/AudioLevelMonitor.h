#pragma once
#include "ofMain.h"
#include "ofxComponent.h"
#include "ofxWhisper.h"
using namespace ofxComponent;

class AudioLevelMonitor : public ofxComponentBase {
public:
    AudioLevelMonitor(ofxWhisper *w);
    void onStart() override;
    void onDraw() override;
    void setWhisper(ofxWhisper *w);
    void whisperAudioEvent(ofxWhisper::AudioEventArgs& args);

    void onMousePressedOverComponent(ofMouseEventArgs&);
    void onMouseDraggedOverComponent(ofMouseEventArgs&);

private:
    ofxWhisper *whisper;
    
    void mouseEvent(ofMouseEventArgs&);
};
