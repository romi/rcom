import threading
import socket
import time

success = True

def is_valid_response(response):
    return ((response.find("HTTP/1.1 101 Switching Protocols") >= 0)
            and (response.find("Upgrade: websocket") >= 0)
            and (response.find("Connection: Upgrade") >= 0)
            and (response.find("Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") >= 0))
  
def send_handshake():
    global success
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    time.sleep(1)
    request = ('GET / HTTP/1.1\r\n'
               + 'Upgrade: websocket\r\n'
               + 'Connection: Upgrade\r\n'
               + 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n'
               + 'Sec-WebSocket-Version: 13\r\n'
               + '\r\n')
    s.send(request.encode('utf-8'))
    response = s.recv(4096).decode("utf-8")
    print(response)
    s.close()
    if not is_valid_response(response):
        success = False

def test_parallel_connections():
    global success
    num_threads = 100
    threads = []
    for i in range(num_threads):
        thread = threading.Thread(target=send_handshake)
        threads.append(thread)
    for i in range(num_threads):
        threads[i].start()
    for i in range(num_threads):
        threads[i].join()
    return success
