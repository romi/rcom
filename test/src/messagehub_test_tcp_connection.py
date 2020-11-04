import socket
import time

def test_tcp_connection():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    time.sleep(0.1)
    s.close()
    return true
