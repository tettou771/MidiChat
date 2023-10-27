#pragma once
#include "ofxComponentUI.h"
#include "ofxWhisper.h"
#include "ChatThread.h"
#include "Midi/SequencerView.h"
#include "ChatView/ChatView.h"
#include "StatusIcon.h"
#include "AudioLevelMonitor.h"

using namespace ofxComponent;

class MidiChat: public ofxComponentBase {
public:
    MidiChat();
    void onSetup() override;
	void onUpdate() override;
	void onDraw() override;
    void onKeyPressed(ofKeyEventArgs &key) override;
    void onLocalMatrixChanged() override;

    shared_ptr<SequencerView> sequencerView;
    shared_ptr<ChatView> chatView;
    
    // メッセージを送信
    void sendMessage(string& message);
    
    // 応答がないときに再送するためのメソッド
    // すでにassistantから返信があった場合は、それは削除される
    void regenerate();
    
    // 生成を中止
    void cancel();
    
    // 再生開始
    void startMidi(); // シーケンスの頭から
    void stopMidi();

    ChatThread chat;
    
    // 保存用のファイル
    ofFile logFile;
    void makeLogFile();
    void writeToLogFile(const string& message);

    // whisper
    ofxWhisper whisper;
    
    shared_ptr<InfoObject> regenerateButton = nullptr;
    
    MidiChatStatus status = Stop;
    void setState(MidiChatStatus next);
    shared_ptr<StatusIcon> statusIcon;
    shared_ptr<AudioLevelMonitor> audioLevelMonitor;
    void onStatusIconClicked();
    
private:
    shared_ptr<MessageObject> transcriptingObject = nullptr;
    void newTranscriptObject(const string& transcript);
    void addToTranscriptingObject(const string& transcript);
    void sendTranscriptingObject();
    bool isTranscriptingObjectEmpty();
    string gptSystemPrompt;
    
    // スペースキーとクリックに対応
    // 次の状態に遷移する
    void nextState();
    
    string MidiChatStatusToString(const MidiChatStatus &s) {
        switch (s) {
        case Stop: return "Stop";
        case Recording: return "Recording";
        case RecordingToChatGPT: return "RecordingToChatGPT";
        case WaitingForChatGPT: return "WaitingForChatGPT";
        case Error: return "Error";
        default: return "";
        }
    };
};
