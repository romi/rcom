from websocket import create_connection
import json
import subprocess
import time
import sys
import pathlib

current_path = pathlib.Path(__file__).parent.absolute()
rcreg_exe = current_path / "rcregistry"
print("rcreg_exe '%s'" % rcreg_exe)
rcreg = subprocess.Popen([str(rcreg_exe), "-P", "10108"])

# rcreg = subprocess.Popen(["/home/dboari/Development/romi-rover-build-and-test/cmake-build-debug/bin/rcregistry", "-P", "10109"])
time.sleep(1)

ws = create_connection("ws://127.0.0.1:10108")
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
