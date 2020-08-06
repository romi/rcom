import traceback
import json
import logging
import pathlib
import ssl
import subprocess
import sys
import time
import websocket

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
        try:
            self.ws = websocket.WebSocket(sslopt={"cert_reqs": ssl.CERT_NONE})
            self.ws.connect("ws://127.0.0.1:"+str(self.connection_port))
        except websocket.WebSocketException:
            traceback.print_exc()
            print("HEADERS: " + str(self.ws.getheaders()))
        return self.ws.connected

    @staticmethod
    def registry_check_results(result, success_response, message_response):
        test_results = False
        result_dictionary = json.loads(result)
        if result_dictionary["success"] != success_response:
            print("[----------] '%s'" % result)
            print("[  FAILED  ] success != " + "\"" + success_response + "\"")
        elif result_dictionary["message"] != message_response:
            print("[----------] '%s'" % result)
            print("[  FAILED  ] message != " + "\"" + message_response + "\"")
        else:
            print("[       OK ]")
            test_results = True
        return test_results

    def registry_send_test(self, send_data, success_response, message_response):
        return_code = 0
        print("[ RUN      ] registry_send_test \"" + send_data + "\" \""
              + str(success_response) + "\" \"" + message_response + "\"")
        self.ws.send(send_data)
        result = self.ws.recv()
        # print("RESULT: '%s'" % result)
        if not self.registry_check_results(result, success_response, message_response):
            return_code = 1
        return return_code
