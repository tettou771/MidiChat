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
        ofSetLineWidth(2);
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
    void onDestroy() override;

    void setupOscReceiver(int port);
	void setupOscSender(const string& addr, const int& port);
    void setupBroadcast(const string& addr, const int& port);
	void threadedFunction() override;
	float getFps();
    
    void startMidi(); // 送信を再開（シーケンスの最初から）
    void stopMidi(); // 送信を停止
    void toggleMidi(); // start, stop をトグル
    void sendGlobalBpm();
    
    // bpm設定
    void setGlobalBpm(float bpm);
    void setGlobalBpmEnabled(bool enabled);
    void toggleGlobalEnabled();
    bool getGlobalBpmEnabled() {return globalBpmEnabled; }

	float futureSec; // どのくらい未来を先取りしてOSC送信するか sec
	float phase;

	// MIDIデータをofJsonで加えるときのメソッド
	void setNextSequence(string& sequenceStr);
	void setCurrentSequence(string& sequenceStr);
    
    // 次のMIDIに移行していいというフラグを立てるためのメソッド
    void nextSequenceReady() {nextSequenceReadyFlag = true;}

	int getSequenceCount() { return sequenceCount; }
	bool getSentThisFrame() { bool b = sentOsc; sentOsc = false; return b; }
    
    float getBpm() {return bpm;}

