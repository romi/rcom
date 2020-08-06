import websocket
import json
import subprocess
import time
import pathlib
import sys
import ssl
import logging
logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

class RegistryTests:
    ws = None;
    # websocket.enableTrace(True)

    connection_port = None
    rcreg = None

    def registry_run(self, connection_port):
        self.connection_port = connection_port
        recregistry_switches = "-P"
        current_path = pathlib.Path(__file__).parent.absolute()
        rcreg_exe = current_path / "rcregistry"
        print("%s %s %s".format(str(rcreg_exe), recregistry_switches, str(connection_port)))
        self.rcreg = subprocess.Popen([str(rcreg_exe), recregistry_switches, str(connection_port)])
        time.sleep(2)
        if self.rcreg is not None:
            return True
        return False

    def registry_terminate(self):
        if self.ws.connected:
            self.ws.close(timeout=3)
        if self.rcreg is not None:
            self.rcreg.terminate()

    def registry_connect(self):
        print("connecting to: " + "ws://127.0.0.1:"+str(self.connection_port))
        self.ws = websocket.WebSocket(sslopt={"cert_reqs": ssl.CERT_NONE})
        self.ws.connect("ws://127.0.0.1:"+str(self.connection_port))
        # self.ws = websocket.create_connection("ws://127.0.0.1:"+str(self.connection_port))
        return self.ws.connected

    def registry_send_test(self):
        return_code = 0
        print("Sending {'request':'list'}")
        self.ws.send("{'request':'list'}")
        print("Sent")
        print("Receiving...")
        result = self.ws.recv()
        self.ws.close()
        print("Received '%s'" % result)
        print("Done\r\n")

        result_dictionary = json.loads(result)
        if result_dictionary["success"]:
            print("success")
        else:
            print("failed")
            return_code = 1
        return return_code
