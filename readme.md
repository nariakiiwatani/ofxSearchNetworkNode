[日本語版はこちら](readme.ja.md)

# ofxSearchNetworkNode

Searching and listing up nodes on networks.  
You no longer need to check nor type ip addresses.  

- only IPv4 is supported
- works on macOS, Windows and iOS
- may work on Linux
- won't fully work on other platforms unless you code a bit for `libs/NetworkUtils.cpp`.

## Setup
```
ofxSearchNetworkNode search;

// (optional)
// target IP addresses for finding nodes.
// by default, valid broadcast addresses are set automatically.
// you can set not only broadcast addresses but also unicast addresses manually.
// addresses should be separeted by commas.
search.setTargetIp("192.168.0.255,10.0.0.1");

// (optional)
// request peers to keep sending heartbeat messages.
// if you didn't hear from a peer during a certain time, the node would be marked as "lost".
// if you don't need to check connection lost, set heartbeat to false. default is true.
search.setRequestHeartbeat(false);

// (optional)
// if you want to make your node private, use secret mode.
// in secret mode, nodes have different keys cannot be shown.
// though setting group names works almost same as secret mode,
// using secret mode is recommended because you can keep your group names common and simple.
search.enableSecretMode("secret_key");
// search.disableSecretMode();

// setup your node with a port and a name
search.setup(8000);
// with second argument, you can set a name of this node. default is hostname of the machine.
// with third argument, you can set groups to belong to.
// you can set multipul groups by comma separated string.
// nodes in different group won't be shown each other.
// search.setup(8000, "my node", "class1,class2");

// broadcast a request message to search nodes.
// it's enough to send request once (usually at startup).
// because if an other node appeared on the network even after your request
// she shall also broadcast a request message, so you will receive it then you can find her.
// in case your network often loses packets, there is no problem for requesting multipul times or even continuously.
search.request();
// provided group name(s) as an argument, you can find nodes belonging to certain group(s).
// you can even search groups that your node "don't" belong to.
// search.request("awesomegroup");
```

## Getting nodes

You can get a list of existing nodes by using `getNodes`.

```
const std::map<string, ofxSearchNetworkNode::Node> &nodes = search.getNodes();
```

or you can also notice from event objects

- nodeFound : new node found
- nodePropertyChanged : node name or group changed
- nodeDisconnected : disconnected by peer
- nodeLost : heartbeat from peer not received
- nodeReconnected : re-received heartbeat from lost node

```
ofAddListener(search.nodeFound, this, &ofApp::onFoundNode);
void ofApp::onFoundNode(const std::pair<std::string, ofxSearchNetworkNode::Node> &node) {
	cout << "new node : " << node.first << "," << node.second.name << "(" << node.second.groups << ")" << endl;
}
```

## License
MIT
