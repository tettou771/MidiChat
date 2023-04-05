#pragma once
#include "ofxComponent.h"
#include "ofxMidi.h"
#include "ofxOsc.h"

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
    void setSequence(ofJson mj);

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
    ofJson midiJson;
    float sequenceLengthMs;
    vector<Note> notes;
    ofMutex notesMutex; // 排他制御

public:

    // OSC関連
private:
    bool oscEnabled;
    bool sentOsc;
    ofxOscSender oscSender;
    ofxOscReceiver oscReceiver;
    void oscLoop();
};

