import time
import websocket
import json

class Client:
    def __init__(self, address):
        try:
            self.ws = websocket.WebSocket()
            self.ws.connect("ws://%s" % address)
        except websocket.WebSocketException:
            print("HEADERS: " + str(self.ws.getheaders()))
            raise Exception("Failed to connect")

    def execute(self, method, params = None):
        request = { 'method': method, 'params': params }
        self.ws.send(json.dumps(request))
        reply = self.ws.recv()
        return json.loads(reply)
    
