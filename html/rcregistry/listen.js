var ws = null;
var svgdoc = null;
var svg = null;
var posX = [];
var posY = [];

function drawCircles()
{
    /*circle code*/
    for (let i = 0; i < posX.length; i++) {
        var circles = svgdoc.createElementNS("http://www.w3.org/2000/svg", "circle");
        circles.setAttributeNS(null, 'r', '3%');
        circles.setAttributeNS(null, 'cx', posX[i]);
        circles.setAttributeNS(null, 'cy', posY[i]);
        circles.setAttributeNS(null, 'fill', 'green');
        svg.appendChild(circles);
    };
    /*line code*/
    for (let i = 0; i < posX.length-1; i++) {
      var lines = svgdoc.createElementNS("http://www.w3.org/2000/svg", "line");
      lines.setAttributeNS(null, 'x1', posX[i]);
      lines.setAttributeNS(null, 'y1', posY[i]);
      lines.setAttributeNS(null, 'x2', posX[i]);
      lines.setAttributeNS(null, 'y2', posY[i]);
      lines.setAttributeNS(null, 'style', 'stroke:rgb(22,8,8);stroke-width:1');
      svg.appendChild(lines);
    };
}

function addCircle(x, y)
{
    if (!svgdoc) return;

    x = 400 * x / 3.6;
    y = 500 * y / 3.6;

    if (posX.length > 0) {
        var x1 = posX[posX.length - 1];
        var y1 = posY[posY.length - 1];
        var d = Math.sqrt((x1 - x) * (x1 - x) + (y1 - y) * (y1 - y));
        //console.log([[x,y],[x1,y1],[x1 - x, y1 - y],d]);
        if (d < 20) return;
    }

    posX.push(x);
    posY.push(y);

    var i = posX.length - 1;
    var j = posX.length - 2;

    if (j >= 0) {
        var line = svgdoc.createElementNS("http://www.w3.org/2000/svg", "line");
        line.setAttributeNS(null, 'x1', posX[j]);
        line.setAttributeNS(null, 'y1', posY[j]);
        line.setAttributeNS(null, 'x2', posX[i]);
        line.setAttributeNS(null, 'y2', posY[i]);
        line.setAttributeNS(null, 'style', 'stroke:rgb(22,8,8);stroke-width:2');
        svg.appendChild(line);
     }

    var circle = svgdoc.createElementNS("http://www.w3.org/2000/svg", "circle");
    circle.setAttributeNS(null, 'r', '2%');
    circle.setAttributeNS(null, 'cx', posX[i]);
    circle.setAttributeNS(null, 'cy', posY[i]);
    circle.setAttributeNS(null, 'fill', 'green');
    svg.appendChild(circle);
}

function listenTo(values)
{
    document.head.insertAdjacentHTML('beforeend', "<link rel='stylesheet' href='css/style.css'>");
    document.head.insertAdjacentHTML('afterbegin', "<link rel='stylesheet' href='css/bootstrap.min.css'>");
    document.head.insertAdjacentHTML('afterbegin', "<meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>");
    document.body.insertAdjacentHTML('afterbegin', "<button type='button' class='btn btn-primary btn-block col-4 offset-4 col-lg-2 offset-lg-5'><a href='http://0.0.0.0:10100/registry.html'><b>Back</b></a></button>");
    document.body.insertAdjacentHTML('afterbegin', "<object id='drum' type='image/svg+xml' data='drawing.svg'> your browser does not support SVG </object>");
    document.body.insertAdjacentHTML('beforeend', "<script src='js/bootstrap.min.js'></script>");
    document.body.insertAdjacentHTML('beforeend', "<script src='js/jquery.js'></script>");
    document.body.insertAdjacentHTML('beforeend', "<script src='graph.js'></script>");


    document.getElementById('drum').addEventListener('load', function() {
        svgdoc = document.getElementById('drum').contentDocument;
        svg = svgdoc.getElementsByTagName('svg')[0];
        //console.log(svg);
    }, true);


    ws = new WebSocket(values);

    ws.onopen = function(e) {
        console.log("Registry: websocket open");
    }

    ws.onmessage = function(e) {
        var pos = JSON.parse(e.data);
        console.log(pos);
        addCircle(pos[0], pos[1]);

    }

    ws.onclose = function(e) {
        console.log("Registry: websocket closed");
    }

    ws.onerror = function(e) {
        console.log("Registry: websocket error: " + e.toString());
    }
}
