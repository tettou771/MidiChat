#include "SequencerView.h"

SequencerView::SequencerView() {
    sequenceLengthMs = 0;
    bpm = globalBpm = 0;
}

SequencerView::~SequencerView() {
	sendMidiStop();
	midiOut.closePort();
}

void SequencerView::onStart() {
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

	// make dummy data
	{
        string dummyStr = R"(
t:4,4,16
b:120
M:C4e,G3i,A3s,G3s,F3i,E3q,D3i,E3i,F3s,G3i,A2w|C4i,E4s,G4s,F4s,E4i,D4s,C4i,B3q,A3h|G3q,A3i,B3i,C4i,D4i,E4i,F4s,G4i,A3q,G3|F3i,E3s,D3i,C3s,B2i,A2s,G2i,F2s,E2q,R|
B2h,C3i,D3s,C3s,B2i,A2q,G2i,A2i,B2s,C3i,D3e,E3s,F3i,G3i,A2w|C4i,E4s,G4s,F4s,E4i,D4s,C4i,B3q,A3h|G3q,A3i,B3i,C4i,D4i,E4i,F4s,G4i,A3q,G3|F3i,E3s,D3i,C3s,B2i,A2s,G2i,F2s,E2q,R|
C:Fmaj7q,G7,Cmaj7,E7|Ami7,D7,Gmaj7,C7|Fmaj7,Bb7,Ebmaj7,Ab7|Dmi7,G7,Cmaj7,A7|
Dmi7,G7,Cmaj7,F7|Bb7,Eb7,Abmaj7,D7|Gmi7,C7,Fmaj7,Bb7|Ebmaj7,Ab7,Dmi7,G7|
P:C2f,E2,R,R|C2,E2,R,R|G2,C2,R,R|C2,E2,R,R|G2,C2,R,R|C2,E2,R,R|G2,C2,R,R|C2,E2,R,R
)";
        
        //setCurrentSequence(dummyStr); // debug
	}
    
    ofAddListener(Thumbnail::selectedEvents, this, &SequencerView::setNextSequence);
    
    font.load(OF_TTF_SANS, 30);
        
	startThread();
}

void SequencerView::onUpdate() {
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
    if (globalBpmEnabled) {
        ss << "Global BPM " << ofToString(globalBpm, 0) << " Original " << ofToString(bpm, 0) << endl;
    } else {
        ss << "BPM " << ofToString(bpm, 0) << endl;
    }
    ss << "Beat " << beatNumerator << "/" << beatDenominator << " " << numMeasures << "beats" << endl;
    
    notesMutex.lock();
    ss << "Note " << notes.size() << endl;
    notesMutex.unlock();
    
    onpuMutex.lock();
    ss << "Onpu " << onpus.size() << endl;
    onpuMutex.unlock();
    
    ss << (isPlaying ? "Play" : "Stop") << endl;


    ofDrawBitmapString(ss.str(), 4, 20);
    
    // 次のシーケンスがあったら、枠を点滅させる
    if (hasNextMidiJson) {
        ofPushStyle();
        
        float t = fmod(phase * numMeasures, 1);
        float alpha = ofMap(cos(t * TAU), -0.7, 1, 0, 255, true);
        
        ofColor readyColor(200);
        ofColor awaitingColor(0, 255, 0);

        // Ready 移行する準備ができている場合
        if (!nextSequenceReadyFlag) ofSetColor(readyColor, alpha);
        // Awaiting 移行直前の場合
        else ofSetColor(awaitingColor, alpha);
        
        ofNoFill();
        int lw = 6;
        ofSetLineWidth(lw); // 太めの線でわかりやすくする
        ofDrawRectangle(lw/2, lw/2, getWidth() - lw, getHeight() - lw);
        
        // Readyなどのテキスト
        string statusStr = "";
        if (!nextSequenceReadyFlag) {
            statusStr = "Ready";
            ofSetColor(readyColor);
        }
        else {
            statusStr = "Awaiting";
            ofSetColor(awaitingColor);
        }
        
        auto bbox = font.getStringBoundingBox(statusStr, 0, 0);
        float margin = 20;
        font.drawString(statusStr, getWidth() - bbox.width - margin, font.getSize() + margin);

        ofPopStyle();
    }
}

void SequencerView::onLocalMatrixChanged() {
}

void SequencerView::onDestroy() {
}

void SequencerView::setupOscReceiver(int port) {
    oscReceiver.setup(port);
}

void SequencerView::setupOscSender(const string& addr, const int& port) {
	// setup OSC
	oscSender.setup(addr, port);
	oscEnabled = true;
}

