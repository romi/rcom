from websocket import create_connection
import json
import subprocess
import time
import pathlib

class RegistryTests:
    ws = None
    connection_port = None
    rcreg = None

    def registry_run(self, connection_port):
        self.connection_port = connection_port
        current_path = pathlib.Path(__file__).parent.absolute()
        rcreg_exe = current_path / "rcregistry"
        print("rcreg_exe '%s'" % rcreg_exe)
        self.rcreg = subprocess.Popen([str(rcreg_exe), "-P", str(connection_port)])
        time.sleep(2)
        if self.rcreg is not None:
            return True
        return False

    def registry_terminate(self):
        if self.rcreg is not None:
            self.rcreg.terminate()

    def registry_connect(self):
        print("connecting to: " + "ws://127.0.0.1:"+str(self.connection_port))
        self.ws = create_connection("ws://127.0.0.1:"+str(self.connection_port))
        if self.rcreg is not None:
            return True
        return False

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
