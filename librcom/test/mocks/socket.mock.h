#ifndef ROMI_ROVER_BUILD_AND_TEST_SOCKET_MOCK_H
#define ROMI_ROVER_BUILD_AND_TEST_SOCKET_MOCK_H

#include "fff.h"
#include <sys/socket.h>

DECLARE_FAKE_VALUE_FUNC(int, socket, int, int, int)
DECLARE_FAKE_VALUE_FUNC(int, bind, int, __CONST_SOCKADDR_ARG, socklen_t)
DECLARE_FAKE_VALUE_FUNC(ssize_t, sendto, int, const void *, size_t, int, __CONST_SOCKADDR_ARG, socklen_t)
DECLARE_FAKE_VALUE_FUNC(ssize_t, recvfrom, int, void *__restrict, size_t, int, __SOCKADDR_ARG,  socklen_t *__restrict)
DECLARE_FAKE_VALUE_FUNC(int, connect, int, __CONST_SOCKADDR_ARG, socklen_t)
DECLARE_FAKE_VALUE_FUNC(int, shutdown, int, int)
DECLARE_FAKE_VALUE_FUNC(ssize_t, recv, int, void *, size_t, int)
DECLARE_FAKE_VALUE_FUNC(ssize_t, send, int, const void *, size_t, int)


#endif //ROMI_ROVER_BUILD_AND_TEST_LOG_MOCK_H
