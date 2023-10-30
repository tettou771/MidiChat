#include "MessageObject.h"

ofTrueTypeFont TextArea::font;
#ifdef WIN32
wstring_convert<codecvt_utf8<uint32_t>, uint32_t> TextArea::convert8_32;
#else
std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> TextArea::convert8_32;
#endif

void TextArea::onStart() {
}

void TextArea::onUpdate() {
	if (!delayText) return;

	// delayTextがtrueのとき、つらつらと一文字ずつ描画するためにmessageに一つずつ追加する

	// 今の経過時間
	float elapsedTime = ofGetElapsedTimef() - startTime;

	// 1秒あたりに表示する文字数
    // 5秒以内で表示できるように文字数に比例するが、最低でも 20 にしておく
    cps = MAX(20, ((int)u32message.length() - offsetTextIndex) / 5);

	// 今あるべき文字数
	int expectedNum = MIN((int)u32message.length(), cps * elapsedTime + offsetTextIndex);

	for (int i = delayTextIndex; i < expectedNum; ++i) {
		string s = UTF32toUTF8(u32message[i]);
		float w = font.stringWidth(message + s);
		if (w > getWidth()) {
			message += '\n' + s;
		}
		else {
			message += s;
		}
	}
	delayTextIndex = expectedNum;

	// 文字列の大きさに合わせてサイズを変更
	auto bbox = font.getStringBoundingBox(message, 0, font.getSize());
	setHeight(bbox.y + bbox.getHeight());

	if (delayTextIndex == u32message.length()) {
		delayText = false;
	}

}

void TextArea::onDraw() {
	// サムネイルが左に入るので、右に寄せて描画
    ofSetColor(color);
	font.drawString(message, 0, font.getSize());
}

void TextArea::setString(const string& str) {
    clear();
    addString(str);
}

void TextArea::setStringDelay(const string& str) {
    clear();
    addStringDelay(str);
}

void TextArea::addString(const string& str) {
    u32message = u32message + UTF8toUTF32(str);
    for (int i = 0; i < u32message.size(); ++i) {
        string s = UTF32toUTF8(u32message[i]);
        float w = font.stringWidth(message + s);
        if (w > getWidth()) {
            message += '\n' + s;
        }
        else {
            message += s;
        }
    }

    // 文字列の大きさに合わせてサイズを変更
    auto bbox = font.getStringBoundingBox(message, 0, font.getSize());
    setHeight(bbox.y + bbox.getHeight());
}

void TextArea::addStringDelay(const string& str) {
    // すでに全部表示されているなら、delayTextIndexを最後の文字まで進めておく
    if (!delayText) delayTextIndex = u32message.length();
        
    u32message += UTF8toUTF32(str);
    startTime = ofGetElapsedTimef();
    offsetTextIndex = delayTextIndex;
    delayText = true;
}

string TextArea::getMessage() {
    // messageはあくまで表示用で、delayさせて表示している時は全文が入っていない場合がある
    // なので、u32messageを使って全文を取り出している。
    // また、こちらには表示用の無駄な改行がないからその点でも良い
    stringstream ss;
    for (auto c : u32message) {
        ss << UTF32toUTF8(c);
    }
    return ss.str();
}

void TextArea::clear() {
    message = "";
    u32message = U"";
    delayTextIndex = 0;
    offsetTextIndex = 0;
}
