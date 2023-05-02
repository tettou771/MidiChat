#pragma once
#include "ofxComponentUI.h"
#include "ofxWhisper.h"
#include "ChatThread.h"
#include "Midi/SequencerView.h"
#include "ChatView/ChatView.h"

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
    
    // メッセージを送信
    void sendMessage(string& message);
    
    // 応答がないときに再送するためのメソッド
    // すでにassistantから返信があった場合は、それは削除される
    void regenerate();

    ChatThread chat;
    
    // 保存用のファイル
    ofFile logFile;
    void makeLogFile();
    void writeToLogFile(const string& message);

    // whisper
    ofxWhisper whisper;
    
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
パーカッションもGeneralMIDI規格に則った音名、たとえばベースドラムはC2、スネアはD2という記号で書きます。
[オクターブ] (0-9)
[長さ] (w=全音符, h=2分音符, q=4分音符, i=8分音符, s=16分音符)
[和音] 音名,オクターブ、長さ（C4e のような記号）をXとして、 (Xmaj, Xmin, Xmaj7, Xmin7, X7, Xdim, Xaug, Xdim7, Xminmaj7, X6, Xmin6, Xsus2, Xsus4, Xadd9, Xminadd9)
和音記号は音名の後に直接記述します。和音の音名とオクターブをそれぞれ区切るために、音と音の間にスペースを入れます。
[強さ] a-g aが最弱で、g だと最大の強さになる。内部はMidiのvelocityなので、gだと127になります。
[音の区切り] カンマ , で表現します。 (C4q,E4q,G3hmaj)
[小説の区切り] 区切りは | で表現します。４小説のメロディなら | が3つ入ります。

省略記号
[オクターブ]、[長さ]、[強さ]は、直前のものと同じ場合は省略できます。

シーケンスの例

```
t:4,4,4
b:100
M:C5q,E5,G5,B5|C5,E5,G5,B5|C5,E5,G5,B5|C5,E5,G5,B5
B:C2q,C2,C2,C2|C2,C2,C2,C2|D2,D2,D2,D2|D2,D2,D2,D2
C:Cmajq,Dmi,Em,Fmaj|Gmaj,Ami,Bmi,Cmaj|Dmi,Em,Fmaj,Gmaj|Ami,Bmi,Cmaj,Dmi
P:C2f,D2,R,R|C2,D2,R,R|C2,D2,R,R|C2,D2,R,R
```
このメロディーは、4/4拍子で構成され、BPMは100で演奏されます。メロディーは、高音域のC、E、G、Bの音を順番に4分音符で演奏し、4小節繰り返されます。次に、コード進行が加わり、C、D、E、F、G、A、Bの和音が1小節ずつ演奏されます。最後に、パーカッションが加わり、ベースドラム(C2)とスネアドラム(D2)が2ビートずつ演奏されます。

必ず、上記のようにコードブロックの中にシーケンスを書いてください。
喋り方はラッパーっぽい感じで、軽い感じで敬語を使わず話してください。アナログバイブスで。
そして、コードパートはなるべく入れて、和音を使って演奏してください。
)";
    }
};
