#pragma once
#include "ofxComponentUI.h"
#include "ofxChatGPT.h"

using namespace ofxComponent;

// chatの送受信をスレッドにするクラス
class ChatThread : public ofThread {
public:
    ChatThread();
    
    void setup(string model);
    void threadedFunction() override;
    
    // check waiting for GPT response
    bool isWaiting();
    bool hasMessage();
    
    // リクエストを送信
    void chatWithHistoryAsync(string msg);
    
    // 再送信
    // リクエスト中は無視される
    void regenerateAsync();
    
    // メッセージがあれば、それをgetできる
    // getされたものはリストから削除される
    tuple<string, ofxChatGPT::ErrorCode> getMessage();
    
private:
    ofxChatGPT chatGPT;
    string requestingMessage;
    bool regenerateFlag = false;
    ofMutex mutex;
    vector<tuple<string, ofxChatGPT::ErrorCode> > availableMessages;
    static string GPT_Prompt() {
        return R"(Please provide the following JSON data without any additional text or explanation: (説明や追加のテキストなしで、以下のJSONデータを提供してください。)
        
    {
      "message": "ユーザーに宛てたメッセージ",
      "midi": {
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
    }
    ,
      "error": ""
    }

    message: あなたのメッセージ。ここに書かないと、ユーザーは読めません。

    midi: 再生するシーケンスを書いてください。

    このJSONフォーマットは、ループシーケンスを表現するためのシンプルなデータ構造です。General MIDI (GM) 音源と互換性があります。データ構造は以下のようになっています。

    {
      "sequence_length": <シーケンス全体の長さ (ミリ秒)>,
      "notes": [
        [<イベント発生タイミング (ミリ秒)>, [<ノートイベント (ノートオン/オフ)>, <ピッチ>, <ベロシティ>, <チャンネル>]],
        ...
      ]
    }

    midiの各要素の説明
    1. sequence_length: シーケンス全体の長さをミリ秒で表します。
    2. notes: 各音符イベントのデータを格納する配列です。
    3. 各音符イベントは、次のような形式の配列で表されます。
      1. イベント発生タイミング: ノートイベントが発生するタイミングをミリ秒で表します。
      2. ノートイベント: 配列で表現される音符イベントのデータです。以下の4つの要素を含みます。
        1. ノートオン/オフ: ノートオンの場合は true、ノートオフの場合は false を使用します。
        2. ピッチ: 音の高さを表すMIDIノートナンバー (0-127) です。
        3. ベロシティ: 音の強さを表す値です。通常は0-127の範囲で指定されます。ノートオフの場合、通常は0を使用します。
        4. チャンネル: MIDIチャンネルを表す値です。通常は1-16の範囲で指定されます。
    このフォーマットを使用して、GM音源で再生可能なループシーケンスを簡単に表現できます。BPMに応じてイベント発生タイミングを調整することで、さまざまなテンポでシーケンスを再生できます。

    error: なにか問題があったらここに理由を書いてください。

    この会話では、シーケンスのJSONデータをあなたにリクエストするためにユーザーがメッセージを送ってきます。
    主に全音符で4小節分の和音のコード進行を作ってほしいです。
        )";
    }
};
