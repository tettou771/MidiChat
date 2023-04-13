#include "Note.h"

Note::Note(float time, MidiStatus midiStatus, unsigned char pitch, unsigned char velocity, unsigned char channel) :
    timeMs(time), midiStatus(midiStatus), pitch(pitch), velocity(velocity), channel(channel) {
    midiMessage.push_back(MIDI_NOTE_ON + (channel - 1));
    midiMessage.push_back(pitch);
    midiMessage.push_back(velocity);
}