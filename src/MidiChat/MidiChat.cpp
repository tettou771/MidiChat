#include "MidiChat.h"

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

    // chatGPTのapiKey
    ofJson configJson = ofLoadJson("config.json");
    string apiKey = configJson["apiKey"].get<string>();

    // GPTの何が使えるか調べる
    ofxChatGPT chappy;
    chappy.setup(apiKey);
    vector<string> models;
    ofxChatGPT::ErrorCode err;
    tie(models, err) = chappy.getModelList();
    ofLogNotice("MidiChat") << "Available OpenAI GPT models:";
    for (auto model : models) {
        // GPT系のモデルだけをリストアップ
        if (ofIsStringInString(model, "gpt")) {
            ofLogNotice("MidiChat") << model;
        }
    }

    // セットする
    string model = "gpt-3.5-turbo";
    //string model = "gpt-4";

    chat.setup(model, apiKey);
    chat.setSystemMessage(GPT_Prompt());
    
    // GPTのモデルを情報に入れる
    auto info = make_shared<InfoObject>("GPT model: " + model, ofColor(180));
    chatView->addElement(info);
        
    // フォントをロード
    string fontName;
#ifdef TARGET_OS_MAC
    fontName = "HiraginoSans-W4";
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

    makeLogFile();
}

void MidiChat::onUpdate(){
    if (chat.hasMessage()) {
        string gptResponse;
        ofxChatGPT::ErrorCode errorCode;
        tie(gptResponse, errorCode) = chat.getMessage();
        
        // 成功していたらオブジェクトを追加する
        if (errorCode == ofxChatGPT::Success) {
            ofJson newGPTMsg;
            newGPTMsg["message"]["role"] = "assistant";
            newGPTMsg["message"]["content"] = gptResponse;
            
            ofLogVerbose("MidiChat") << "GPT: " << newGPTMsg;
            writeToLogFile("GPT:");
            writeToLogFile(gptResponse);

            chatView->addMessage(newGPTMsg);
            
            // If the message has sequence, apply to SequencerView
            auto lastMsg = chatView->getLastMessageObject();
            if (lastMsg && lastMsg->hasMidi()) {
                sequencerView->setNextSequence(lastMsg->getSequenceStr());
            }
        }
        // 失敗していたらInfoObjectを追加
        else {
            ofLogError("MidiChat") << "ofxChatGPT has an error. " << ofxChatGPT::getErrorMessage(errorCode);
            string message = "Error: " + ofxChatGPT::getErrorMessage(errorCode);
            auto info = make_shared<InfoObject>(message, ofColor(255, 50, 0));
            chatView->addElement(info);

            writeToLogFile(message);

            regenerateButton = make_shared<InfoObject>("Regenerate", ofColor(255, 255, 0));
            chatView->addElement(regenerateButton);
            ofAddListener(regenerateButton->mousePressedOverComponentEvents, this, &MidiChat::regenerate);
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
    
    // もし自分がMIDIを書いていたら、それを反映させる
    auto lastMsg = chatView->getLastMessageObject();
    if (lastMsg && lastMsg->hasMidi()) {
        sequencerView->setNextSequence(lastMsg->getSequenceStr());
    }
    
    ofLogVerbose("MidiChat") << "User: " << newUserMsg;
    
    // メッセージを送信
    chat.chatWithHistoryAsync(message);

    writeToLogFile("User:");
    writeToLogFile(message);
    
    ime.clear();
}

void MidiChat::regenerate() {
    ofLogNotice("MidiChat") << "Regenerate";
    
    // 一旦、最後のメッセージがassistantだったら削除する
    chatView->deleteLastAssistantMessage();
    
    ofRemoveListener(regenerateButton->mousePressedOverComponentEvents, this, &MidiChat::regenerate);
    regenerateButton->destroy();
    regenerateButton = nullptr;

    writeToLogFile("Regenerate");
    
    chat.regenerateAsync();
}

void MidiChat::makeLogFile() {
    auto now = time(nullptr);
    auto tm = *localtime(&now);
    ostringstream timestamp;
    timestamp << put_time(&tm, "%Y-%m-%d_%H%M");
    string filename = "log/log_" + timestamp.str() + ".txt";

    logFile.open(filename, ofFile::WriteOnly);
}

void MidiChat::writeToLogFile(const string& message) {
    if (logFile.is_open()) {
        logFile << message << endl << endl;
    }
    else {
        ofLogError("writeToLogFile") << "Log file is not open";
    }
}
