#include "Onpu.h"

void Onpu::onStart() {
    updateSize();
}

void Onpu::onDraw() {
    // 音符の色相はchannelに応じる
    float h = 255 * (137 * begin->channel % 360) / 360;
    // 音符の明度は再生中かどうかによる
    float b = isPlaying ? 140 : 100;
    // 彩度は固定
    float s = 200;
    auto onpuColor = ofColor::fromHsb(h, s, b);

    // 無限に小さくなければ描画する
    if (getWidth() > 0 && isfinite(getWidth()) && getHeight() > 0) {
        ofSetColor(onpuColor);
        float r = MIN(MIN(6, getWidth() / 2), getHeight() / 2);
        ofDrawRectRounded(0, 0, getWidth(), getHeight(), r);
    }

    // 音符の種類
    ofSetColor(200);
    stringstream name;
    name << "P " << (int)begin->pitch;
    name << "\nV " << (int)begin->velocity;
    name << "\nC " << (int)begin->channel;
    ofDrawBitmapString(name.str(), 2, 15);
}

void Onpu::updateSize() {
    auto p = getParent();
    if (!p) return;
    ofRectangle rect;
    rect.x = p->getWidth() * (float)begin->timeMs / (float)sequenceLengthMs;
    rect.width = p->getWidth() * ((float)end->timeMs - (float)begin->timeMs) / (float)sequenceLengthMs;

    // ピッチあたりの高さ
    float pitchHeight = p->getHeight() / pitchNum;

    rect.y = p->getHeight() - (begin->pitch) * pitchHeight;
    rect.height = pitchHeight;

    setRect(rect);
}
