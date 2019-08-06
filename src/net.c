#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "rcom/log.h"
#include "rcom/app.h"
#include "rcom/addr.h"

#include "net.h"

static int posix_wait_data(int socket, int timeout);
static addr_t *posix_socket_addr(int s);

//*********************************************************
// udp socket 

udp_socket_t open_udp_socket()
{
        int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == -1) {
                log_err("open_udp_socket: failed to create the socket");
                return INVALID_UDP_SOCKET;
        }
        
        // zero out the structure
        struct sockaddr_in local_addr;
        int socklen = sizeof(local_addr);
        memset((char *) &local_addr, 0, socklen);
        
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(0);
        inet_aton(app_ip(), &local_addr.sin_addr);
        
        // bind socket to port
        int ret = bind(s, (struct sockaddr*) &local_addr, socklen);
        if (ret == -1) {
                log_err("open_udp_socket: failed to bind the socket");
                close(s);
                return INVALID_UDP_SOCKET;
        }

        return s;
}

void close_udp_socket(udp_socket_t s)
{
        close(s);
}

addr_t *udp_socket_addr(udp_socket_t s)
{
        return posix_socket_addr(s);
}

int udp_socket_send(udp_socket_t socket, addr_t *addr, data_t *data)
{
        int ret;
        int addrlen = sizeof(struct sockaddr_in);
        
        ret = sendto(socket, data_packet(data),
                     PACKET_HEADER + data_len(data), 0,
                     (struct sockaddr *) addr, addrlen);
        if (ret == -1) {
                char b[64];
                log_err("messenger_send: sendto() to %s failed: %s",
                        addr_string(addr, b, 64),
                        strerror(errno));
                return -1;
        }
        
        return 0;
}

int udp_socket_wait_data(udp_socket_t socket, int timeout)
{
        return posix_wait_data(socket, timeout);
}

int udp_socket_read(udp_socket_t socket, data_t *data, addr_t *addr)
{
        int addrlen = sizeof(addr_t);
        int len;

        len = recvfrom(socket,
                       data_packet(data),
                       PACKET_MAXLEN, 0,
                       (struct sockaddr *) addr,
                       &addrlen);
        if (len < 0) {
                log_err("udp_socket_read: recvfrom failed");
                return -1;
        }
        data_set_len(data, len - PACKET_HEADER);
        return 0; 
}

//*********************************************************
// tcp socket 

tcp_socket_t open_tcp_socket(addr_t *addr)
{
        int addrlen = sizeof(struct sockaddr_in);
        int s;
        int ret;
        
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == -1) {
                log_err("open_tcp_socket: failed to create the socket");
                return -1;
        }

        ret = connect(s, (struct sockaddr *) addr, addrlen);
        if (ret < 0) {
                log_err("open_tcp_socket: failed to bind the socket");
                return -1;
        }
        
        return s;
}

void close_tcp_socket(tcp_socket_t socket)
{
        ssize_t n;
        char buf[512];
        if (socket != -1) {
                shutdown(socket, SHUT_RDWR);
                while (1) {
                        n = recv(socket, buf, 512, 0);
                        if (n <= 0) break;
                }
                close(socket);
        }
}

addr_t *tcp_socket_addr(tcp_socket_t s)
{
        return posix_socket_addr(s);
}

int tcp_socket_send(tcp_socket_t socket, const char *data, int len)
{
        int ret;
        int sent = 0;
        
        if (socket < 0) {
                log_err("http_send: invalid socket");
                return -1;
        }

        //log_debug("http_send: sending: %.*s", len, data);

        while (sent < len) {
                // Using MSG_NOSIGNAL to prevent SIGPIPE signals in
                // case the client closes the connection before all
                // the data is sent.
                ret = send(socket, data + sent, len - sent, MSG_NOSIGNAL);
                //log_err("socket_send: len=%d, sent=%d, len-send=%d, ret=%d",
                // len, sent, len - sent, ret);
                if (ret < 0) {
                        log_err("socket_send: send failed: %s", strerror(errno));
                        return -1;
                }
                if (ret == 0) {
                        log_err("socket_send: send() returned zero");
                        return -1;
                }
                sent += ret;
        }
        return 0;
}

