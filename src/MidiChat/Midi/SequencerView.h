#pragma once
#include "ofxComponent.h"
#include "ofxMidi.h"
#include "ofxOsc.h"
#include "../ChatView/Thumbnail.h"

using namespace ofxComponent;

// GM音源に対応したnote
class Note {
public:
    Note(float time, MidiStatus midiStatus, unsigned char pitch, unsigned char velocity, unsigned char channel) :
        timeMs(time), midiStatus(midiStatus), pitch(pitch), velocity(velocity), channel(channel) {
        midiMessage.push_back(MIDI_NOTE_ON + (channel - 1));
        midiMessage.push_back(pitch);
        midiMessage.push_back(velocity);
    }
    MidiStatus midiStatus;
    float timeMs;
    unsigned char pitch, velocity, channel;

    // 予め作成しておくメッセージ
    // midiOut.sendMidiBytes(midiMessage) として送信できる
    vector<unsigned char> midiMessage;
};

// 音符を描画するクラス
class Onpu : public ofxComponentBase {
public:
    void onStart() override {
        updateSize();
    }
    
    void updateSize() {
        auto p = getParent();
        if (!p) return;
        ofRectangle rect;
        rect.x = p->getWidth() * (float)begin->timeMs / (float)sequenceLengthMs;
        rect.width = p->getWidth() * ((float)end->timeMs - (float)begin->timeMs) / (float)sequenceLengthMs;
        
        // ピッチあたりの高さ
        float pitchHeight = p->getHeight() / pitchNum;
        
        rect.y = p->getHeight() - (begin->pitch - pitchOffset) * pitchHeight;
        rect.height = pitchHeight;

        int margin = 1;
        rect.x += margin;
        rect.y += margin;
        rect.width -= margin*2;
        rect.height -= margin*2;
        setRect(rect);
    }
    
    void onDraw() override {
        // 音符の色相はchannelに応じる
        float h = 255 * (137 * begin->channel % 360) / 360;
        // 音符の明度は再生中かどうかによる
        float b = isPlaying ? 140 : 100;
        // 彩度は固定
        float s = 200;
        auto onpuColor = ofColor::fromHsb(h, s, b);
        
        ofSetColor(onpuColor);
        float r = MIN(MIN(6, getWidth()/2), getHeight()/2);
        ofDrawRectRounded(0, 0, getWidth(), getHeight(), r);

        // 音符の種類
        ofSetColor(200);
        stringstream name;
        name << "P " << (int)begin->pitch;
        name << "\nV " << (int)begin->velocity;
        name << "\nC " << (int)begin->channel;
        ofDrawBitmapString(name.str(), 2, 15);
    }
        
    Note *begin, *end;
    float sequenceLengthMs;
    
    // 今鳴らしている最中かどうか（描画用のフラグ）
    bool isPlaying = false;
    
    // 鍵盤の表示範囲
    static const int pitchNum = 80;
    static const int pitchOffset = 30;
};

class SeekBar : public ofxComponentBase {
public:
    void onDraw() override {
        ofSetColor(200, 0, 0);
        ofDrawLine(0, 0, 0, getHeight());
    }
};

class SequencerView : public ofxComponentBase, ofThread {
public:
	SequencerView();
    ~SequencerView();

    void onStart() override;
    void onUpdate() override;
    void onDraw() override;
    void onLocalMatrixChanged() override;

    void setupOsc(string sendAddr, int sendPort, int receivePort);
    void start();
    void threadedFunction() override;
    float getFps();

    float futureSec; // どのくらい未来を先取りしてOSC送信するか sec
    float phase;

    // MIDIデータをofJsonで加えるときのメソッド
    void setNextSequence(ofJson& mj);
    void setCurrentSequence(ofJson& mj);

    int getSequenceCount() { return sequenceCount; }
    bool getSentThisFrame() { bool b = sentOsc; sentOsc = false; return b; }
    static const int quantizeNum = 16;
    
private:
    float now; // 今の時刻
    float diff; // 前回からの差分の時刻
    float fps; // threadedFunctionのフレームレート
    float pastUpdatedTime; // 前回threadedFunctionを実行した時刻
    float sequenceTime, pastSequenceTime; // シーケンスの時刻 sec
    int sequenceCount; // シーケンスのループが何周目か

    ofMutex phaseMutex;

    // MIDI関連
    ofxMidiOut midiOut;

    bool midiStopFlag = false;

    void openMidi();
    void closeMidi();
    void sendMidiStart();
    void sendMidiStop();
    void sendMidiTimeClock();
    void midiLoop();

    // MIDIのシーケンスファイル
    ofJson midiJson, nextMidiJson;
    bool hasNextMidiJson = false;
    float sequenceLengthMs;
    vector<Note> notes;
    ofMutex notesMutex; // 排他制御
    
    // MIDIのシーケンスを次に変えるためのメソッド
    // ループの最後にやる
    void changeToNextSequence();
    ofMutex sequenceMutex;
    
private:
    // OSC関連
    bool oscEnabled;
    bool sentOsc;
    ofxOscSender oscSender;
    ofxOscReceiver oscReceiver;
    void oscLoop();
    
    // 描画関連
    shared_ptr<SeekBar> seekBar;
    vector<shared_ptr<Onpu> > onpus;
    void updateDrawObjectsPosotion();
};

