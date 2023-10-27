#include "MessageObject.h"

MessageObject::MessageObject(ofJson json, ofColor _txtColor) {
	raw = json;
	valid = false;
    itHasMidi = false;
    txtColor = _txtColor;

	if (raw.count("message") > 0 && raw["message"].is_object() &&
		raw["message"].count("role") > 0 && raw["message"]["role"].is_string() &&
		raw["message"].count("content") > 0 && raw["message"]["content"].is_string()) {

		role = raw["message"]["role"].get<string>();
		message = raw["message"]["content"];
		valid = true;
		sequenceStr = "";

		itHasMidi = extractSequence(message, sequenceStr);
    }
    else {
		ofLogError("MessageObject") << "JSON is not valid: " << raw;
	}
}

void MessageObject::onStart() {
	if (hasMidi()) {
		// make thumbnail
		auto thumb = make_shared<Thumbnail>(sequenceStr);
		thumb->setRect(ofRectangle(10, 10, 60, 60));
		addChild(thumb);
	}

	// jsonをもとに、テキストオブジェクトを作る
	textArea = make_shared<TextArea>();
    textArea->color = txtColor;
	float margin = 15;
	textArea->setRect(ofRectangle(100, 70, getWidth() - 100 - margin, 0));

    // つらつら出てくるように表示する
    textArea->setStringDelay(message);

	float h = textArea->getPos().y + textArea->getHeight() + margin;
	h = MAX(h, 50); // 最低50pixの大きさを持つ
	setHeight(h);
	addChild(textArea);

	// 親オブジェクトにリサイズ要求
	auto p = getParentBox();
	if (p) {
		p->updateList();
	}
}

void MessageObject::onUpdate() {
	if (textArea && textArea->delayText) {
		// 文字列がつらつらと出ている最中はリサイズ
		float margin = 15;
		float h = textArea->getPos().y + textArea->getHeight() + margin;
		h = MAX(h, 50); // 最低50pixの大きさを持つ
		setHeight(h);

		// 親オブジェクトにリストの調整要求
		auto p = getParentBox();
		if (p) {
			p->updateList();
		}
	}
}

void MessageObject::onDraw() {
	if (!valid) return;

	// ざぶとん
	ofFill();
	ofSetColor(0, 0, 100, 100);
	ofDrawRectangle(1, 1, getWidth() - 2, getHeight() - 2);

	// サムネイルが左に入るので、右に寄せて描画
	ofSetColor(180);
	TextArea::font.drawString(role, 100, 40);
}

bool MessageObject::extractSequence(const string& content, string& sequenceStr) {
	istringstream contentStream(content);
	string line;
	bool inSequence = false;

	while (getline(contentStream, line)) {
		if (line.find("```") != string::npos) {
			inSequence = !inSequence;
            if (!inSequence) break;
		}
		else if (inSequence) {
			sequenceStr += line + "\n";
		}
	}
    
    // コードブロックがないとき
    // "t:" という文字列で始まるかどうかを確かめて、そこからシーケンスとみなす
    if (sequenceStr.empty()) {
        // 再度読み込み
        contentStream = istringstream(content);
        while (getline(contentStream, line)) {
            if (line.length() > 2 && line[1] == ':') {
                sequenceStr += line + "\n";
            }
        }
    }

	return !sequenceStr.empty();
}

InfoObject::InfoObject(string infoMsg, ofColor txtColor, ofColor bgColor): infoMsg(infoMsg), txtColor(txtColor), bgColor(bgColor) {
}

void InfoObject::onStart() {
    // infoMsgをもとにテキストオブジェクトを作る
    textArea = make_shared<TextArea>();
    textArea->color = txtColor;
    float margin = 15;
    textArea->setRect(ofRectangle(margin, margin + TextArea::font.getSize(), getWidth() - 100 - margin, 0));

    textArea->setString(infoMsg);

    float h = textArea->getPos().y + textArea->getHeight() + margin;
    h = MAX(h, 50); // 最低40pixの大きさを持つ
    setHeight(h);
    addChild(textArea);

    // 親オブジェクトにリサイズ要求
    auto p = getParentBox();
    if (p) {
        p->updateList();
    }
}

void InfoObject::onUpdate() {
    if (textArea && textArea->delayText) {
        // 文字列がつらつらと出ている最中はリサイズ
        float margin = 15;
        float h = textArea->getPos().y + textArea->getHeight() + margin;
        h = MAX(h, 50); // 最低50pixの大きさを持つ
        setHeight(h);

        // 親オブジェクトにリストの調整要求
        auto p = getParentBox();
        if (p) {
            p->updateList();
        }
    }
}

void InfoObject::onDraw() {
    // ざぶとん
    ofFill();
    ofSetColor(0, 0, 100, 100);
    ofDrawRectangle(1, 1, getWidth() - 2, getHeight() - 2);
}
