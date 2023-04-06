#include "MidiChat.h"
#include "prompt.h"

MidiChat::MidiChat() {
    
}

void MidiChat::onSetup(){
    ofSetLogLevel(OF_LOG_VERBOSE);
    
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
        
    // setup chatGPT
    string apiKey;
    ofJson configJson = ofLoadJson("config.json");
    apiKey = configJson["apiKey"].get<string>();
    chatGPT.setup(apiKey);
    
    // send system message
    chatGPT.setSystem(GPT_Prompt());
    chatGPT.setModel("gpt-3.5-turbo");
    //chatGPT.setModel("gpt-4");
    
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

void MidiChat::onLocalMatrixChanged() {
    setWidth(ofGetWidth());
    setHeight(ofGetHeight());
    ime.setPos(20, getHeight() - 200);
}

void MidiChat::onKeyPressed(ofKeyEventArgs &key) {
    if (key.key == OF_KEY_RETURN && ofGetKeyPressed(OF_KEY_CONTROL)) {
        // Ignore if text is empty.
        if (ime.getString() == "") {
            return;
        }
        
        // send to chatgpt
        ofxChatGPT::ErrorCode errorCode;
        string message = ime.getString();

        // chat data
        ofJson newUserMsg;
        newUserMsg["message"]["role"] = "user";
        newUserMsg["message"]["content"] = message;
        chatView->addMessage(newUserMsg);
        
        ofLogVerbose("MidiChat") << "User: " << newUserMsg;

        string gptResponse;
        tie(gptResponse, errorCode) = chatGPT.chatWithHistory(newUserMsg.dump());
        
        ofJson newGPTMsg;
        newGPTMsg["message"]["role"] = "assistant";
        newGPTMsg["message"]["content"] = gptResponse;
        
        ofLogVerbose("MidiChat") << "GPT: " << newGPTMsg;

        chatView->addMessage(newGPTMsg);
        
        // If the message has image, apply to SequencerView
        auto lastMsg = chatView->getLastMessageObject();
        if (lastMsg && lastMsg->hasMidi) {
            sequencerView->setNextSequence(lastMsg->midiJson);
        }

        ime.clear();
    }
}
