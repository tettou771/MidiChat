#include "MidiChat.h"

MidiChat::MidiChat() {
    
}

void MidiChat::onSetup(){
    setRect(ofRectangle(0, 0, ofGetWidth(), ofGetHeight()));
    
    //ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetLogLevel(OF_LOG_NOTICE);
    
    // show in scroll view
    chatView = make_shared<ChatView>();
    chatView->setAlign(ListBox::Align::FitWidth); // 要素を幅で合わせる
    auto scrollView = make_shared<ScrollView>(ScrollView::FitWidth);
    scrollView->setRect(ofRectangle(60, 60, 900, 800));
    scrollView->setContents(chatView);
    addChild(scrollView);
    
    sequencerView = make_shared<SequencerView>();
    auto scrollView2 = make_shared<ScrollView>(ScrollView::FitWidth);
    scrollView2->setRect(ofRectangle(1000, 60, 860, 800));
    scrollView2->setContents(sequencerView);
    addChild(scrollView2);

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
    string model = "gpt-3.5-turbo"; // default model
    for (auto m : models) {
        // GPT系のモデルだけをリストアップ
        // もしgpt-4があればそれを使う
        if (ofIsStringInString(m, "gpt")) {
            ofLogNotice("MidiChat") << m;
            if (m == "gpt-4") model = "gpt-4";
        }
    }

    // セットする
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
    ofTrueTypeFontSettings settings(fontName, 32);
    settings.addRanges(ofAlphabet::Latin);
    settings.addRanges(ofAlphabet::Japanese);
    settings.addRange(ofUnicode::KatakanaHalfAndFullwidthForms);
    settings.addRange(ofUnicode::range{0x301, 0x303F}); // 日本語の句読点などの記号
    TextArea::font.load(settings);
    
    whisper.setup(apiKey);
    int soundDeviceID = 1;
    whisper.setupRecorder(soundDeviceID);
    whisper.setLanguage("ja");
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
            
            // エラー内容がBadRequestだったら、文字数オーバーの可能性があるのでメッセージ履歴を一部削除する
            if (errorCode == ofxChatGPT::ErrorCode::BadRequest) {
                ofLogNotice("MidiChat") << "Delete oldest 2 messages";
                // 履歴をちょっと消す
                chat.getChatGPT().eraseConversation(1, 3);

                // regenerate
                ofLogNotice("MidiChat") << "Regenerate";
                chatView->deleteLastAssistantMessage();
                writeToLogFile("Regenerate");
                chat.regenerateAsync();

                string message = "Regenerating...";
                auto info = make_shared<InfoObject>(message, ofColor(255, 255, 0));
                chatView->addElement(info);
            }
            
            // Timeoutなら、自動的にリトライする
            else if (errorCode == ofxChatGPT::ErrorCode::Timeout) {
                // regenerate
                ofLogNotice("MidiChat") << "Regenerate";
                chatView->deleteLastAssistantMessage();
                writeToLogFile("Regenerate");
                chat.regenerateAsync();

                string message = "Regenerating...";
                auto info = make_shared<InfoObject>(message, ofColor(255, 255, 0));
                chatView->addElement(info);
            }
            
            // その他のエラーなら、Regenerateボタンを表示
            else {
                writeToLogFile(message);
                
                regenerateButton = make_shared<InfoObject>("Regenerate", ofColor(255, 255, 0));
                chatView->addElement(regenerateButton);
                ofAddListener(regenerateButton->mousePressedOverComponentEvents, this, &MidiChat::regenerate);
            }
        }
    }
    
    // whisper task
    while (whisper.hasTranscript()) {
        string newMessage = whisper.getNextTranscript();
        sendMessage(newMessage);
    }
}

void MidiChat::onDraw(){
    if (whisper.isRecording()) {
        ofVec2f p;
        p.x = getWidth() / 2;
        p.y = getHeight() - 60;
        ofSetColor(sin(ofGetElapsedTimef() * 3) * 50 + 205, 0, 0);
        ofDrawCircle(p, 40);
    }
}

void MidiChat::onKeyPressed(ofKeyEventArgs &key) {
    if (key.key == ' ') {
        if (whisper.isRecording()) {
            whisper.stopRecording();
        }
        else if (!chat.isWaiting()) {
            whisper.startRecording();
        }
    }
}

void MidiChat::onLocalMatrixChanged() {
    setWidth(ofGetWidth());
    setHeight(ofGetHeight());
}

void MidiChat::sendMessage(string& message) {
    if (message == "") return;
    
    // send to chatgpt
    ofxChatGPT::ErrorCode errorCode;

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
}

void MidiChat::regenerate() {
    ofLogNotice("MidiChat") << "Regenerate";
        
    ofRemoveListener(regenerateButton->mousePressedOverComponentEvents, this, &MidiChat::regenerate);
    regenerateButton->destroy();
    regenerateButton = nullptr;

    chatView->deleteLastAssistantMessage();
    
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
    if (!logFile.is_open()) {
        makeLogFile();
    }
    
    logFile << message << endl << endl;
}
