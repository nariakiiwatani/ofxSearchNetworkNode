# ofxSearchNetworkNode

ネットワーク上のノードを探してリストアップします。  
もうデバイスのIPアドレスを調べて入力する必要はありません。

- IPv4のみ対応
- macOSとiOSで動作確認済み
- Linuxでも動くはず（動作未確認）
- Windowsでは一部機能が動きません。`libs/IPAddress.cpp`のみ適切に書き換えれば動くはずです。。

## declare
```
ofxSearchNetworkNode search;
```

## setup
```
// (optional)
// ノードの検索に使うIPアドレスを指定します。
// 通常はブロードキャストアドレスを指定しますが、ユニキャストアドレスも指定可能です。（カンマ区切りで複数指定可）
// 指定しない場合、自動でブロードキャストアドレスを検出します。
// いまのところWindows環境では自動検出が働かないので、この値は必ず設定してください。
search.setTargetIp("192.168.0.255");

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
search.setup(8000, "my node");
// 3番目の引数でグループを設定できます。カンマ区切りで複数指定することもできます。
// 所属するグループが違うノード同士は互いに検索されません。
// search.setup(8000, "my node", "class1,class2");

// requestを呼ぶと、ノードを探すためのメッセージがブロードキャストされます。
// 多くの場合、requestはプログラム起動時に一度呼べば十分です。
// 他のノードが後から現れても、そのノードがあなたにリクエストを送ってくれるはずですから、
// そこであなたもそのノードを見つけることができます。
// もしネットワークが不安定である場合は、requestを複数回呼んでも構いませんし、定期的に呼ぶようにしても問題ありません。
search.request();
// requestにグループ名を指定すると、特定のグループに含まれるノードのみを検索することができます。
// 自分が所属していないグループのノードも検索可能です。
// search.request("awesomegroup");
```

## get node list

```
const std::map<string, ofxSearchNetworkNode::Node> &nodes = net_.getNodes();
```

## events

- nodeFound : 新規ノードが発見された
- nodePropertyChanged : ノードのプロパティ（名前やグループ名）が変更された
- nodeDisconnected : 接続相手から切断された
- nodeLost : 死活監視信号が途切れた
- nodeReconnected : 死活監視信号が復帰した

### usage
```
ofAddListener(search.nodeFound, this, &ofApp::onFoundNode);
void ofApp::onFoundNode(const std::pair<std::string, ofxSearchNetworkNode::Node> &node) {
	cout << "new node : " << node.first << "," << node.second.name << "(" << node.second.groups << ")" << endl;
}
```

## License
MIT
