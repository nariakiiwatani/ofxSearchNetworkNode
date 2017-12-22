[english version](readme.md)

# ofxSearchNetworkNode

ネットワーク上のノードを探してリストアップします。  
もうデバイスのIPアドレスを調べて入力する必要はありません。

- IPv4のみ対応
- macOSとWindowsとiOSで動作確認済み
- Linuxでも動くはず（動作未確認）
- 他の環境では一部機能が動きません。`libs/NetworkUtils.cpp`のみ適切に書き換えれば動くはずです。。

## Setup
```
// target IP addresses for finding nodes.
// by default, valid broadcast addresses are set automatically.
// you can set not only broadcast addresses but also unicast addresses manually.
// addresses should be separeted by commas.

// (optional)
// ノードの検索に使うIPアドレスを指定します。
// 指定しない場合、自動でブロードキャストアドレスを検出します。
// ブロードキャストアドレスでもユニキャストアドレスも指定可能です。
// カンマ区切りで複数指定も可です。
search.setTargetIp("192.168.0.255,10.0.0.1");

// (optional)
// 接続相手に死活確認信号を要求します。
// 一定時間信号が途絶えたら、"lost"したノードとしてマークされます。
// 死活監視の必要がない場合、falseをセットしてください。
search.setRequestHeartbeat(false);

// (optional)
// シークレットモードを使うと、ノードをprivateにできます。
// シークレットモードにあるノードは、同じシークレットキーを持つノードとしか接続されません。
// 後述するグループ機能でも同じようなことが実現できますが、
// グループ名を簡潔に保つためにも、この目的ではシークレットモードの使用を推奨します。
search.enableSecretMode("secret_key");
// search.disableSecretMode();

// 上述の設定が終わったら、ポート名とノード名を指定してsetupします。
search.setup(8000);
// 2番目の引数でこのノードの名前を設定できます。指定しない場合はマシンのホスト名が使われます。
// 3番目の引数でグループを設定できます。カンマ区切りで複数指定することもできます。
// 所属するグループが違うノード同士は互いに検索されません。
// search.setup(8000, "my node", "class1,class2");


// broadcast a request message to search nodes.
// it's enough to send request once (usually at startup).
// because if an other node appeared on the network even after your request
// she shall also broadcast a request message, so you will receive it then you can find her.
// in case your network often loses packets, there is no problem for requesting multipul times or even continuously.
// requestを呼ぶと、ノードを探すためのメッセージがブロードキャストされます。
// 多くの場合、requestはプログラム起動時に一度呼べば十分です。
// 他のノードが後から現れても、そのノードがあなたにリクエストを送ってくれるはずですから、
// そこであなたもそのノードを見つけることができます。
// もしネットワークが不安定である場合は、requestを複数回呼んでも構いませんし、定期的に呼ぶようにしても問題ありません。
search.request();
// requestの引数にグループ名を指定すると、特定のグループに含まれるノードのみを検索することができます。
// 自分が所属していないグループのノードも検索可能です。
// search.request("awesomegroup");
```

## ノードの取得

`getNodes` を使うといつでも現在存在するノードのリストが受け取れます。
```
const std::map<string, ofxSearchNetworkNode::Node> &nodes = search.getNodes();
```

見つかったり見失ったりしたときに通知が欲しい場合は下記のイベントを使用してください。

- nodeFound : 新規ノードが発見された
- nodePropertyChanged : ノードのプロパティ（名前やグループ名）が変更された
- nodeDisconnected : 接続相手から切断された
- nodeLost : 死活監視信号が途切れた
- nodeReconnected : 死活監視信号が復帰した

イベントの登録や利用の仕方は以下を参考にしてください。

```
ofAddListener(search.nodeFound, this, &ofApp::onFoundNode);
void ofApp::onFoundNode(const std::pair<std::string, ofxSearchNetworkNode::Node> &node) {
	cout << "new node : " << node.first << "," << node.second.name << "(" << node.second.groups << ")" << endl;
}
```

## License
MIT