private:
	float now; // 今の時刻
	float diff; // 前回からの差分の時刻
	float fps; // threadedFunctionのフレームレート
	float pastUpdatedTime; // 前回threadedFunctionを実行した時刻
	float sequenceTime, pastSequenceTime; // シーケンスの時刻 sec
	int sequenceCount; // シーケンスのループが何周目か

	// MIDI関連
	ofxMidiOut midiOut;

	bool midiStopFlag = false;
    bool isPlaying = false;
    bool nextSequenceReadyFlag = false;

	void openMidi();
	void closeMidi();
	void sendMidiStart();
	void sendMidiStop();
	void sendMidiTimeClock();
	void midiLoop();

    // 合わせる時のBPM
    float globalBpm;
    bool globalBpmEnabled = false;

	// MIDIのシーケンスファイル
	string currentSequenceStr, nextSequenceStr;
	bool hasNextMidiJson = false;
	float bpm, sequenceLengthMs;
    int beatNumerator, beatDenominator, numMeasures;

	// MIDIのシーケンスを次に変えるためのメソッド
	// ループの最後にやる
	void changeToNextSequence();
	ofMutex sequenceMutex;

    // OSCとMIDI両方で送信
    void sendNote(vector<unsigned char> midiMessage);
    
	// OSC関連
	bool oscEnabled = false;
	bool sentOsc;
	ofxOscSender oscSender, oscBroadcastSender;
    ofMutex oscSenderMutex;
	void oscLoop();
    
    ofxOscReceiver oscReceiver;
    
    string broadcastAddress;
    int broadcastPort;

	// 描画関連
	shared_ptr<SeekBar> seekBar;
	vector<shared_ptr<Onpu> > onpus;
	void updateDrawObjectsPosotion();
    ofMutex onpuMutex;
    void noteOffCurrentOnpus();
    
    // Readyなどを表示するフォント
    ofTrueTypeFont font;

    // パーサ
    vector<int> shortenChordNotation(string& noteStr) {
        
        const tuple<string, vector<int>> chordOffsets[] = {
            {"maj7", {0, 4, 7, 11}},
            {"maj", {0, 4, 7}},
            {"m7", {0, 4, 7, 11}},
            {"M", {0, 4, 7}},
            {"minmaj7", {0, 3, 7, 11}},
            {"minadd9", {0, 3, 7, 14}},
            {"min7", {0, 3, 7, 10}},
            {"mi7", {0, 3, 7, 10}},
            {"min6", {0, 3, 7, 9}},
            {"min", {0, 3, 7}},
            {"mi", {0, 3, 7}},
            {"m", {0, 3, 7}},
            {"aug", {0, 4, 8}},
            {"dim7", {0, 3, 6, 9}},
            {"dim", {0, 3, 6}},
            {"sus2", {0, 2, 7}},
            {"sus4", {0, 5, 7}},
            {"add9", {0, 4, 7, 14}},
            {"7", {0, 4, 7, 10}},
            {"6", {0, 4, 7, 9}},
        };

        for (auto offset : chordOffsets) {
            if (ofIsStringInString(noteStr, get<0>(offset))) {
                ofStringReplace(noteStr, get<0>(offset), "");
                return get<1>(offset);
            }
        }

        // 該当なしの場合
        return vector<int>() = {0};
    }
    
	int noteToMidiPitch(const std::string& note, char octave) {
		const std::map<std::string, int> noteToMidi = {
            {"C", 0}, {"C#", 1}, {"Db", 1}, {"D", 2}, {"D#", 3}, {"Eb", 3}, {"E", 4}, {"F", 5}, {"F#", 6}, {"Gb", 6}, {"G", 7}, {"G#", 8}, {"Ab", 8}, {"A", 9}, {"A#", 10}, {"Bb", 10}, {"B", 11}
		};

		int midiPitch = 0; // ノートが見つからない場合のデフォルト値
		auto it = noteToMidi.find(note);
		if (it != noteToMidi.end()) {
			midiPitch = it->second;
		}

		// オクターブによってシフトする
		int octaveInt = octave - '0';
		midiPitch += (octaveInt + 1) * 12;

		return midiPitch;
	}

	int partTypeToMidiChannel(const std::string& partType) {
		const map<char, int> partToChannel = {
			{'M', 1}, {'B', 2}, {'C', 3}, {'P', 10}
		};

        auto it = partToChannel.find(partType[0]);
        if (it != partToChannel.end()) {
            return it->second;
        } else {
            // キーが見つからない場合、デフォルト値を返す
            return 1;
        }
	}

	int noteLengthToMilliseconds(char noteLength, float bpm) {
		std::map<char, double> noteLengthValues = { {'w', 1.0}, {'h', 0.5}, {'q', 0.25}, {'i', 0.125}, {'s', 0.0625} };
		return static_cast<int>(noteLengthValues[noteLength] * beatDenominator * 60000.0 / bpm);
	}

	int intensityToMidiVelocity(char intensity) {
		std::map<char, int> intensityValues = { {'a', 18}, {'b', 36}, {'c', 54}, {'d', 73}, {'e', 91}, {'f', 109}, {'g', 127} };
		return intensityValues[intensity];
	}
    
    vector<std::string> parseLines(std::istringstream& iss) {
        std::vector<std::string> lines;
        std::string line, currentLine;

        while (std::getline(iss, line)) {
            // 英字とコロンで始まる行かどうかをチェック
            if (!line.empty() && isalpha(line[0]) && line.size() > 1 && line[1] == ':') {
                // 新しい行が始まったら、現在の行を保存
                if (!currentLine.empty()) {
                    lines.push_back(currentLine);
                    currentLine.clear();
                }
                currentLine = line; // 新しい行を現在の行としてセット
            } else {
                // 新しい行が始まるまで現在の行に結合
                currentLine += line;
            }
        }

        // 最後の行を追加
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }

        return lines;
    }
    
	void makeNotes(const string& sequenceStr, vector<shared_ptr<Onpu> >& onpus, float& bpm) {
        onpuMutex.lock();
        // すでにある描画用の音符を削除
        for (auto onpu : onpus) {
            onpu->destroy();
        }
        onpus.clear();
        onpuMutex.unlock();
        
        istringstream iss(sequenceStr);
		string line;

		// a/b拍子 n小節
        beatNumerator = 4;
        beatDenominator = 4;
        numMeasures = 4;
        
        // 実際に入っていた小節の数
        // これが numMeasures と食い違っていたら、強制的に numMeasures に代入する
        int realNumMeasuresMax = 0;

		// bpm default
		bpm = 100;

		sequenceLengthMs = 0;

        // 行を配列に入れる
        auto lines = parseLines(iss);
        
		for (auto &line: lines) {
			if (line.length() < 2) {
				continue;
			}

			// 拍子の情報
			if (line.substr(0, 2) == "t:") {
				size_t first_comma_pos = line.find(',', 2);
				size_t second_comma_pos = line.find(',', first_comma_pos + 1);

				beatNumerator = std::stoi(line.substr(2, first_comma_pos - 2));
				beatDenominator = std::stoi(line.substr(first_comma_pos + 1, second_comma_pos - first_comma_pos - 1));
				numMeasures = std::stoi(line.substr(second_comma_pos + 1));

				continue;
			}

			// BPMの情報
			if (line.substr(0, 2) == "b:") {
				bpm = std::stof(line.substr(2));
				continue;
			}
            
            // 以下は、各パートの情報

			float currentTimeMs = 0.0;
            string partType = "M:"; // 名前がなければ M パートとみなす
            // コロンがあれば、それをパートタイプとする
            if (line[1] == ':') {
                partType = line.substr(0, 2);
                line = line.substr(2);
            }

            // デフォルト値
            string note = "";
            char octave = '3';
            char length = 'q';
            char intensity = 'g';
            int measureNum = 0;

            // パートの中の1行をパース
            for (auto measureStr : ofSplitString(line, "|")){
                if (measureStr == "") continue;
                
                measureNum++;
                // 不要な文字が入っていたら削除
                // スペース
                ofStringReplace(measureStr, " ", "");
                // コロン
                ofStringReplace(measureStr, ":", "");
                

                for (auto noteStr : ofSplitString(measureStr, ",")) {
                    if (noteStr == "") continue;
                    
                    int i=0;
                                        
                    // Extract code
                    auto offsets = shortenChordNotation(noteStr);

                    // Extract note
                    note = noteStr[i];
                    ++i;

                    if (noteStr[i] == '#' || noteStr[i] == 'b') {
                        note += noteStr[i];
                        ++i;
                    }

                    // Extract octave if present
                    if (isdigit(noteStr[i])) {
                        octave = noteStr[i];
                        ++i;
                    } else {
                        // default
                        octave = '3';
                    }
                    
                    // ここまでのパースで使った文字列は削除する
                    noteStr = noteStr.substr(i, noteStr.length() - i);
                    i=0;
                    

                    // Extract length if present
                    if (noteStr.length() > i && strchr("whqis", noteStr[i]) != nullptr) {
                        length = noteStr[i];
                        ++i;
                    }

                    // Extract intensity if present
                    else if (noteStr.length() > i && strchr("abcdefg", noteStr[i]) != nullptr) {
                        intensity = noteStr[i];
                        ++i;
                    }
                    
                    // R は休符なのでNoteを作らない
                    if (!(note.length() == 0) && note[0] != 'R') {
                        // 微妙にランダムに遅延させる msec
                        float delay = ofRandom(50);
                        
                        float noteLengthMs = noteLengthToMilliseconds(length, bpm);
                        int pitch = noteToMidiPitch(note, octave);
                        int channel = partTypeToMidiChannel(partType);
                        int velocity = intensityToMidiVelocity(intensity);
                        
                        // コードなら複数作成される
                        for (auto offset : offsets) {
                            // onpuを生成
                            auto newOnpu = make_shared<Onpu>(currentTimeMs + delay, noteLengthMs - delay, pitch + offset, velocity, channel);
                            addChild(newOnpu);

                            onpuMutex.lock();
                            onpus.push_back(newOnpu);
                            onpuMutex.unlock();
                        }
                    }

                    // 音符の長さ分シフト
                    currentTimeMs += noteLengthToMilliseconds(length, bpm);
                    
                    // 1拍分シフト
                    //currentTimeMs += 60000. / bpm;
                    
                    // 1拍または、音符が長い時はその長さ分シフト
                    //currentTimeMs += MAX(noteLengthToMilliseconds(length, bpm), 60000. / bpm);

                } // note end

                currentTimeMs = measureNum * 60000. * beatNumerator / bpm;
            } // measure end
            
            // 区切りの数を数える
            realNumMeasuresMax = MAX(measureNum, realNumMeasuresMax);
		} // part end

        // もし、実際の小節数が与えられ得た小節数の3/2倍以上あったら実際の方に合わせる
        if (numMeasures > realNumMeasuresMax * 3 / 2) {
            numMeasures = realNumMeasuresMax;
        }
        
		// 小節の数がシーケンス全体の長さになる
		sequenceLengthMs = 60000. * numMeasures * beatNumerator / bpm;
        
        // 全ての音符にそれを教える
        Onpu::sequenceLengthMs = sequenceLengthMs;
	}
};