void SequencerView::setupBroadcast(const string& addr, const int& port) {
    // setup OSC
    oscBroadcastSender.setup(addr, port);
}

void SequencerView::threadedFunction() {
	openMidi();

	while (!midiStopFlag) {
		now = ofGetElapsedTimef();
		diff = now - pastUpdatedTime;

		// MIDI task
		midiLoop();

		// OSC task
		oscLoop();

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

void SequencerView::startMidi() {
    isPlaying = true;
    sequenceTime = 0;
}

void SequencerView::stopMidi() {
    isPlaying = false;
    sequenceTime = pastSequenceTime = 0;
    phase = 0;
    
    // 今再生中のノートをオフにする
    onpuMutex.lock();
    for (auto onpu : onpus) {
        if (!onpu->isPlaying) continue;
        sendNote(onpu->end->midiMessage);
        onpu->isPlaying = false;
    }
    onpuMutex.unlock();
}

void SequencerView::toggleMidi() {
    if (isPlaying) stopMidi();
    else startMidi();
}

void SequencerView::sendGlobalBpm() {
    globalBpm = bpm;
    ofxOscMessage m;
    m.setAddress("/bpm");
    m.addFloatArg(globalBpm);
    oscBroadcastSender.sendMessage(m);
}

void SequencerView::setNextSequence(string& sequenceStr) {
    sequenceMutex.lock();
    nextSequenceStr = sequenceStr;
    hasNextMidiJson = true;
    sequenceMutex.unlock();
    nextSequenceReadyFlag = false;
    
    if (!isPlaying) {
        changeToNextSequence();
    }
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
    if (!isPlaying) return;
    
	// noteを読む
    if (globalBpmEnabled) {
        sequenceTime += diff * globalBpm / bpm;
    }else {
        sequenceTime += diff;
    }
    float sequenceLength = sequenceLengthMs / 1000;

	if (sequenceLengthMs > 0) {
        notesMutex.lock();
		for (auto& note : notes) {
			float notetime = note.timeMs / 1000;
			if (pastSequenceTime < notetime &&
				notetime <= MIN(sequenceTime, sequenceLength)) {
                sendNote(note.midiMessage);
			}
		}
        notesMutex.unlock();

        // シーケンスの終わりと始まりを跨いでいたら、sequenceTimeをループの最初に戻す
        // そして、ループの最初のあたりの音を踏んでいたら鳴らす
		if (sequenceLength < sequenceTime) {
            sequenceTime -= sequenceLength;

            // もしあれば、なおかつフラグが立っていれば、
            // 次のシーケンスに移行
            if (nextSequenceReadyFlag) {
                changeToNextSequence();
            }

            notesMutex.lock();
            for (auto& note : notes) {
				float notetime = note.timeMs / 1000;
				if (notetime <= sequenceTime) {
                    sendNote(note.midiMessage);
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

void SequencerView::setGlobalBpm(float _bpm) {
    globalBpm = _bpm;
}

void SequencerView::setGlobalBpmEnabled(bool enabled) {
    globalBpmEnabled = enabled;
}

void SequencerView::toggleGlobalEnabled() {
    setGlobalBpmEnabled(!globalBpmEnabled);
}

void SequencerView::changeToNextSequence() {
    if (!hasNextMidiJson) return;
    sequenceMutex.lock();
    setCurrentSequence(nextSequenceStr);
    hasNextMidiJson = false;
    nextSequenceStr = "";
    sequenceMutex.unlock();
}

void SequencerView::sendNote(vector<unsigned char> midiMessage) {
    midiOut.sendMidiBytes(midiMessage);
    if (oscEnabled) {
        ofxOscMessage m;
        m.setAddress("/midi/note");
        for (auto &mm : midiMessage) {
            m.addCharArg(mm);
        }
        oscSenderMutex.lock();
        oscSender.sendMessage(m);
        oscSenderMutex.unlock();
    }
}

void SequencerView::oscLoop() {
	// OSCで何かを受信する
    while (oscReceiver.hasWaitingMessages()) {
        ofxOscMessage m;
        oscReceiver.getNextMessage(m);
        if (m.getAddress() == "/bpm") {
            setGlobalBpm(m.getArgAsInt(0));
            setGlobalBpmEnabled(true);
        }
    }
}

void SequencerView::updateDrawObjectsPosotion() {
    for (auto onpu:onpus) {
        onpu->updateSize();
    }
    seekBar->setHeight(getHeight());
}
