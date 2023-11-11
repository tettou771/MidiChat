#include "MidiChat.h"

MidiChat::MidiChat() {
    
}

void MidiChat::onSetup(){
    setRect(ofRectangle(0, 0, ofGetWidth(), ofGetHeight()));
    
    //ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetLogLevel(OF_LOG_NOTICE);
    
    // show fit view
    sequencerView = make_shared<SequencerView>();
    auto scrollView2 = make_shared<FitView>();
    scrollView2->setRect(ofRectangle(0, 0, getWidth(), getHeight()));
    scrollView2->setContents(sequencerView);
    addChild(scrollView2);

    // show in scroll view
    chatView = make_shared<ChatView>();
    chatView->setAlign(ListBox::Align::FitWidth); // 要素を幅で合わせる
    auto scrollView = make_shared<ScrollView>(ScrollView::FitWidth);
    int margin = 30;
    int topOffset = 120;
    scrollView->setRect(ofRectangle(margin, topOffset, getWidth() - margin*2, getHeight() - margin - topOffset));
    scrollView->setContents(chatView);
    addChild(scrollView);
    
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
    string targetModel = model;
    // もしconfigに文字があり、実際に該当モデルがあればそれを使う
    try {
        string configGptModel = configJson["gpt-model"];
        if (configGptModel == "") {
            ofLogNotice("MidiChat") << "gpt-model in config.json is empty.";
        }
        else {
            targetModel = configGptModel;
        }
    }
    catch (exception e) {
        ofLogNotice("MidiChat") << "No gpt-model in config.json.";
    }
    ofLogNotice("MidiChat") << "GPT Model Target: " << targetModel;
    
    for (auto m : models) {
        // GPT系のモデルだけをリストアップ
            if (ofIsStringInString(m, "gpt")) {
                ofLogNotice("MidiChat") << m;
                if (m == targetModel) model = targetModel;
        }
    }
    
    // GPTのsystemメッセージを読む
    {
        ofFile file("system_prompt.txt");
        if(file.exists()){
            ofBuffer buffer = file.readToBuffer();
            gptSystemPrompt = buffer.getText();
        }
    }

    // セットする
    chat.setup(model, apiKey);
    chat.setSystemMessage(gptSystemPrompt);

    // タイムアウト秒数
    try {
        float timeoutSec = configJson["gpt-timeout"];
        chat.getChatGPT().setTimeoutSec(timeoutSec);
    }
    catch(exception e) {
        
    }
    
    // OSCを使うかどうか確認
    if (configJson.find("osc") != configJson.end() && configJson["osc"].is_object()) {
        try {
            auto receiverJson = configJson["osc"]["receiver"];
            {
                sequencerView->setupOscReceiver(receiverJson["port"]);
            }
            
            auto remoteJson = configJson["osc"]["remote"];
            if (remoteJson["enabled"]) {
                sequencerView->setupOscSender(remoteJson["address"], remoteJson["port"]);
            }
            
            auto broadcastJson = configJson["osc"]["broadcast"];
            {
                string addr = broadcastJson["address"];
                int port = broadcastJson["port"];
                sequencerView->setupBroadcast(addr, port);
            }
        } catch(exception e) {
            ofLogError("MidiChat") << "osc JSON parse error";
        }
    }
    
    // GPTのモデルを情報に入れる
    auto info = make_shared<InfoObject>("GPT model: " + model, ofColor(180));
    chatView->addElement(info);
    
    // status icon
    {
        statusIcon = make_shared<StatusIcon>();
        float size = 80;
        ofRectangle r((getWidth() - size)/2, (topOffset - size) / 2, size, size);
        statusIcon->setRect(r);
        statusIcon->setWhisper(&whisper);
        ofAddListener(statusIcon->mousePressedOverComponentEvents, this, &MidiChat::onStatusIconClicked);
        addChild(statusIcon);
    }
    
    // audio level monitor
    {
        audioLevelMonitor = make_shared<AudioLevelMonitor>(&whisper);
        float h = 30;
        ofRectangle r(getWidth()/2 + 90, (topOffset - h)/2, 500, h);
        audioLevelMonitor->setRect(r);
        addChild(audioLevelMonitor);
    }
    
    // フォントをロード
    string fontName;
#ifdef TARGET_OS_MAC
    fontName = "HiraginoSans-W4";
#elif defined WIN32
    fontName = "Meiryo";
#else
    fontName = OF_TTF_SANS;
#endif
    ofTrueTypeFontSettings settings(fontName, 24);
    settings.addRanges(ofAlphabet::Latin);
    settings.addRanges(ofAlphabet::Japanese);
    settings.addRange(ofUnicode::KatakanaHalfAndFullwidthForms);
    settings.addRange(ofUnicode::range{0x301, 0x303F}); // 日本語の句読点などの記号
    TextArea::font.load(settings);
    
    // setup whisper (with api)
   // RtAudio audioTemp(toRtAudio(api));

    // 指定されているオーディオデバイスがあるかどうか
    bool specifiedAudio = false;
    string findingDeviceName;
    try {
        findingDeviceName = configJson["audio"]["name"];
        if (findingDeviceName != "") {
            specifiedAudio = true;
            ofLogNotice("MidiChat") << "Audio device specified name: " << findingDeviceName;
        }
    }
    catch (exception e) {
        ofLogWarning("MidiChat") << "Audio not specified";
    }

    whisper.printSoundDevices();
    int soundDeviceIndex = -1;
    // 指定されたデバイス名findingDeviceNameに合致するものを探す
    // 指定がない場合はDefaultを探す
    int i = 0;
    for (auto device : whisper.getSoundDevices()) {
        // 指定がある場合、名前を部分一致で探す
        if (specifiedAudio) {
            if (ofIsStringInString(device.name, findingDeviceName)){
                soundDeviceIndex = i;
                ofLogNotice("MidiChat") << "Find specified audio device: " << device.name << " id: " << i;
                break;
            }
        }
        // 指定がない場合、デフォルトかどうか判定
        else if (device.isDefaultInput) {
            soundDeviceIndex = i;
            ofLogNotice("MidiChat") << "Find default audio device: " << device.name << " id: " << device.deviceID;
            break;
        }
        
        i++;
    }
    
    // if device is not founded, use first device
    if (soundDeviceIndex == -1 && whisper.getSoundDevices().size() > 0) {
        soundDeviceIndex = 0;
        ofLogWarning("MidiChat") << "Audio device not found. Select the first device.";
    }
    
    // if default device is founded, set to whisper
    if (soundDeviceIndex != -1) {
        whisper.setup(apiKey);
        whisper.setupRecorder(soundDeviceIndex);
        whisper.setRrSilenceTimeMax(0.7);
        whisper.setLanguage("ja");
        whisper.setPrompt(R"(音楽のシーケンスを作るための会話をしています。Cメジャー、Bマイナーなどは Cmaj Bmin などと表記してください。音楽のジャンルや演奏のテクニック、コード理論の話もします。)");

        setState(RecordingToChatGPT);
    }
    else {
        // オーディオデバイスが見つからないときは、エラー表示
        setState(Error);
    }
}

