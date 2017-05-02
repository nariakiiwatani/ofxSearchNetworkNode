# ofxSearchNetworkNode

Searching and listing up nodes on networks.  
You don't need to check and enter ip addresses anymore.  

- only IPv4 is supported
- works on macOS and iOS
- may work on Linux
- won't fully work on Windows unless you code a bit for `libs/IPAddress.cpp`.

## declare
```
ofxSearchNetworkNode search;
```

## setup
```
// (optional)
// target IP address for finding nodes.
// though in many cases this should be a broadcast address, you can set unicast addresses also.
// you can set multipul target by comma separated string.
// by default, valid broadcast addresses are set.
// for now on Windows, you must set this else no node will be found.
search.setTargetIp("192.168.0.255");

// (optional)
// request peers to keep sending heartbeat message.
// if you didn't hear from a peer during a certain time, the node would be marked as "lost".
// if you don't need to check connection lost, set heartbeat to false. default is true.
search.setRequestHeartbeat(false);

// (optional)
// if you want to make your node private, use secret mode.
// in secret mode, nodes cannot be connected unless they have same secret key.
// setting group names works almost same as secret mode,
// but using secret mode allows you to use common and simple group names.
search.enableSecretMode("secret_key");
// search.disableSecretMode();

// finally, setup your node with port and name
search.setup(8000, "my node");
// with third argument, you can set groups to belong to.
// you can set multipul groups by comma separated string.
// nodes in different group won't be shown each other.
// search.setup(8000, "my node", "class1,class2");

// broadcast request message to search nodes.
// it's enough to send request once (usually at startup).
// if an other node appeared to the network even after your request
// she shall also broadcast request message
// and you can receive and register her at that time.
// if your network often lose packets, it's ok to request multipul times or even continuously.
search.request();
// with group name(s), you can find nodes belonging to certain group(s).
// you can even search groups that your node "don't" belong to.
// search.request("awesomegroup");
```

## get node list

```
const std::map<string, ofxSearchNetworkNode::Node> &nodes = net_.getNodes();
```

## events

- nodeFound : new node found
- nodePropertyChanged : node name or group changed
- nodeDisconnected : disconnected by peer
- nodeLost : heartbeat from peer not received
- nodeReconnected : re-received heartbeat from lost node

### usage
```
ofAddListener(search.nodeFound, this, &ofApp::onFoundNode);
void ofApp::onFoundNode(const std::pair<std::string, ofxSearchNetworkNode::Node> &node) {
	cout << "new node : " << node.first << "," << node.second.name << "(" << node.second.groups << ")" << endl;
}
```

## License
MIT
