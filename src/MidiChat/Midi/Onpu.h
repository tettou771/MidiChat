#pragma once
#include "ofxComponent.h"
#include "Note.h"

using namespace ofxComponent;

// 音符を描画するクラス
class Onpu : public ofxComponentBase {
public:
    void onStart() override;
    void onDraw() override;

    void updateSize();
            
    Note *begin, *end;
    float sequenceLengthMs;
    
    // 今鳴らしている最中かどうか（描画用のフラグ）
    bool isPlaying = false;
    
    // 鍵盤の数
    static const int pitchNum = 128;
};
