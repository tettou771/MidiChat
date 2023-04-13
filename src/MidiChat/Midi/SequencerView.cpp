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
		string dummyStr = 
R"(t:3,4,2
b:80
M:C5q,D5egE5gdC5CCEDCEECE
B:C3hG2h
C:C5i+E5+G5C5i+D5+E5
P:C1qgD1qg)";
        setCurrentSequence(dummyStr);
	}
    
    ofAddListener(Thumbnail::selectedEvents, this, &SequencerView::setNextSequence);
    
	start();
}

void SequencerView::onUpdate() {
    // シーク
    float x = phase * getWidth();
    seekBar->setPos(x, 0);
    
    // 踏んでいる音符にフラグを立てる
    int sequenceTimeMs = sequenceTime * 1000;
    notesMutex.lock();
    for (auto onpu : onpus) {
        bool playing = onpu->begin->timeMs <= sequenceTimeMs &&
        sequenceTimeMs < onpu->end->timeMs;
        onpu->isPlaying = playing;
    }
    notesMutex.unlock();
}

void SequencerView::onDraw() {
    // このオブジェクトの背景
    ofSetColor(0);
    ofDrawRectangle(0, 0, getWidth(), getHeight());
    
    // 行ごとに色を塗る
    for (int pitch=0; pitch<Onpu::pitchNum; ++pitch) {
        float pitchHeight = getHeight() / Onpu::pitchNum;
        // 黒鍵と白鍵
        switch((pitch + Onpu::pitchOffset + 1) % 12) {
        case 1: case 3: case 6: case 8: case 10:
            ofSetColor(0);
            break;
        default:
            ofSetColor(30);
            break;
        }
        ofDrawRectangle(0, getHeight() - pitch * pitchHeight, getWidth(), -pitchHeight);
    }
    
    // info
    ofSetColor(200);
    stringstream ss;
    ss << "BPM " << ofToString(bpm, 0) << endl;
    ss << "Note " << notes.size() << endl;
    ss << "Onpu " << onpus.size() << endl;
    ofDrawBitmapString(ss.str(), 4, 20);
    
    // 次のシーケンスがあった羅、枠を点滅
    if (hasNextMidiJson && fmod(ofGetElapsedTimef(), 0.8) < 0.4) {
        ofSetColor(200);
        ofNoFill();
        ofDrawRectangle(0, 0, getWidth(), getHeight());
    }
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

void SequencerView::setNextSequence(string& sequenceStr) {
    sequenceMutex.lock();
    nextSequenceStr = sequenceStr;
    hasNextMidiJson = true;
    sequenceMutex.unlock();
}

void SequencerView::setCurrentSequence(string& sequenceStr) {
    currentSequenceStr = sequenceStr;
    
    // set sequence data
    notesMutex.lock();
    notes.clear();

    // notes作成
    makeNotes(currentSequenceStr, notes, bpm);
    
    // すでにある描画用の音符を削除
    for (auto onpu : onpus) {
        onpu->destroy();
    }
    onpus.clear();
    
    // 描画用の音符を作る
    vector<shared_ptr<Onpu> > currentOnpu;
    for (auto &note : notes) {
        if (note.midiStatus == MIDI_NOTE_ON) {
            bool founded = false;
            for (auto onpu : currentOnpu) {
                if (onpu->begin == &note) {
                    founded = true;
                    break;
                }
            }
            if (!founded) {
                currentOnpu.push_back(make_shared<Onpu>());
                currentOnpu.back()->begin = &note;
            }
        }
        else if (note.midiStatus == MIDI_NOTE_OFF) {
            for (int i=0; i < currentOnpu.size(); ++i) {
                auto onpu = currentOnpu[i];
                if (onpu->begin &&
                    onpu->begin->pitch == note.pitch &&
                    onpu->begin->channel == note.channel) {
                    onpu->end = &note;
                    onpu->sequenceLengthMs = sequenceLengthMs;
                    onpus.push_back(onpu);
                    addChild(onpu);
                    currentOnpu.erase(currentOnpu.begin() + i);
                    break;
                }
            }
        }
    }
    
    // シークバーを最後に描画するために最後尾につける
    if (!seekBar) seekBar = make_shared<SeekBar>();
    addChild(seekBar);
    
    updateDrawObjectsPosotion();
    
    // メモリ解放
    for (auto onpu:currentOnpu) {
        onpu->destroy();
    }
    
    notesMutex.unlock();
}

void SequencerView::openMidi() {
	// setup midi
	// print the available output ports to the console
	midiOut.listOutPorts();

	// connect
	string searchName = "VirtualMIDI"; // MIDIデバイス名（部分一致）
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

        // 見つからなくても、何かあればそれに繋ぐ
        if (midiOut.getOutPortList().size() > 0) {
            auto &port = midiOut.getOutPortList()[0];
            midiOut.openPort(port);
            ofLogNotice() << "Try connect to " << port;
        }
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
	// noteを読む
	sequenceTime += diff;

	if (sequenceLengthMs > 0) {
        notesMutex.lock();
        phase = sequenceTime * 1000 / sequenceLengthMs;
		for (auto& note : notes) {
			float notetime = note.timeMs / 1000;
			if (pastSequenceTime < notetime &&
				notetime <= sequenceTime) {
				midiOut.sendMidiBytes(note.midiMessage);
			}
		}
        notesMutex.unlock();

        // シーケンスの終わりと始まりを跨いでいたら、sequenceTimeをループの最初に戻す
        // そして、最初のループの最初のあたりの音を踏んでいたら鳴らす
		if (sequenceLengthMs / 1000 < sequenceTime) {
            sequenceTime -= sequenceLengthMs / 1000;

            // もしあれば、次のシーケンスに移行
            changeToNextSequence();

            notesMutex.lock();
            for (auto& note : notes) {
				float notetime = note.timeMs / 1000;
				if (notetime <= sequenceTime) {
					midiOut.sendMidiBytes(note.midiMessage);
				}
			}
            notesMutex.unlock();
		}
	}
	pastSequenceTime = sequenceTime;
}

void SequencerView::changeToNextSequence() {
    if (!hasNextMidiJson) return;
    sequenceMutex.lock();
    setCurrentSequence(nextSequenceStr);
    hasNextMidiJson = false;
    nextSequenceStr = "";
    sequenceMutex.unlock();
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

void SequencerView::updateDrawObjectsPosotion() {
    for (auto onpu:onpus) {
        onpu->updateSize();
        seekBar->setHeight(getHeight());
    }
}