int tcp_socket_wait_data(tcp_socket_t socket, int timeout)
{
        return posix_wait_data(socket, timeout);
}

// Returns:
// -2: timeout
// -1: error
// 0: shutdown
// >0: amount of data received
int tcp_socket_recv(tcp_socket_t socket, char *data, int len)
{
        int received = recv(socket, data, len, 0);
        if (received < 0) {
                log_err("tcp_socket_recv: recv failed: %s", strerror(errno));
                return (errno == EAGAIN)? -2 : -1;
        } else if (received == 0) {
                log_err("tcp_socket_recv: socket shut down");
                return 0;
        }
        //log_debug("http_recv: len=%d data='%.*s'", received, received, data);        
        return received;
}

int tcp_socket_read(tcp_socket_t socket, char *data, int len)
{
        int received = 0;
        while (received < len) {
                int n = tcp_socket_recv(socket, data + received, len - received);
                if (n == -2) continue;
                if (n < 0) return -1;
                if (n == 0) return received;
                received += n;
        }
        return received;
}


//*********************************************************
// tcp server socket

tcp_socket_t open_server_socket(addr_t* addr)
{
        int ret;
        int socklen = sizeof(struct sockaddr_in);

        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == -1) {
                log_err("open_server_socket: failed: %s", strerror(errno));
                return INVALID_TCP_SOCKET;
        }

        ret = bind(s, (struct sockaddr *) addr, socklen);
        if (ret == -1) {
                log_err("open_server_socket: bind failed: %s", strerror(errno));
                return INVALID_TCP_SOCKET;
        }

        if (addr_port(addr) == 0)
                getsockname(s, (struct sockaddr *) addr, &socklen);
        
        ret = listen(s, 10);
        if (ret == -1) {
                log_err("open_server_socket: listen failed: %s", strerror(errno));
                return INVALID_TCP_SOCKET;
        }
        
        return s;
}

tcp_socket_t server_socket_accept(tcp_socket_t s)
{
        int addrlen = sizeof(struct sockaddr_in);
        struct sockaddr_in addr;
        int client = -1;

        // The code waits for incoming connections for one
        // second. 
        if (posix_wait_data(s, 1)) {
                //log_debug("server_socket_accept: calling accept");
                client =  accept(s, (struct sockaddr*) &addr, &addrlen);
                if (client == -1) {
                        // Server socket is probably being closed
                        // FIXME: is this true?
                        log_err("server_socket_accept: accept failed: %s (socket %d)",
                                strerror(errno), s); 
                        return INVALID_TCP_SOCKET;
                }
                //log_debug("server_socket_accept: new connection");
                return client;
        } else
                return TCP_SOCKET_TIMEOUT;
}


//*********************************************************
// posix  

static int posix_wait_data(int socket, int timeout)
{
        struct timeval timev;
        struct timeval* tp = NULL;
        fd_set readset;
        int ret = 0;
        
        if (timeout < 0) {
                log_err("posix_wait_data: invalid value for timeout: %d", timeout);
                return -1;
        }
        
        FD_ZERO(&readset);
        FD_SET(socket, &readset);

        // man select(2): If both fields of the timeval structure are
        // zero, then select() returns immediately.  (This is useful
        // for polling.)
        timev.tv_sec = timeout;
        timev.tv_usec = 0;
        tp = &timev;

        
        ret = select(socket+1, &readset, NULL, NULL, tp);
        if (ret < 0) {
                log_err("udp_socket_wait_data: error: %s", strerror(errno));
                return -1;
        } else if (ret == 0) {
                //log_err("http_wait_data: time out");
                return 0;
        }
        return 1; 
}

static addr_t *posix_socket_addr(int s)
{
        struct sockaddr_in local_addr;
        int socklen = sizeof(&local_addr);        
        memset((char *) &local_addr, 0, socklen);        
        getsockname(s, (struct sockaddr*) &local_addr, &socklen);
        return new_addr(inet_ntoa(local_addr.sin_addr),
                        ntohs(local_addr.sin_port));
}
