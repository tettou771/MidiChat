#include "ChatView.h"

ChatView::ChatView() {
    
}

void ChatView::onSetup() {
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
        
    // オブジェクトの位置を調整
    updateMessagesPosition();
    
    // TODO
    // メッセージを追加するたびに下にスクロールする
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

void ChatView::deleteLastAssistantMessage() {
    // 最後のオブジェクトがassistantだったらそれを消す
    if (!messages.empty() && messages.back()->getRole() == "assistant") {
        messages.back()->destroy();
        messages.erase(messages.end() - 1);
    }
}
