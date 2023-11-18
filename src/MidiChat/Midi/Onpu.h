#pragma once
#include "ofxComponent.h"
#include "Note.h"

using namespace ofxComponent;

// 音符を描画するクラス
class Onpu : public ofxComponentBase {
public:
    Onpu(float time, float length, unsigned char pitch, unsigned char velocity, unsigned char channel);
    void onStart() override;
    void onDraw() override;

    void updateSize();
            
    Note noteOn, noteOff;
    static float sequenceLengthMs;
    
    // 今鳴らしている最中かどうか（描画用のフラグ）
    bool isPlaying = false;
    
    // 鍵盤の数
    static const int pitchNum = 128;
};
