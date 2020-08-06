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
    ws = None
    connection_port = None
    rcreg = None

    def __init__(self, enable_socket_logging):
        websocket.enableTrace(enable_socket_logging)

    def registry_run(self, connection_port):
        self.connection_port = connection_port
        recregistry_switches = "-P"
        current_path = pathlib.Path(__file__).parent.absolute()
        rcreg_exe = current_path / "rcregistry"
        print("RUN COMMAND: " + str(rcreg_exe) + " " + recregistry_switches + " " + str(connection_port))
        self.rcreg = subprocess.Popen([str(rcreg_exe), recregistry_switches, str(connection_port)])
        time.sleep(1)
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
        return self.ws.connected

    def registry_check_results(self, result, success_response, message_response):
        test_results = False
        result_dictionary = json.loads(result)
        if result_dictionary["success"] != success_response:
            print("RESULT: '%s'" % result)
            print("TEST ERROR: success != " + "\"" + success_response + "\"")
        elif result_dictionary["message"] != message_response:
            print("RESULT: '%s'" % result)
            print("Test ERROR: message != " + "\"" + message_response + "\"")
        else:
            test_results = True
        return test_results

    def registry_send_test(self, send_data, success_response, message_response):
        return_code = 0
        print("TEST: registry_send_test \"" + send_data + "\" \"" + str(success_response) + "\" \"" + message_response + "\"")
        self.ws.send(send_data)
        result = self.ws.recv()
        # print("RESULT: '%s'" % result)
        if self.registry_check_results(result, success_response, message_response):
            print("TEST SUCCESS")
        else:
            print("TEST FAILED")
            return_code = 1
        return return_code



