#pragma once
#include "ofxComponentUI.h"
#include "ofxChatGPT.h"
#include "Midi/SequencerView.h"
#include "ChatView/ChatView.h"
#include "ofxGoogleIME.h"

using namespace ofxComponent;

class Chat : public ofThread {
public:
    Chat();
    
    void setup();
    void threadedFunction() override;
    
    // check waiting for GPT response
    bool isWaiting();
    bool hasMessage();
    
    // リクエストを送信
    void chatWithHistoryAsync(string msg);
    
    // メッセージがあれば、それをgetできる
    // getされたものはリストから削除される
    tuple<string, ofxChatGPT::ErrorCode> getMessage();
    
private:
    ofxChatGPT chatGPT;
    string requestingMessage;
    ofMutex mutex;
    vector<tuple<string, ofxChatGPT::ErrorCode> > availableMessages;
};

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
    
    //
    void sendMessage();

    Chat chat;
    
    ofxGoogleIME ime;
private:
};
