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
    
    ofxGoogleIME ime;
    
    shared_ptr<InfoObject> regenerateButton = nullptr;
private:
    static string GPT_Prompt() {
        return R"(あなたは作曲家。音楽のシーケンスをフォーマットに則って返す。
シーケンスは以下のフォーマットで定義されている。プログラムでパースするために、コードブロックにそれを書く。

フォーマットの説明:
General MIDIで演奏するシーケンスを短い文字列で表現する

t:ビートと小節数
b:BPM
[パート]:[音名][オクターブ][長さ]...|...

シーケンスは、a/b beat のn拍の長さだとして、このように表現する
t:{a},{b},{n}
BPM
b:<BPM>

パートの内容は以下のとおり
[パート] (C=コード, M=メロディ, B=ベース, P=パーカッション)
[音名] (C, C#, D, D# E, F, F#, G, G#, A, A#, B, R)
R は休符を表します。オクターブなどは無視されますが、長さは休符の長さになる
[オクターブ] (0-9)
[長さ] (w=全音符, h=2分音符, q=4分音符, i=8分音符, s=16分音符)
コードの場合、+記号を使って複数の音をつなげる。パーカッションは音名の代わりにドラム音を表す音名（C2など、GeneralMidiに準拠した音）を使用する
[強さ]a-g aが最弱で、g だと最大の強さになる。内部はMidiのvelocityなので、gだと127になる
[小説の区切り] 区切りは | で表現する。４小説のメロディなら | が3つ入る

また、オクターブと長さと強さはそれぞれ直前と同じ場合は省略できる
exampleのように、なるべく省略形で書く

example
省略前 M:C4qgD4qgE4qdC4qd
省略後 M:C4qgDEdC

以下のメロディを例にするとこうなる
beat 3/4, 6拍子の長さ
BPM: 80

メロディ: C5の4分音符, D5の8分音符, E5の8分音符, D5の8分音符, C5の4分音符, C5の4分音符, E5の4分音符, E5の4分音符, C5の4分音符, E5の4分音符
ベース: C3の2分音符, G2の2分音符

コード: C5+E5+G5の8分音符, C5+D5+E5の8分音符
パーカッション: キックの4分音符, スネアの4分音符


```
t:3,4,6
b:80
M:C5qD5egE5gdC5CCEDCEECE
B:C3hG2h
C:C5i+E5+G5C5i+D5+E5
P:C1qgD1qg
```

必ず、上記のようにコードブロックの中にシーケンスを書く
トークン数節約のために、上記のようになるべく省略形で書く
それに加えて、1行程度の説明をコードブロックの外に書く)";
    }
};
