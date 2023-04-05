#include "ChatView.h"

ChatView::ChatView() {
    
}

void ChatView::onSetup() {
    bool debug = false;
    //for (int i=0;i<20;++i) {
    if (debug) {
        ofJson newDebugMessage;
        newDebugMessage["message"]["role"] = "assistant";
        newDebugMessage["message"]["content"] = R"({
    "message": "This is a white triangle on black.",
    "image": {
    "width": 16,
    "height": 16,
    "pixels": [
    "0000000110000000",
    "0000000110000000",
    "0000001111000000",
    "0000001111000000",
    "0000011111100000",
    "0000011111100000",
    "0000111111110000",
    "0000111111110000",
    "0001111111111000",
    "0001111111111000",
    "0011111111111100",
    "0011111111111100",
    "0111111111111110",
    "0111111111111110",
    "1111111111111111",
    "1111111111111111"
    ]
    },
    "error": "Write any error messages here if there is an issue"
    })";
        addMessage(newDebugMessage);
    }
}

void ChatView::onUpdate() {
    
}

void ChatView::onDraw() {
    //ofSetColor(40, 120);
    //ofDrawRectangle(0, 0, getWidth(), getHeight());
    
}

void ChatView::addMessage(ofJson message) {
    auto newMsgObj = make_shared<MessageObject>(message);
    
    if (newMsgObj->isValid()) {
        messages.push_back(newMsgObj);
        // 仮にサイズを決めておく
        newMsgObj->setWidth(getWidth());
        newMsgObj->setHeight(100);
        addElement(newMsgObj);
        //updateMessagesPosition();
    }
    else {
        newMsgObj->destroy();
    }
    
    // メッセージを追加するたびに下にスクロールする
    updateMessagesPosition();
    
    auto view = getParent()->getThisAs<ScrollView>();
    if (view) {
        view->scrollY(-100);
    }
}

void ChatView::updateMessagesPosition() {
    int y = 0;
    for (auto &m : messages) {
        m->setPos(0, y);
        y+= m->getHeight();
    }
}

shared_ptr<MessageObject> ChatView::getLastMessageObject() {
    if (messages.empty()) return nullptr;
    else return messages.back();
}
