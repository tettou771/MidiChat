#pragma once
#include "ofxComponent.h"
#include "ofxMidi.h"
#include "ofxOsc.h"
#include "../ChatView/Thumbnail.h"
#include "Note.h"
#include "Onpu.h"

using namespace ofxComponent;

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
	void setNextSequence(string& sequenceStr);
	void setCurrentSequence(string& sequenceStr);

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
	string currentSequenceStr, nextSequenceStr;
	bool hasNextMidiJson = false;
	float bpm, sequenceLengthMs;
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

	int noteToMidiPitch(const std::string& note, int octave) {
		const std::map<std::string, int> noteToMidi = {
			{"C", 0}, {"C#", 1}, {"D", 2}, {"D#", 3}, {"E", 4}, {"F", 5}, {"F#", 6}, {"G", 7}, {"G#", 8}, {"A", 9}, {"A#", 10}, {"B", 11}
		};

		int midiPitch = noteToMidi.at(note) + (octave + 1) * 12;
		return midiPitch;
	}

	int partTypeToMidiChannel(const std::string& partType) {
		const map<char, int> partToChannel = {
			{'M', 1}, {'B', 2}, {'C', 3}, {'P', 10}
		};

		int channel = partToChannel.at(partType[0]);
		return channel;
	}

	int noteLengthToMilliseconds(char noteLength, int bpm) {
		std::map<char, double> noteLengthValues = { {'w', 1.0}, {'h', 0.5}, {'q', 0.25}, {'i', 0.125}, {'s', 0.0625} };
		return static_cast<int>(noteLengthValues[noteLength] * 60000.0 / bpm);
	}

	int intensityToMidiVelocity(char intensity) {
		std::map<char, int> intensityValues = { {'a', 18}, {'b', 36}, {'c', 54}, {'d', 73}, {'e', 91}, {'f', 109}, {'g', 127} };
		return intensityValues[intensity];
	}

	void makeNotes(const std::string& sequenceStr, std::vector<Note>& notes, float& bpm) {
		istringstream iss(sequenceStr);
		string line;
		string partType = "P";
		string prevNote = "C";
		string prevOctave = "4";
		char prevLength = 'q';
		char prevIntensity = 'g';

		// a/b拍子 n小節分
		int a = 4, b = 4, n = 4;

		// bpm default
		bpm = 100;

		sequenceLengthMs = 0;

		while (std::getline(iss, line)) {
			if (line.empty()) {
				continue;
			}

			// 拍子の情報
			if (line.substr(0, 2) == "t:") {
				size_t first_comma_pos = line.find(',', 2);
				size_t second_comma_pos = line.find(',', first_comma_pos + 1);

				a = std::stoi(line.substr(2, first_comma_pos - 2));
				b = std::stoi(line.substr(first_comma_pos + 1, second_comma_pos - first_comma_pos - 1));
				n = std::stoi(line.substr(second_comma_pos + 1));

				continue;
			}

			// BPMの情報
			if (line.substr(0, 2) == "b:") {
				bpm = std::stof(line.substr(2));
				continue;
			}

			float currentTimeMs = 0.0;
			partType = line.substr(0, 2);
			line = line.substr(2);

			size_t i = 0;
			while (i < line.length()) {
				string note = prevNote;
				string octave = prevOctave;
				char length = prevLength;
				char intensity = prevIntensity;
				bool harmony = false;
				bool valid = false;

				// Extract note
				if (isupper(line[i])) {
					note = line[i];
					i++;
					valid = true;
					if (i < line.length() && line[i] == '#') {
						note += line[i];
						i++;
					}
				}

				// Extract octave if present
				if (i < line.length() && isdigit(line[i])) {
					octave = line[i];
					i++;
					valid = true;
				}

				// Extract length if present
				if (i < line.length() && strchr("whqis", line[i]) != nullptr) {
					length = line[i];
					i++;
					valid = true;
				}

				// Extract intensity if present
				if (i < line.length() && strchr("abcdefg", line[i]) != nullptr) {
					intensity = line[i];
					i++;
					valid = true;
				}

				// Move to the next note in the chord if there is a '+' symbol
				if (i < line.length() && line[i] == '+') {
					i++;
					valid = true;
					harmony = true;
				}

				if (!valid) {
					i++;
					continue;
				}

				// Create the note and add it to the notes vector
				if (!note.empty() && note[0] != 'R') {
					float noteLengthMs = noteLengthToMilliseconds(length, bpm);
					int pitch = noteToMidiPitch(note, std::stoi(octave));
					int channel = partTypeToMidiChannel(partType);
					int velocity = intensityToMidiVelocity(intensity);

					notes.push_back(Note(currentTimeMs, MIDI_NOTE_ON, pitch, velocity, channel));
					notes.push_back(Note(currentTimeMs + noteLengthMs, MIDI_NOTE_OFF, pitch, velocity, channel));
				}

				if (!harmony) {
					int l = noteLengthToMilliseconds(length, bpm);
					currentTimeMs += l;
				}

				prevNote = note;
				prevOctave = octave;
				prevLength = length;
				prevIntensity = intensity;
			}
		}

		// a / b beat n 小節分の長さがシーケンス全体の長さになる
		sequenceLengthMs = (60000. * n * (a / b)) / bpm;
	}

};

