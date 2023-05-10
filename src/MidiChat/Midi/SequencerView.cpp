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
        string dummyStr = R"(
t:4,4,4
b:130
M:C5i,D5,E5,F5,G5,A5,G5,F5|E5,D5,C5,D5,E5,F5,E5,D5|C5,D5,E5,F5,G5,A5,G5,F5|E5,D5,C5,D5,E5,F5,E5,D5
P:C1i,C#1i,D1i,D#1i,E1i,F1i,F#1i,G1i|G#1i,A1i,A#1i,B1i,C2i,C#2i,D2i,D#2i
)";
        
        //setCurrentSequence(dummyStr);
	}
    
    ofAddListener(Thumbnail::selectedEvents, this, &SequencerView::setNextSequence);
    
    setHeight(1280);
    
	start();
}

void SequencerView::onUpdate() {
    setHeight(1280);
    // シーク
    float x = phase * getWidth();
    if (seekBar) seekBar->setPos(x, 0);
    
    // 踏んでいる音符にフラグを立てる
    int sequenceTimeMs = sequenceTime * 1000;
    onpuMutex.lock();
    for (auto onpu : onpus) {
        bool playing = onpu->begin->timeMs <= sequenceTimeMs &&
        sequenceTimeMs < onpu->end->timeMs;
        onpu->isPlaying = playing;
    }
    onpuMutex.unlock();
}

void SequencerView::onDraw() {
    // このオブジェクトの背景
    ofSetColor(0);
    ofDrawRectangle(0, 0, getWidth(), getHeight());
    
    // 行ごとに色を塗る
    for (int pitch=0; pitch<Onpu::pitchNum; ++pitch) {
        float pitchHeight = getHeight() / Onpu::pitchNum;
        // 黒鍵と白鍵
        switch(pitch % 12) {
        case 0: case 2: case 5: case 7: case 9:
            ofSetColor(0);
            break;
        default:
            ofSetColor(30);
            break;
        }
        ofDrawRectangle(0, getHeight() - pitch * pitchHeight, getWidth(), -pitchHeight);
    }
    
    // 小節ごとに線を引く
    ofSetColor(100);
    for (int i=1; i<numMeasures; ++i) {
        int ix = i * getWidth() / numMeasures;
        ofDrawLine(ix, 0, ix, getHeight());
    }
    
    // info
    ofSetColor(200);
    stringstream ss;
    ss << "BPM " << ofToString(bpm, 0) << endl;
    ss << "Beat " << beatNumerator << "/" << beatDenominator << " " << numMeasures << "beats" << endl;
    
    notesMutex.lock();
    ss << "Note " << notes.size() << endl;
    notesMutex.unlock();
    
    onpuMutex.lock();
    ss << "Onpu " << onpus.size() << endl;
    onpuMutex.unlock();

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
    numMeasures = 4;
    beatNumerator = 4;
    beatDenominator = 4;

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
    if (currentSequenceStr == "") {
        phase = sequenceTime = 0;
        pastSequenceTime = 0;
    }
    
    currentSequenceStr = sequenceStr;
    
    // set sequence data
    notesMutex.lock();
    notes.clear();

    // notes作成
    makeNotes(currentSequenceStr, notes, bpm);
    
    onpuMutex.lock();
    
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
    
    onpuMutex.unlock();
    
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
    float sequenceLength = sequenceLengthMs / 1000;

	if (sequenceLengthMs > 0) {
        notesMutex.lock();
		for (auto& note : notes) {
			float notetime = note.timeMs / 1000;
			if (pastSequenceTime < notetime &&
				notetime <= MIN(sequenceTime, sequenceLength)) {
				midiOut.sendMidiBytes(note.midiMessage);
			}
		}
        notesMutex.unlock();

        // シーケンスの終わりと始まりを跨いでいたら、sequenceTimeをループの最初に戻す
        // そして、ループの最初のあたりの音を踏んでいたら鳴らす
		if (sequenceLength < sequenceTime) {
            sequenceTime -= sequenceLength;

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
    // シーケンスが何もない時
    else {
        // もしあれば、次のシーケンスに移行
        changeToNextSequence();
        pastSequenceTime = sequenceTime = 0;
    }
     
    phase = sequenceTime / sequenceLength;
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
    }
    seekBar->setHeight(getHeight());
}
