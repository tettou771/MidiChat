#pragma once
#include "ofxComponent.h"
#include "MessageObject.h"
#include "ofxChatGPT.h"
using namespace ofxComponent;

class ChatView : public ListBox {
public:
    ChatView();
    
    void onSetup() override;
    void onUpdate() override;
    void onDraw() override;
    void onMouseScrolled(ofMouseEventArgs& mouse) override;
    
    void addMessage(ofJson message, ofColor txtColor);
    void addMessageObject(shared_ptr<MessageObject> mObj);
    void updateMessagesPosition();
    shared_ptr<MessageObject> getLastMessageObject();
    void deleteLastAssistantMessage();
    void scrollToLast();
    
private:
    vector<shared_ptr<MessageObject> > messages;
    bool autoScroll = true; // 自分でスクロールしてない時、下にスクロールし続ける
};
