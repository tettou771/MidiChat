#include "MessageObject.h"

ofTrueTypeFont TextArea::font;

void TextArea::onStart() {
}

void TextArea::onUpdate() {
	if (!delayText) return;

	// delayTextがtrueのとき、つらつらと一文字ずつ描画するためにmessageに一つずつ追加する

	// 今の経過時間
	float elapsedTime = ofGetElapsedTimef() - startTime;

	// 1秒あたりに表示する文字数
    // 5秒以内で表示できるように文字数に比例するが、最低でも 20 にしておく
    cps = MAX(20, (int)u32message.length() / 5);

	// 今あるべき文字数
	int expectedNum = MIN((int)u32message.length(), cps * elapsedTime);

	for (int i = delayTextIndex; i < expectedNum; ++i) {
		string s = ofxGoogleIME::UTF32toUTF8(u32message[i]);
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

void TextArea::setString(string& str) {
	u32message = ofxGoogleIME::UTF8toUTF32(str);
	message = "";
	for (int i = 0; i < u32message.size(); ++i) {
		string s = ofxGoogleIME::UTF32toUTF8(u32message[i]);
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

void TextArea::setStringDelay(string& str) {
	u32message = ofxGoogleIME::UTF8toUTF32(str);
	startTime = ofGetElapsedTimef();
	delayText = true;
	delayTextIndex = 0;
}
