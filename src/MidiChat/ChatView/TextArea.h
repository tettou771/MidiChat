#pragma once
#include "ofxComponentUI.h"
// text code convert
#include <codecvt>

using namespace ofxComponent;

class TextArea : public ofxComponentBase {
public:
    void onStart() override;
    void onUpdate() override;
    void onDraw() override;
    void setString(const string &str);
    void setStringDelay(const string &str);
    void addString(const string &str);
    void addStringDelay(const string &str);
    void clear();
    
    static ofTrueTypeFont font;
    string message; // 適宜改行したmessage
    u32string u32message;
    float startTime;
        
    // 遅れてテキストがずらずら出るエフェクト
    bool delayText = false;
    int delayTextIndex;
    int offsetTextIndex = 0; // 追加で文字を出す場合に、最初から出ている文字のインデックス
    // 1秒あたりに表示する文字数
    // 文字数が多い時は早く設定する
    float cps; // char / sec
    
    ofColor color;
    
private:
    
#ifdef WIN32
    // 変換器(UTF8 UTF32)
    // char32_t を使うとVS2015でリンクエラーとなるので、unit32_t を使っている
    // ソース Qiita http://qiita.com/benikabocha/items/1fc76b8cea404e9591cf
    static wstring_convert<codecvt_utf8<uint32_t>, uint32_t> convert8_32;
#else
    // 上記でもエラーが出たので、ChatGPTで聞いた変換器
    static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert8_32;
#endif

    static string UTF32toUTF8(const char32_t &u32char) {
        return convert8_32.to_bytes(u32char);
    }

    static u32string UTF8toUTF32(const string & str) {
        auto A = convert8_32.from_bytes(str);
        return u32string(A.cbegin(), A.cend());
    }
};
