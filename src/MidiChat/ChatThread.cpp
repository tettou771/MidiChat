#include "ChatThread.h"

ChatThread::ChatThread() {
    
}

void ChatThread::setup(string model) {
    // setup chatGPT
    string apiKey;
    ofJson configJson = ofLoadJson("config.json");
    apiKey = configJson["apiKey"].get<string>();
    chatGPT.setup(apiKey);

    chatGPT.setSystem(GPT_Prompt());
    chatGPT.setModel(model);
}

void ChatThread::threadedFunction() {
    tuple<string, ofxChatGPT::ErrorCode> response;
    
    if (regenerateFlag) {
        response = chatGPT.chatRegenerate();
    }
    else {
        response = chatGPT.chatWithHistory(requestingMessage);
    }
    
    mutex.lock();
    availableMessages.push_back(response);
    mutex.unlock();
}

void ChatThread::chatWithHistoryAsync(string msg) {
    if (isThreadRunning()) return;
    requestingMessage = msg;
    ofLogNotice("Chat") << "chatWithHistoryAsync " << msg;
    regenerateFlag = false;
    startThread();
}

void ChatThread::regenerateAsync() {
    if (isThreadRunning()) return;
    
    regenerateFlag = true;
    ofLogNotice("Chat") << "regenerateAsync";
    startThread();
}

bool ChatThread::isWaiting() {
    return isThreadRunning();
}

bool ChatThread::hasMessage() {
    return availableMessages.size() > 0;
}

tuple<string, ofxChatGPT::ErrorCode> ChatThread::getMessage() {
    tuple<string, ofxChatGPT::ErrorCode> result;
    mutex.lock();
    if (availableMessages.size() > 0) {
        ofLogNotice("Chat") << "getMessage";
        result = availableMessages.front();
        availableMessages.erase(availableMessages.begin());
    } else {
        ofLogNotice("Chat") << "No message";
        result = tuple<string, ofxChatGPT::ErrorCode>("", ofxChatGPT::UnknownError);
    }
    mutex.unlock();
    return result;
}
