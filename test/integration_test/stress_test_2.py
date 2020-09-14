import socket
import sys


##################################################

def test_1():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', 10101)
    sock.connect(server_address)

    try:

        message = b'GET / HTTP/1.1\r\nUpgrade: websocket\r\nHost: 127.0.0.1:7832\r\nOrigin: http://127.0.0.1:7832\r\nSec-WebSocket-Key: 6rIsB616DkE8dPbZu6mHrg==\r\nSec-WebSocket-Version: 13\r\nConnection: upgrade\r\n\r\n'
    
        sock.sendall(message)

        expected_reply = b'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: Dlb6dUdzkQ9+vYSFzUgrmFg=\r\n\r\n'

        amount_received = 0
        amount_expected = len(expected_reply)
        reply = bytearray()

        while amount_received < amount_expected:
            data = sock.recv(16)
            reply += data
            amount_received += len(data)
            #print('received {!r}'.format(data))
    
        
        print(reply)
    
    finally:
        print('closing socket')
        sock.close()



##################################################

def test_2():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('localhost', 10101)
    sock.connect(server_address)

    try:

        message = b'GET / HTTP/1.1\r\nUpgrade: websocket\r\nHost: 127.0.0.1:7832\r\nOrigin: http://127.0.0.1:7832\r\nSec-WebSocket-Key: 6rIsB616DkE8dPbZu6mHrg==\r\nSec-WebSocket-Version: 12\r\nConnection: upgrade\r\n\r\n'
    
        sock.sendall(message)

        expected_reply = b'HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n'

        amount_received = 0
        amount_expected = len(expected_reply)
        reply = bytearray()

        while amount_received < amount_expected:
            data = sock.recv(16)
            reply += data
            amount_received += len(data)
            #print('received {!r}'.format(data))
    
        
        print(reply)
            
    finally:
        print('closing socket')
        sock.close()


test_2()
