from websocket import create_connection
import json
import subprocess
import time
import sys
import pathlib

current_path = pathlib.Path(__file__).parent.absolute()
print(current_path)

rcreg = subprocess.Popen(["current_path" + "/rcregistry", "-P", "10109"])

# rcreg = subprocess.Popen(["/home/dboari/Development/romi-rover-build-and-test/cmake-build-debug/bin/rcregistry", "-P", "10109"])
time.sleep(1)

ws = create_connection("ws://127.0.0.1:10109")
print("Sending {'request':'list'}")
ws.send("{'request':'list'}")
print("Sent")
print("Receiving...")
result = ws.recv()
ws.close()
print("Received '%s'" % result)
print("Done\r\n\r\n")

result_dictionary = json.loads(result)
rcreg.terminate()

if result_dictionary["success"]:
    print("success")
    sys.exit(0)
else:
    print("failed")
    sys.exit(1)
