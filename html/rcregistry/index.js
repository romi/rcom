var registry;
var ws1 = null;

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
    document.head.insertAdjacentHTML('afterbegin', "<meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>");
    document.body.insertAdjacentHTML('afterbegin', "<div class='robotTitle'>NameOfRobot</div>");
    document.body.insertAdjacentHTML('afterbegin', "<script src='listen.js'></script>");

    this.updateView = function() {
        console.log("updateView: " + this.topics.length + " topics");
        var s = "";
        for (var i = 0; i < this.topics.length; i++) {
            var topic = this.topics[i];

            if(topic.name == "camera"){
                s += "<div class='topic'><div><h1 class='topic-name came'>" + topic.name + "</h1>"

            }else{
                s += "<div class='topic'><h1 class='topic-name'>" + topic.name + "</h1>"
            };

            for (var j = 0; j < topic.list.length; j++) {
                var e = topic.list[j];

                  if(e.name == "camera"){
                    s += ("<div class='entry " + e.type + "'>"
                          + "<div class='gauche'><h2 class='entry-name'>" + e.name + "</h2>"
                          + "<p class='entry-addr'>" + e.addr + "</p></div>"
                          +"<div class='droite'><a href='javascript:switchDisplay();'><div id='Cam1'><img class='logoCam' src='img/photo.png'></div></a>"
                          +"<a href='javascript:switchDisplay();'><div id='Cam2' style='display:none'><img class='logoCam' src='img/photo2.png'></div></a>"
                          +"<img id='camera' src='http://" + e.addr + "/rgb' width='80%' height='80%' style='display:none' />"
                          + "</div></div></div>"
                          );


                  }else if(e.topic =="encoders" &&  e.name == "motorcontroller"){
                    s += ("<a href=listen.html?" + e.addr + "> <div id='svg' class='entry " + e.type + "'>"
                            + "<div class='gauche'><h2 class='entry-name'>" + e.name + "</h2>"
                            + "<p class='entry-addr'>" + e.addr + "</p></div>"
                            +"<div class='droite'><a href='javascript:switchDisplay2();'><div id='motor1'><img class='logoMotor' src='img/logoMotor.png'></div></a>"
                            +"<a href='javascript:switchDisplay2();'><div id='motor2' style='display:none'><img class='logoMotor' src='img/logoMotor2.png'></div></a>"
                            + "</div></div></a>");
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

function switchDisplay(){
    var defaut = document.getElementById('Cam1');
    var autre = document.getElementById('Cam2');
    var cam = document.getElementById('camera');
    autre.style.display = (autre.style.display == 'none' ? '' : 'none');
    defaut.style.display = (defaut.style.display == 'none' ? '' : 'none');
    cam.style.display = (cam.style.display == 'none' ? '' : 'none');
}

function switchDisplay2(){
    var defaut = document.getElementById('motor1');
    var autre = document.getElementById('motor2');
    autre.style.display = (autre.style.display == 'none' ? '' : 'none');
    defaut.style.display = (defaut.style.display == 'none' ? '' : 'none');

}



function showRegistry(uri)
{
    registry = new Registry(uri);
}