void MidiChat::onUpdate(){
    if (chat.hasMessage()) {
        string gptResponse;
        ofxChatGPT::ErrorCode errorCode;
        tie(gptResponse, errorCode) = chat.getMessage();
        
        // 成功していたらオブジェクトを追加する
        if (errorCode == ofxChatGPT::Success) {
            ofJson newGPTMsg;
            newGPTMsg["message"]["role"] = "ChatGPT";
            newGPTMsg["message"]["content"] = gptResponse;
            
            ofLogVerbose("MidiChat") << "ChatGPT: " << newGPTMsg;
            writeToLogFile("GPT:");
            writeToLogFile(gptResponse);
            
            ofColor gptTxtColor(20, 190, 150);
            chatView->addMessage(newGPTMsg, gptTxtColor);
            
            // If the message has sequence, apply to SequencerView
            auto lastMsg = chatView->getLastMessageObject();
            if (lastMsg && lastMsg->hasMidi()) {
                sequencerView->setNextSequence(lastMsg->getSequenceStr());
            }
            
            setState(RecordingToChatGPT);
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
        string transcript = whisper.getNextTranscript();
        
        bool ignore = false;
        string ignoreMessages[] = {
            "ご視聴ありがとうございました。",
            "音楽のジャンルや演奏のテクニック、コード理論の話もします。",
            "[はじめしゃちょーエンディング]"
        };
        for (auto & m: ignoreMessages) {
            if (transcript == m) {
                ignore = true;
            }
        }
        if (ignore) {
            ofLogWarning("MidiChat") << "Ignore Transcript " << transcript;
        }else {
            ofLogNotice("MidiChat") << "Transcript " << transcript;
            addToTranscriptingObject(transcript);
        }
    }
    
    // Whisper待ちの時、録音中かWhisperからの返答待ちじゃなくなった時に次の状態に自動遷移する
    // これはユーザーからは見えない遷移
    if (status == WaitingForWhisper) {
        if (!whisper.isRecording() && !whisper.isThreadRunning()) {
            nextState();
        }
    }
}

void MidiChat::onDraw(){

}

void MidiChat::onKeyPressed(ofKeyEventArgs &key) {
    switch (key.key) {
        // フットべダルで押す予定のキー, 1つ目
    case ' ':
        changeButtonPressed();
        break;
        // フットべダルで押す予定のキー, 2つ目
    case 'n':
        sequencerView->nextSequenceReady();
        break;
        // フットべダルで押す予定のキー, 3つ目
    case OF_KEY_RETURN:
        sequencerView->toggleMidi();
        break;
    case 'b':
        sequencerView->toggleGlobalEnabled();
        if (sequencerView->getGlobalBpmEnabled()) {
            sequencerView->sendGlobalBpm();
        }
        break;
    case 'l':
        audioLevelMonitor->setActive(!audioLevelMonitor->getActive());
        break;
    case OF_KEY_ESC:
        // Transcriptionを削除
        if (transcriptingObject) transcriptingObject->destroy();
        break;
    }
}

void MidiChat::onLocalMatrixChanged() {
    setWidth(ofGetWidth());
    setHeight(ofGetHeight());
}

void MidiChat::onDragEvent(ofDragInfo& info) {
    for (auto &path : info.files) {
        auto file = ofFile(path);
        if (file.getExtension() == "txt" && file.exists()) {
            ofLogNotice("MidiChat") << "Replace system with \"" << file.path() << "\"";
            ofBuffer buffer = file.readToBuffer();
            gptSystemPrompt = buffer.getText();
            chat.setSystemMessage(gptSystemPrompt);
            
            // infoにSystemを書き換えたことを書く
            auto info = make_shared<InfoObject>("Replace system with \"" + file.getFileName() + "\"", ofColor(150));
            chatView->addElement(info);
        }
    }
}

void MidiChat::sendMessage(string& message) {
    if (message == "") return;
    
    // send to chatgpt
    ofxChatGPT::ErrorCode errorCode;

    // chat data
    ofJson newUserMsg;
    newUserMsg["message"]["role"] = "User";
    newUserMsg["message"]["content"] = message;
    chatView->addMessage(newUserMsg, ofColor::gray);
    
    // もし自分がMIDIを書いていたら、それを反映させる
    auto lastMsg = chatView->getLastMessageObject();
    if (lastMsg && lastMsg->hasMidi()) {
        sequencerView->setNextSequence(lastMsg->getSequenceStr());
    }
    
    ofLogVerbose("MidiChat") << "User: " << newUserMsg;
    
    // メッセージを送信
    chat.chatWithHistoryAsync(message);
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

void MidiChat::cancel(){
    ofLogNotice("MidiChat") << "Cancel";
    chat.cancel();
    writeToLogFile("Cancel");
}

void MidiChat::startMidi(){
    ofLogNotice("MidiChat") << "Start";
    writeToLogFile("Start");
    
    sequencerView->startMidi();
}

void MidiChat::stopMidi(){
    ofLogNotice("MidiChat") << "Stop";
    writeToLogFile("Stop");
    
    sequencerView->stopMidi();
}

void MidiChat::makeLogFile() {
    // filename
    auto now = time(nullptr);
    auto tm = *localtime(&now);
    ostringstream timestamp;
    timestamp << put_time(&tm, "%Y-%m-%d_%H%M");
    string fileName = "log_" + timestamp.str() + ".txt";
    
    // temp folder
    char tempPath[PATH_MAX];
    size_t tempPathSize = confstr(_CS_DARWIN_USER_TEMP_DIR, tempPath, sizeof(tempPath));
    string tmpFolder = ofToDataPath(string(tempPath) + "MidiChat/log/");
    
    // open logFile
    string filePath = tmpFolder + fileName;
    ofLogNotice("MidiChat") << "Log file path: " << filePath;
    logFile.open(filePath, ofFile::WriteOnly);
}

void MidiChat::writeToLogFile(const string& message) {
    if (!logFile.is_open()) {
        makeLogFile();
    }
    
    logFile << message << endl << endl;
}

void MidiChat::setState(MidiChatStatus next) {
    ofLogNotice("MidiChat") << "Set state " << MidiChatStatusToString(next);
    statusIcon->setStatus(next);
    statusIcon->setBpm(sequencerView->getBpm());

    switch (next) {
    case Stop:
        whisper.stopRealtimeRecording();
        break;
    case Recording:
        whisper.startRealtimeRecording();
        transcriptingObject = nullptr; // こうすることで、文字起こしを新しいMessageObjectに入れられる
        break;
    case RecordingToChatGPT:
        whisper.startRealtimeRecording();
        transcriptingObject = nullptr;
        break;
    case WaitingForWhisper:
        break;
    case WaitingForChatGPT:
        sendTranscriptingObject();
        transcriptingObject = nullptr;
        break;
    default: break;
    }

    status = next;
}

void MidiChat::onStatusIconClicked() {
    changeButtonPressed();
}

void MidiChat::newTranscriptObject(const string& transcript) {
    // まだオブジェクトがない場合（最初の発話と、一度MidiChatに送った後の発話）
    // 新しくオブジェクトを作る。
    ofJson j; // MessageObjectを作るためのテンプレート
    ofColor txtColor = ofColor::white; // 文字の色
    j["message"]["role"] = "User";
    // ChatGPTに送る部分だけは、文字色を黄色にする
    if (status == RecordingToChatGPT) {
        j["message"]["role"] = "User to assistant";
        txtColor = ofColor(255, 220, 0);
    }
    j["message"]["content"] = transcript;
    transcriptingObject = make_shared<MessageObject>(j, txtColor);
    chatView->addMessageObject(transcriptingObject);
}

void MidiChat::addToTranscriptingObject(const string& transcript) {
    if (!transcriptingObject) {
        newTranscriptObject(transcript);
    }
    else {
        auto textArea = transcriptingObject->getTextArea();
        if (textArea) {
            if (textArea->message != "") {
                // 改行をつけて追加
                textArea->addStringDelay("\n" + transcript);
            } else {
                textArea->addStringDelay(transcript);
            }
        } else {
            ofLogError("MidiChat") << "No TextArea";
        }
    }
}

void MidiChat::sendTranscriptingObject() {
    // 文字列がまだ入っていない場合は、何もしない
    if (isTranscriptingObjectEmpty()) {
        ofLogWarning("MidiChat") << "No transcript";
        return;
    }
    
    // テキストエリアを取り出して、の文字列を送信
    auto textArea = transcriptingObject->getTextArea();
    string sendMessage = textArea->getMessage();
    if (textArea) {
        // まっさら, 作り直す, 新しい曲, 全て忘れて などのキーワードが含まれている場合は、
        // ChatGPTの履歴をリセットする
        const string resetWords[] = {"まっさら", "マッサラ", "真っさら", "作り直", "新しい曲", "やり直し", "全て忘れ", "すべて忘れ"};
        bool reset = false;
        for (auto word : resetWords) {
            if (ofIsStringInString(sendMessage, word)) {
                reset = true;
            }
        }

        if (reset) {
            chat.eraseChatGPTHistory();
            // infoにResetしたことを書く
            auto info = make_shared<InfoObject>("Reset", ofColor(150));
            chatView->addElement(info);
        }
        
        // chappyに送る
        chat.chatWithHistoryAsync(sendMessage);

        // ログに残す
        writeToLogFile("User:");
        writeToLogFile(sendMessage);
    } else {
        // オブジェクトが見つからない場合、Recordingステータスに移行
        ofLogError("MidiChat") << "No transcriptingObject textArea";
    }
}

bool MidiChat::isTranscriptingObjectEmpty() {
    if (!transcriptingObject) return true;
    auto ta = transcriptingObject->getTextArea();
    if (!ta) return true;
    return ta->message.empty();
}

void MidiChat::nextState() {
    // 基本的に、RecordingToChatGPTとここをトグルするだけのステート遷移
    // Stop, Recordingは不使用
    
    switch (status) {
        // 停止中なら音声入力開始
        // ChatGPTに送るテキストではない
    case Stop:
        setState(RecordingToChatGPT);
        break;
                
        // 音声入力中ならChatGPT用に
    case Recording:
        setState(RecordingToChatGPT);
        break;
                
    case RecordingToChatGPT:
        // transcriptingObjectがあるときだけ次に進む
        if (!isTranscriptingObjectEmpty()) {
            // 録音中か、whisperの応答待ちの時は、一旦返答がくるのを待つstatusに移行
            if (whisper.isRecording() || whisper.isThreadRunning()) {
                setState(WaitingForWhisper);
            }
            // 音声系のタスクがなければ直接ChatGPTに送信
            else {
                setState(WaitingForChatGPT);
            }
        }
        break;
        
    case WaitingForWhisper:
        setState(WaitingForChatGPT);
        break;

    case WaitingForChatGPT:
        setState(RecordingToChatGPT);
        break;
        
    default: break;
    }
}

void MidiChat::changeButtonPressed() {
    // RecordingToChatGPTのときだけ、次に進める
    if (status == RecordingToChatGPT) {
        nextState();
    }
}

