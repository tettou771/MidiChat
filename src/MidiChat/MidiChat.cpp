#include "MidiChat.h"
#include "prompt.h"

Chat::Chat() {
    
}

void Chat::setup() {
    // setup chatGPT
    string apiKey;
    ofJson configJson = ofLoadJson("config.json");
    apiKey = configJson["apiKey"].get<string>();
    chatGPT.setup(apiKey);

    vector<string> models;
    ofxChatGPT::ErrorCode err;
    tie(models, err) = chatGPT.getModelList();
    ofLogNotice("Chat") << "Available OpenAI GPT models:";
    for (auto model : models) {
        // GPT系のモデルだけをリストアップ
        if (ofIsStringInString(model, "gpt")) {
            ofLogNotice("Chat") << model;
        }
    }
    
    // send system message
    chatGPT.setSystem(GPT_Prompt());
    chatGPT.setModel("gpt-3.5-turbo");
    //chatGPT.setModel("gpt-4");
}

void Chat::threadedFunction() {
    auto response = chatGPT.chatWithHistory(requestingMessage);
    
    mutex.lock();
    availableMessages.push_back(response);
    mutex.unlock();
}

void Chat::chatWithHistoryAsync(string msg) {
    if (isThreadRunning()) return;
    requestingMessage = msg;
    ofLogNotice("Chat") << "chatWithHistoryAsync " << msg;
    startThread();
}

bool Chat::isWaiting() {
    return isThreadRunning();
}

bool Chat::hasMessage() {
    return availableMessages.size() > 0;
}

tuple<string, ofxChatGPT::ErrorCode> Chat::getMessage() {
    mutex.lock();
    if (availableMessages.size() > 0) {
        ofLogNotice("Chat") << "getMessage";
        auto msg = availableMessages.front();
        availableMessages.erase(availableMessages.begin());
        return msg;
    } else {
        return tuple<string, ofxChatGPT::ErrorCode>("", ofxChatGPT::UnknownError);
    }
    mutex.unlock();
}


MidiChat::MidiChat() {
    
}

void MidiChat::onSetup(){
    //ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetLogLevel(OF_LOG_NOTICE);
    
    // show in scroll view
    chatView = make_shared<ChatView>();
    chatView->setAlign(ListBox::Align::FitWidth); // 要素を幅で合わせる
    auto scrollView = make_shared<ScrollView>(ScrollView::FitWidth);
    scrollView->setRect(ofRectangle(100, 100, 1000, 1000));
    scrollView->setContents(chatView);
    addChild(scrollView);
    
    sequencerView = make_shared<SequencerView>();
    sequencerView->setRect(ofRectangle(1200, 100, 700, 1000));
    addChild(sequencerView);
    
    chat.setup();
    
    /*
    vector<string> models;
    ofxChatGPT::ErrorCode errorCode;
    tie(models, errorCode) = chatGPT.getModelList();
    for (auto m : models) {
        ofLog() << "Model " << m;
    }
     */
    
    // フォントをロード
    string fontName;
#ifdef TARGET_OS_MAC
    fontName = "HiraKakuStdN-W4";
#elif defined WIN32
    fontName = "Meiryo";
#else
    fontName = OF_TTF_SANS;
#endif
    ofTrueTypeFontSettings settings(fontName, 20);
    settings.addRanges(ofAlphabet::Latin);
    settings.addRanges(ofAlphabet::Japanese);
    settings.addRange(ofUnicode::KatakanaHalfAndFullwidthForms);
    settings.addRange(ofUnicode::range{0x301, 0x303F}); // 日本語の句読点などの記号
    TextArea::font.load(settings);
    
    ime.setFont(fontName, 20);
    ime.enable();
    setWidth(ofGetWidth());
    setHeight(ofGetHeight());
    ime.setPos(20, getHeight() - 200);
}

void MidiChat::onUpdate(){
    if (chat.hasMessage()) {
        string gptResponse;
        ofxChatGPT::ErrorCode errorCode;
        tie(gptResponse, errorCode) = chat.getMessage();
        
        ofJson newGPTMsg;
        newGPTMsg["message"]["role"] = "assistant";
        newGPTMsg["message"]["content"] = gptResponse;

        ofLogVerbose("MidiChat") << "GPT: " << newGPTMsg;

        chatView->addMessage(newGPTMsg);
        
        // If the message has sequence, apply to SequencerView
        auto lastMsg = chatView->getLastMessageObject();
        if (lastMsg && lastMsg->hasMidi) {
            sequencerView->setNextSequence(lastMsg->midiJson);
        }
    }
}

void MidiChat::onDraw(){

    /*
    // Display the conversation on the screen.
    stringstream conversationText;
    // Iterate through the conversation messages and build the display text.
    for (const ofJson &message : chatGPT.getConversation()) {
        conversationText << message["role"] << ": " << message["content"] << "\n";
    }
    // Draw the conversation text on the screen.
    ofDrawBitmapString("conversation:\n" + conversationText.str(), 20, 70);
     */

}

void MidiChat::onKeyPressed(ofKeyEventArgs &key) {
    if (key.key == OF_KEY_RETURN && ofGetKeyPressed(OF_KEY_CONTROL)) {
        // IMEで改行されないようにnullを入れておく
        key.key = '\0';
        
        // Ignore if text is empty.
        if (ime.getString() == "") {
            return;
        }
        
        // 返信待ちなら送信できないタイミング
        if (chat.isWaiting()) return;

        // メッセージを送信してIMEをクリア
        sendMessage();
    }
}

void MidiChat::onLocalMatrixChanged() {
    setWidth(ofGetWidth());
    setHeight(ofGetHeight());
    ime.setPos(20, getHeight() - 200);
}

void MidiChat::sendMessage() {
    // send to chatgpt
    ofxChatGPT::ErrorCode errorCode;
    string message = ime.getString();

    // chat data
    ofJson newUserMsg;
    newUserMsg["message"]["role"] = "user";
    newUserMsg["message"]["content"] = message;
    chatView->addMessage(newUserMsg);
    
    ofLogVerbose("MidiChat") << "User: " << newUserMsg;
    
    // メッセージを送信
    chat.chatWithHistoryAsync(message);
    
    ime.clear();
}
