#pragma once
#include "ofxComponentUI.h"
#include "Thumbnail.h"
#include "ofxGoogleIME.h"
using namespace ofxComponent;

class TextArea : public ofxComponentBase {
public:
    void onStart() override;
    void onUpdate() override;
    void onDraw() override;
    void setString(string &str);
    void setStringDelay(string &str);
    
    static ofTrueTypeFont font;
    string message; // 適宜改行したmessage
    u32string u32message;
    float startTime;
        
    // 遅れてテキストがずらずら出るエフェクト
    bool delayText = false;
    int delayTextIndex;
};

class MessageObject : public ListElement {
public:
    MessageObject(ofJson json);
    
    void onStart() override;
    void onUpdate() override;
    void onDraw() override;;
    
    shared_ptr<TextArea> textArea;
    ofJson raw, data;
    ofJson midiJson;
    bool hasMidi = false;
    string role;
    bool isValid() {return valid;}
    
    string message;
private:
    bool valid = false;
};
