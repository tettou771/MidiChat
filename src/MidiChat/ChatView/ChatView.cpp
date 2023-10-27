#include "ChatView.h"

ChatView::ChatView() {
    
}

void ChatView::onSetup() {
}

void ChatView::onUpdate() {
    // 自動スクロールする
    if (autoScroll) {
        auto view = getParent()->getThisAs<ScrollView>();
        if (view) {
            view->scrollY(-3);
        }
    }
}

void ChatView::onDraw() {
    ofSetColor(0, 0, 40, 100);
    ofDrawRectangle(0, 0, getWidth(), getHeight());
    
}

void ChatView::onMouseScrolled(ofMouseEventArgs& mouse) {
    // 上にスクロールした時はautoScrollを無効化
    if (mouse.scrollY > 0) autoScroll = false;
    // 下にスクロールしていて、かつ下端にいる場合はautoScrollを有効化
    else if (getPos().y == getParent()->getHeight() - getHeight()) {
        autoScroll = true;
    }
}

void ChatView::addMessage(ofJson message, ofColor txtColor) {
    auto newMsgObj = make_shared<MessageObject>(message, txtColor);
    
    if (newMsgObj->isValid()) {
        messages.push_back(newMsgObj);
        // 仮にサイズを決めておく
        newMsgObj->setWidth(getWidth());
        newMsgObj->setHeight(80);
        addElement(newMsgObj);
        //updateMessagesPosition();
    }
    else {
        newMsgObj->destroy();
    }
        
    // オブジェクトの位置を調整
    updateMessagesPosition();
}

void ChatView::addMessageObject(shared_ptr<MessageObject> mObj) {
    if (!mObj) return;
    addElement(mObj);
}

void ChatView::updateMessagesPosition() {
    updateList();
    return;
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

void ChatView::scrollToLast() {
    ofVec2f p = getPos();
    p.y = getHeight() - getParent()->getHeight();
    setPos(p);
}
