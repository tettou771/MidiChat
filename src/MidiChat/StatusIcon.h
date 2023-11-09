#pragma once
#include "ofMain.h"
#include "ofxComponent.h"
#include "ofxWhisper.h"
using namespace ofxComponent;

// state
enum MidiChatStatus {
    Stop, // 音声入力しない状態
    Recording, // 音声入力中
    RecordingToChatGPT, // ChatGPT用に音声入力中
    WaitingForWhisper, // Whisperからの返信待ち（返信きたら自動遷移）
    WaitingForChatGPT, // ChatGPTからの返信待ち
    Error // 何らかのエラーで停止
};

class StatusIcon : public ofxComponentBase {
public:
    void onStart() override;
    void onDraw() override;
    void setStatus(MidiChatStatus next);
    void setWhisper(ofxWhisper *w);
    void whisperAudioEvent(ofxWhisper::AudioEventArgs& args);
    void setBpm(float _bpm) {bpm = _bpm;}
    
private:
    MidiChatStatus status;
    float loadingIndicatorBeginTime;
    bool loadingIndicatorShowing = false;
    
    ofImage micIcon, chatgptIcon, errorIcon;
    ofImage* currentIcon;
    
    ofColor bgColor, iconColor;
    
    // オーディオレベル表示
    float level;
    bool isRecording = false;
    
    // bpmに合わせてロードアテンションを動かす
    float bpm;
};
