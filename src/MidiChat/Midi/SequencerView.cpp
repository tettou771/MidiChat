#include "SequencerView.h"

SequencerView::SequencerView() {
}

SequencerView::~SequencerView() {
	sendMidiStop();
	midiOut.closePort();
}

void SequencerView::onStart() {
	// make dummy data
	{
		string dummyStr = R"({
  "sequence_length": 4800,
  "notes": [
    [0, [true, 76, 127, 1]],
    [480, [false, 76, 0, 1]],
    [480, [true, 76, 127, 1]],
    [960, [false, 76, 0, 1]],
    [960, [true, 72, 127, 1]],
    [1440, [false, 72, 0, 1]],
    [1440, [true, 76, 127, 1]],
    [1920, [false, 76, 0, 1]],
    [1920, [true, 79, 127, 1]],
    [2400, [false, 79, 0, 1]],
    [2400, [true, 79, 127, 1]],
    [2880, [false, 79, 0, 1]],
    [2880, [true, 76, 127, 1]],
    [3360, [false, 76, 0, 1]],
    [3360, [true, 74, 127, 1]],
    [3840, [false, 74, 0, 1]],
    [3840, [true, 72, 127, 1]],
    [4320, [false, 72, 0, 1]]
  ]
})";
		ofJson dummy = ofJson::parse(dummyStr);
		setSequence(dummy);
	}
	start();
}

void SequencerView::onUpdate() {
}

void SequencerView::onDraw() {
	float x = phase * getWidth();
	ofSetColor(200, 0, 0);
	ofDrawLine(x, 0, x, getHeight());
}

void SequencerView::onLocalMatrixChanged() {
}

void SequencerView::setupOsc(string sendAddr, int sendPort, int receivePort) {
	// setup OSC
	oscSender.setup(sendAddr, sendPort);
	oscReceiver.setup(receivePort);
	oscEnabled = true;
}

void SequencerView::start() {
	fps = 0;
	futureSec = 0.1;
	pastUpdatedTime = ofGetElapsedTimef();
	phase = 0;
	sequenceTime = 0;
	pastSequenceTime = -1; // 再生開始時にゼロ秒のnoteを再生するためにマイナスの値にしておく
	sequenceCount = 0;

	startThread();
}

void SequencerView::threadedFunction() {
	openMidi();

	while (!midiStopFlag) {
		now = ofGetElapsedTimef();
		diff = now - pastUpdatedTime;

		// MIDI task
		midiLoop();

		// OSC task
		if (oscEnabled) oscLoop();

		// fpsを表示するため
		float diff = now - pastUpdatedTime;
		if (diff > 0) {
			fps += (1. / diff - fps) * 0.05;
		}
		pastUpdatedTime = now;
		ofSleepMillis(1);
	}

	closeMidi();
}

float SequencerView::getFps() {
	return fps;
}

void SequencerView::setSequence(ofJson mj) {
	midiJson = mj;

	notesMutex.lock();
	try {
		sequenceLengthMs = midiJson["sequence_length"];
		notes.clear();
		for (ofJson& noteJson : midiJson["notes"]) {
			float timeMs = noteJson[0];
			MidiStatus ms = noteJson[1][0] ? MIDI_NOTE_ON : MIDI_NOTE_OFF;
			auto note = Note(timeMs, ms, noteJson[1][1].get<int>(), noteJson[1][2].get<int>(), noteJson[1][3].get<int>());
			notes.push_back(note);
		}
	}
	catch (exception e) {
		ofLogError("SequenceView") << "JSON parse error.";
	}
	notesMutex.unlock();
}

void SequencerView::openMidi() {
	// setup midi
	// print the available output ports to the console
	midiOut.listOutPorts();

	// connect
	string searchName = "VirtualMIDISynth"; // MIDIデバイス名（部分一致）
	ofLog() << "Search port name " << searchName;
	bool founded = false;
	for (auto& port : midiOut.getOutPortList()) {
		if (ofIsStringInString(port, searchName)) {
			midiOut.openPort(port);
			founded = true;
			ofLog() << "Find MIDI port " << port << " and connected.";
			break;
		}
	}
	if (!founded) {
		ofLogError() << "MIDI port " << searchName << " is not founded.";
	}

	sendMidiStart();
}

void SequencerView::closeMidi() {
	sendMidiStop();
	midiOut.closePort();
}

void SequencerView::sendMidiStart() {
	std::vector<unsigned char> message;
	message.push_back(MIDI_START);
	message.push_back(0x00);
	message.push_back(0x00);
	midiOut.sendMidiBytes(message);

	ofLogNotice("Midi") << "Send Midi Start";
}

void SequencerView::sendMidiStop() {
	std::vector<unsigned char> message;
	message.push_back(MIDI_STOP);
	message.push_back(0x00);
	message.push_back(0x00);
	midiOut.sendMidiBytes(message);

	ofLogNotice("Midi") << "Send Midi Stop";
}

void SequencerView::sendMidiTimeClock() {
	std::vector<unsigned char> message;
	message.push_back(MIDI_TIME_CLOCK);
	message.push_back(0x00);
	message.push_back(0x00);
	midiOut.sendMidiBytes(message);
}

void SequencerView::midiLoop() {
	// note�𑗂�
	sequenceTime += diff;

	notesMutex.lock();
	if (sequenceLengthMs > 0) {
		phase = sequenceTime * 1000 / sequenceLengthMs;
		
		for (auto& note : notes) {
			float notetime = note.timeMs / 1000;
			if (pastSequenceTime < notetime &&
				notetime <= sequenceTime) {
				midiOut.sendMidiBytes(note.midiMessage);
			}
		}

		if (sequenceLengthMs / 1000 < sequenceTime) {
			sequenceTime -= sequenceLengthMs / 1000;
			for (auto& note : notes) {
				float notetime = note.timeMs / 1000;
				if (notetime <= sequenceTime) {
					midiOut.sendMidiBytes(note.midiMessage);
				}
			}
		}
	}
	notesMutex.unlock();
	pastSequenceTime = sequenceTime;
}

void SequencerView::oscLoop() {
	// OSCで何かをコントロールするため（今はまだ使っていない）
	if (oscReceiver.hasWaitingMessages()) {
		ofxOscMessage m;
		oscReceiver.getNextMessage(m);
		ofLogNotice("SequencerView") << "Got OSC " << m.getAddress();

		if (m.getAddress() == "/gpt/resetLoop") {
		}
		else if (m.getAddress() == "/gpt/resetPos") {
		}
	}
}
