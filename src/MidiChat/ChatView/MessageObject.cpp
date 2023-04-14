#include "MessageObject.h"

MessageObject::MessageObject(ofJson json) {
	raw = json;
	valid = false;
    itHasMidi = false;

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
    textArea->color = ofColor(255);
	float margin = 15;
	textArea->setRect(ofRectangle(100, 70, getWidth() - 100 - margin, 0));

	// もしassistantのメッセージなら、ツラツラと出てくるように描画する
	if (raw["message"]["role"] == "assistant") {
		textArea->setStringDelay(message);
	}
	else {
		textArea->setString(message);
	}

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

bool MessageObject::extractSequence(const std::string& content, std::string& sequenceStr) {
	std::istringstream contentStream(content);
	std::string line;
	bool inSequence = false;

	while (std::getline(contentStream, line)) {
		if (line.find("```") != std::string::npos) {
			inSequence = !inSequence;
		}
		else if (inSequence) {
			sequenceStr += line + "\n";
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
