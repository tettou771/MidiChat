#pragma once
#include "ofxComponentUI.h"
#include "ChatThread.h"
#include "Midi/SequencerView.h"
#include "ChatView/ChatView.h"
#include "ofxGoogleIME.h"

using namespace ofxComponent;

class MidiChat: public ofxComponentBase {
public:
    MidiChat();
    void onSetup() override;
	void onUpdate() override;
	void onDraw() override;
    void onKeyPressed(ofKeyEventArgs &key) override;
    void onLocalMatrixChanged() override;

    shared_ptr<SequencerView> sequencerView;
    shared_ptr<ChatView> chatView;
    
    // メッセージを送信してIMEをクリア
    void sendMessage();
    
    // 応答がないときに再送するためのメソッド
    // すでにassistantから返信があった場合は、それは削除される
    void regenerate();

    ChatThread chat;
    
    // 保存用のファイル
    ofFile logFile;
    void makeLogFile();
    void writeToLogFile(const string& message);

    ofxGoogleIME ime;
    
    shared_ptr<InfoObject> regenerateButton = nullptr;
private:
    static string GPT_Prompt() {
        return R"(あなたは作曲家です。APIと会話しているので、音楽のシーケンスをフォーマットに則って返してください。
シーケンスは以下のフォーマットで定義されています。プログラムでパースするために、コードブロックにそれを書いてください。

フォーマットの説明:
General MIDIで演奏するシーケンスを短い文字列で表現してください。

t:ビートと小節数
b:BPM
[パート]:[音名][オクターブ][長さ]...|...

シーケンスは、a/b beat のn小節の長さだとして、このように表現してください。
t:{a},{b},{n}
BPM
b:<BPM>

パートの内容は以下のとおりです。
[パート] (C=コード, M=メロディ, B=ベース, P=パーカッション)
[音名] (C, C#, D, D# E, F, F#, G, G#, A, A#, B, R)
R は休符を表します。オクターブなどは無視されますが、長さは休符の長さになります。
[オクターブ] (0-9)
[長さ] (w=全音符, h=2分音符, q=4分音符, i=8分音符, s=16分音符)
[和音] 音名,オクターブ、長さ（C4e のような記号）をXとして、 (Xmaj, Xmin, Xmaj7, Xmin7, X7, Xdim, Xaug, Xdim7, Xminmaj7, X6, Xmin6, Xsus2, Xsus4, Xadd9, Xminadd9)
和音記号は音名の後に直接記述します。和音の音名とオクターブをそれぞれ区切るために、音と音の間にスペースを入れます。
[強さ] a-g aが最弱で、g だと最大の強さになる。内部はMidiのvelocityなので、gだと127になります。
[音の区切り] カンマ , で表現します。 (C4q,E4q,G3hmaj)
[小説の区切り] 区切りは | で表現します。４小説のメロディなら | が3つ入ります。

シーケンスの例

```
t:4,4,4
b:100
M:C4qmaj,C5qmin,D4qmaj7,D5qmin7|G4q7,G5qdim,A4qaug,A5qdim7|B4qminmaj7,B5q6,E4qmin6,E5qsus2|F4qsus4,F5qadd9,C4qminadd9
```
4/4拍子で4小説の長さ（16拍）、BPMは100で、メロディー部分に新しい和音表記を使用しています。和音は1小節ごとに変わり、すべてのリストされた和音パターンを網羅しています。

必ず、上記のようにコードブロックの中にシーケンスを書いてください。
説明は1行までにして、コードブロックの外に書いてください。)";
    }
};
