#include "socket.mock.h"

DEFINE_FAKE_VALUE_FUNC(int, socket, int, int, int)
DEFINE_FAKE_VALUE_FUNC(int, bind, int, __CONST_SOCKADDR_ARG, socklen_t)
DEFINE_FAKE_VALUE_FUNC(ssize_t, sendto, int, const void *, size_t, int, __CONST_SOCKADDR_ARG, socklen_t)
DEFINE_FAKE_VALUE_FUNC(ssize_t, recvfrom, int, void *__restrict, size_t, int, __SOCKADDR_ARG,  socklen_t *__restrict)
DEFINE_FAKE_VALUE_FUNC(int, connect, int, __CONST_SOCKADDR_ARG, socklen_t)
DEFINE_FAKE_VALUE_FUNC(int, shutdown, int, int)
DEFINE_FAKE_VALUE_FUNC(ssize_t, recv, int, void *, size_t, int)
DEFINE_FAKE_VALUE_FUNC(ssize_t, send, int, const void *, size_t, int)
