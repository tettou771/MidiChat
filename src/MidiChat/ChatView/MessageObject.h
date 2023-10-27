#pragma once
#include "ofxComponentUI.h"
#include "Thumbnail.h"
#include "TextArea.h"
using namespace ofxComponent;

class MessageObject : public ListElement {
public:
    MessageObject(ofJson json, ofColor _txtColor = ofColor::white);
    
    void onStart() override;
    void onUpdate() override;
    void onDraw() override;;
    bool isValid() {return valid;}
    bool hasMidi() {return itHasMidi;}
    string& getSequenceStr() { return sequenceStr; }
    string getRole() { return role; }
    void setRole(string _role) { role = _role; }
    string message;
    shared_ptr<TextArea> getTextArea() { return textArea; }
private:
    shared_ptr<TextArea> textArea;
    ofJson raw;
    string sequenceStr;
    bool itHasMidi = false;
    string role;
    bool valid = false;
    
    // コンストラクタで決定して、onStart() の中で使うための変数
    ofColor txtColor;
    
    static bool extractSequence(const std::string& content, std::string& sequenceStr);
};

// チャットではなくエラーなどのメッセージだったらこれを使う
class InfoObject : public ListElement {
public:
    // 色はデフォルト値がある
    InfoObject(string infoMsg, ofColor txtColor = ofColor::white, ofColor bgColor = ofColor(0, 0, 100, 100));
    void onStart() override;
    void onUpdate() override;
    void onDraw() override;

private:
    string infoMsg;
    shared_ptr<TextArea> textArea = nullptr;
    ofColor bgColor, txtColor;
};
