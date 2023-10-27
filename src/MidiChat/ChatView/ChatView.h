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
    
    void addMessage(ofJson message);
    void addMessageObject(shared_ptr<MessageObject> mObj);
    void updateMessagesPosition();
    shared_ptr<MessageObject> getLastMessageObject();
    void deleteLastAssistantMessage();
    
private:
    vector<shared_ptr<MessageObject> > messages;
};
