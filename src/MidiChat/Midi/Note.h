#pragma once
#include "ofxComponent.h"
#include "ofxMidi.h"

using namespace ofxComponent;

// GM音源に対応したnote
class Note {
public:
    Note(float time, MidiStatus midiStatus, unsigned char pitch, unsigned char velocity, unsigned char channel);
    MidiStatus midiStatus;
    float timeMs;
    unsigned char pitch, velocity, channel;

    // 予め作成しておくメッセージ
    // midiOut.sendMidiBytes(midiMessage) として送信できる
    vector<unsigned char> midiMessage;
};
