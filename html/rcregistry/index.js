
var registry;

function Topic(name)
{
    this.name = name;
    this.list = [];
}

function Registry(uri)
{
    var self = this;

    this.entries = [];
    this.root = document.getElementById("registry");
    this.topics = [];

    this.sendRequest = function(request) {
        if (this.ws && this.ws.readyState == 1)
            this.ws.send(JSON.stringify(request))
    }

    this.requestList = function() {
        this.sendRequest({"request": "list"});
    }

    this.updateView = function() {
        console.log("updateView: " + this.topics.length + " topics");
        var s = "";
        for (var i = 0; i < this.topics.length; i++) {
            var topic = this.topics[i];
            s += "<div class='topic'><h1 class='topic-name'>" + topic.name + "</h1>";
            for (var j = 0; j < topic.list.length; j++) {
                var e = topic.list[j];
                if(e.name == "camera"){
                  s += ("<a target='_blank' href='http://" + e.addr  + "/rgb'> <div class='entry " + e.type + "'>"
                        + "<h2 class='entry-name'>" + e.name + "</h2>"
                        + "<p class='entry-addr'>" + e.addr + "</p>"
                        + "</div></a>");
                }else{
                  s += ("<div class='entry " + e.type + "'>"
                        + "<h2 class='entry-name'>" + e.name + "</h2>"
                        + "<p class='entry-addr'>" + e.addr + "</p>"
                        + "</div>");
                }

            }
            s += "</div>";
        }
        console.log(s);
        this.root.innerHTML = s;
    }

    this.getTopic = function(name) {
        console.log("getTopic " + name);
        for (var i = 0; i < this.topics.length; i++) {
            if (this.topics[i].name == name)
                return this.topics[i];
        }
        topic = new Topic(name);
        this.topics.push(topic);
        return topic;
    }

    this.updateTopics = function(list) {
        console.log("updateTopics: entries " + this.entries.length);
        this.topics = [];
        for (var i = 0; i < this.entries.length; i++) {
            var e = this.entries[i];
            var topic = this.getTopic(e.topic);
            topic.list.push(e);
        }
    }

    this.updateList = function(list) {
        console.log("updateList");
        this.entries = list;
        this.updateTopics();
        this.updateView();
    }

    this.handleMessage = function(e) {
        if (!e.request) {
            console.log("Registry: message has no request string");

        } else if (e.request == "proxy-update-list") {
            this.updateList(e.list);

        } else if (e.request == "proxy-add") {
            this.entries.push(e.entry);
            this.updateTopics();
            this.updateView();

        } else if (e.request == "proxy-remove") {
            var found = false;
            for (var i = 0; i < this.entries.length; i++) {
                var entry = this.entries[i];
                if (entry.id == e.id) {
                    this.entries.splice(i, 1);
                    found = true;
                    break;
                }
            }
            if (!found) console.log("could not find entry with id " + e.id);
            this.updateTopics();
            this.updateView();

        } else {
            console.log("Registry: unknown request: " + e.request);
        }
    }

    if (uri) {
        this.ws = new WebSocket(uri);

        this.ws.onopen = function(e) {
            console.log("Registry: websocket open");
            self.requestList();
        }

        this.ws.onmessage = function(e) {
            console.log(e.data);
            self.handleMessage(JSON.parse(e.data));
        }

        this.ws.onclose = function(e) {
            console.log("Registry: websocket closed");
        }

        this.ws.onerror = function(e) {
            console.log("Registry: websocket error: " + e.toString());
        }
    }
}

function showRegistry(uri)
{
    registry = new Registry(uri);
}
