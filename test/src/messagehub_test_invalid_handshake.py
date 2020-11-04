import socket
import time


def is_bad_request(response):
    return (response.find("HTTP/1.1 400 Bad Request") >= 0)


def test_invalid_handshake_missing_version():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    request = ('GET / HTTP/1.1\r\n'
               + 'Upgrade: websocket\r\n'
               + 'Connection: Upgrade\r\n'
               + 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n'
               + '\r\n')
    s.send(request.encode('utf-8'))
    response = s.recv(4096).decode("utf-8")
    print(response)
    s.close()    
    return is_bad_request(response)


def test_invalid_handshake_bad_version():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    request = ('GET / HTTP/1.1\r\n'
               + 'Upgrade: websocket\r\n'
               + 'Connection: Upgrade\r\n'
               + 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n'
               + 'Sec-WebSocket-Version: 12\r\n'
               + '\r\n')
    s.send(request.encode('utf-8'))
    response = s.recv(4096).decode("utf-8")
    print(response)
    s.close()    
    return is_bad_request(response)


def test_invalid_handshake_missing_upgrade():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    request = ('GET / HTTP/1.1\r\n'
               + 'Connection: Upgrade\r\n'
               + 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n'
               + 'Sec-WebSocket-Version: 13\r\n'
               + '\r\n')
    s.send(request.encode('utf-8'))
    response = s.recv(4096).decode("utf-8")
    print(response)
    s.close()    
    return is_bad_request(response)


def test_invalid_handshake_missing_connection():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    request = ('GET / HTTP/1.1\r\n'
               + 'Upgrade: websocket\r\n'
               + 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n'
               + 'Sec-WebSocket-Version: 13\r\n'
               + '\r\n')
    s.send(request.encode('utf-8'))
    response = s.recv(4096).decode("utf-8")
    print(response)
    s.close()    
    return is_bad_request(response)


def test_invalid_handshake_missing_key():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    request = ('GET / HTTP/1.1\r\n'
               + 'Upgrade: websocket\r\n'
               + 'Connection: Upgrade\r\n'
               + 'Sec-WebSocket-Version: 13\r\n'
               + '\r\n')
    s.send(request.encode('utf-8'))
    response = s.recv(4096).decode("utf-8")
    print(response)
    s.close()    
    return is_bad_request(response)


def test_invalid_handshake_bad_key():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 10101))
    request = ('GET / HTTP/1.1\r\n'
               + 'Upgrade: websocket\r\n'
               + 'Connection: Upgrade\r\n'
               + 'Sec-WebSocket-Key: HiThere!\r\n'
               + 'Sec-WebSocket-Version: 13\r\n'
               + '\r\n')
    s.send(request.encode('utf-8'))
    response = s.recv(4096).decode("utf-8")
    print(response)
    s.close()    
    return is_bad_request(response)


def test_invalid_handshake():
    success = test_invalid_handshake_missing_version()
    if success:
        success = test_invalid_handshake_bad_version()
        if success:
            success = test_invalid_handshake_missing_upgrade()
            if success:
                success = test_invalid_handshake_missing_connection()
                if success:
                    success = test_invalid_handshake_missing_key()
                    if success:
                        success = test_invalid_handshake_bad_key()
    return success
