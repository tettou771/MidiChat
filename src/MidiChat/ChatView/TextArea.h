#pragma once
#include "ofxComponentUI.h"
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
    // 1秒あたりに表示する文字数
    // 文字数が多い時は早く設定する
    float cps; // char / sec
    
    ofColor color;
};