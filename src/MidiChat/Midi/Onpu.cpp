#include "Onpu.h"

float Onpu::sequenceLengthMs;

Onpu::Onpu(float time, float length, unsigned char pitch, unsigned char velocity, unsigned char channel) {
    noteOn = Note(time, MIDI_NOTE_ON, pitch, velocity, channel);
    noteOff = Note(time + length, MIDI_NOTE_OFF, pitch, velocity, channel);
}

void Onpu::onStart() {
    updateSize();
}

void Onpu::onDraw() {
    // 音符の色相はchannelに応じる
    float h = 255 * (137 * noteOn.channel % 360) / 360;
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
    name << "P " << (int)noteOn.pitch;
    name << "\nV " << (int)noteOn.velocity;
    name << "\nC " << (int)noteOn.channel;
    ofDrawBitmapString(name.str(), 2, 15);
}

void Onpu::updateSize() {
    auto p = getParent();
    if (!p) return;
    ofRectangle rect;
    rect.x = p->getWidth() * (float)noteOn.timeMs / (float)sequenceLengthMs;
    rect.width = p->getWidth() * ((float)noteOff.timeMs - (float)noteOn.timeMs) / (float)sequenceLengthMs;

    // ピッチあたりの高さ
    float pitchHeight = p->getHeight() / pitchNum;

    rect.y = p->getHeight() - (noteOn.pitch) * pitchHeight;
    rect.height = pitchHeight;

    setRect(rect);
}
