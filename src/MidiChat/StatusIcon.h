#pragma once
#include "ofMain.h"
#include "ofxComponent.h"
using namespace ofxComponent;

// state
enum MidiChatStatus {
    WaitingForUser, // 声のコマンドの入力待ち
    Recording, // 声のコマンドを入力中
    WaitingForWhisper, // Whisperで変換中
    WaitingForChatGPT, // ChatGPTからの返信待ち
    Error // 何らかのエラーで停止
};

class StatusIcon : public ofxComponentBase {
public:
    void onStart() override;
    void onDraw() override;
    void setStatus(MidiChatStatus next);
    
private:
    MidiChatStatus status;
    float loadingIndicatorBeginTime;
    bool loadingIndicatorShowing = false;
    
    ofImage micIcon, chatgptIcon, errorIcon;
    ofImage* currentIcon;
    
    ofColor bgColor, iconColor;
};
