#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <r.h>

#include "rcom/app.h"
#include "rcom/messagehub.h"
#include "rcom/addr.h"

#include "util_priv.h"
#include "http.h"
#include "http_parser.h"
#include "net.h"
#include "messagehub_priv.h"
#include "messagelink_priv.h"

/**********************************************
 * utility functions
 */

#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

static void _make_mask(uint8_t *mask)
{
        r_random(mask, 4);
}

static char *_make_key()
{
        uint8_t bytes[11];
        r_random(bytes, 10);
        bytes[10] = 0;
        return encode_base64(bytes);
}

static char *_make_accept(char *key)
{
        unsigned char buffer[100];
        unsigned char digest[21];
        snprintf(buffer, 100, "%s%s", key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        buffer[99] = 0;
        SHA1(buffer, digest);
        digest[20] = 0;
        return encode_base64(digest);
}

/******************************************************************************/

typedef struct _ws_frame_t {
        uint8_t fin : 1;
        uint8_t rsv1 : 1;
        uint8_t rsv2 : 1;
        uint8_t rsv3 : 1;
        uint8_t opcode : 4;
        uint8_t mask : 1;
        uint8_t length : 7;
} __attribute__((packed, aligned(2))) ws_frame_t;

/*****************************************************/

enum {
        WS_CONTINUTATION = 0,
        WS_TEXT = 1,
        WS_BINARY = 2,
        WS_CLOSE = 8,
        WS_PING = 9,
        WS_PONG = 10
};

enum {
        WS_CREATED,
        WS_CLIENT_CONNECTING,
        WS_SERVER_CONNECTING,
        WS_OPEN,
        WS_CLOSING,
        WS_CLOSE_RECEIVED,
        WS_FINALIZING_CLOSE,
        WS_CLOSED
};

enum {
        k_header_new = 0,
        k_header_field,
        k_header_value
};

const char *get_state_str(int state)
{
        switch (state) {
        case WS_CREATED: return "WS_CREATED";
        case WS_CLIENT_CONNECTING: return "WS_CLIENT_CONNECTING";
        case WS_SERVER_CONNECTING: return "WS_SERVER_CONNECTING";
        case WS_OPEN: return "WS_OPEN";
        case WS_CLOSING: return "WS_CLOSING";
        case WS_CLOSE_RECEIVED: return "WS_CLOSE_RECEIVED";
        case WS_FINALIZING_CLOSE: return "WS_FINALIZING_CLOSE";
        case WS_CLOSED: return "WS_CLOSED";
        default: return "UNKNOWN!";
        }
}

/*****************************************************/
/* messagelink_t
 * 
 * The messagelink_t structure is used by the streamer to track
 * all the links that registered for a specific stream.
 */

typedef struct _messagelink_t {
        messagehub_t *hub;
        tcp_socket_t socket;
        addr_t *addr;
        addr_t *remote_addr;
        int cont;

        int state;
        int close_code;

        int parser_header_state;
        list_t *headers; 
        http_header_t *cur_header;
        int http_status;
        
        int is_client;
        membuf_t *in;
        membuf_t *out;
        
        messagelink_onmessage_t onmessage;
        messagelink_onpong_t onpong;
        messagelink_onclose_t onclose;
        void *userdata;

        thread_t *thread;
        mutex_t *send_mutex;
        mutex_t *state_mutex;

} messagelink_t;

static void owner_messagelink_close(messagelink_t *link, int code);

static void messagelink_run(messagelink_t *link);
static int messagelink_read_message(messagelink_t *link, ws_frame_t *frame);

static int messagelink_send_close(messagelink_t *link, int code);
static int messagelink_send_pong(messagelink_t *link, membuf_t *payload);

// Receive messages If an error occurs, the function returns
// json_null(). In that case, the connection will have been closed and
// should no longer be used.
static json_object_t _messagelink_read(messagelink_t *link);
static void messagelink_stop_background(messagelink_t *link);


messagelink_t *new_messagelink(messagelink_onmessage_t onmessage,
                               messagelink_onclose_t onclose,
                               void *userdata)
{
        messagelink_t *link;

        link = r_new(messagelink_t);
        if (link == NULL) return NULL;
        
        link->hub = NULL;
        link->socket = INVALID_TCP_SOCKET;
        link->addr = new_addr0();
        link->remote_addr = NULL;
        link->cont = 1;
        link->state = WS_CREATED;
        link->close_code = 0;
        link->is_client = 0;
        link->onmessage = onmessage;
        link->onpong = NULL;
        link->onclose = onclose;
        link->userdata = userdata;
        link->thread = NULL;
        link->http_status = 0;

        link->send_mutex = new_mutex();
        link->state_mutex = new_mutex();

        link->out = new_membuf();
        link->in = new_membuf();
        if (link->out == NULL || link->in == NULL) {
                delete_messagelink(link);
                return NULL;
        }
        
        return link;
}

void delete_messagelink(messagelink_t *link)
{
        //r_debug("delete_messagelink");
        if (link) {
                if (link->hub)
                        messagehub_remove_link(link->hub, link);
                if (link->state_mutex) {
                        mutex_lock(link->state_mutex);
                        if (link->state == WS_OPEN)
                                owner_messagelink_close(link, 1001);
                        mutex_unlock(link->state_mutex);
                }
                if (link->thread)
                        messagelink_stop_background(link);
                if (link->in)
                        delete_membuf(link->in);
                if (link->out)
                        delete_membuf(link->out);
                if (link->addr)
                        delete_addr(link->addr);
                if (link->remote_addr)
                        delete_addr(link->remote_addr);
                if (link->send_mutex)
                        delete_mutex(link->send_mutex);
                if (link->state_mutex)
                        delete_mutex(link->state_mutex);
                for (list_t *l = link->headers; l != NULL; l = list_next(l)) {
                        http_header_t *h = list_get(l, http_header_t);
                        delete_http_header(h);
                }
                delete_list(link->headers);
                r_delete(link);
        }
}

addr_t *messagelink_addr(messagelink_t *link)
{
        return link->addr;
}

void messagelink_set_userdata(messagelink_t *link, void *userdata)
{
        link->userdata = userdata;
}

void *messagelink_get_userdata(messagelink_t *link)
{
        return link->userdata;
}

void messagelink_set_onmessage(messagelink_t *link, messagelink_onmessage_t onmessage)
{
        link->onmessage = onmessage;
}

void messagelink_set_onpong(messagelink_t *link, messagelink_onpong_t onpong)
{
        link->onpong = onpong;
}

void messagelink_set_onclose(messagelink_t *link, messagelink_onclose_t onclose)
{
        link->onclose = onclose;
}

static int messagelink_message_complete(http_parser *parser)
{
        messagelink_t *link = (messagelink_t *) parser->data;
        link->cont = 0;
        return 0;
}

static int messagelink_on_header_field(http_parser *parser, const char *data, size_t length)
{
        messagelink_t *link = (messagelink_t *) parser->data;
        
        if (link->cur_header == NULL
            || link->parser_header_state == k_header_new
            || link->parser_header_state == k_header_value) {
                
                link->cur_header = new_http_header(data, length);
                if (link->cur_header == NULL)
                        return -1;
                link->headers = list_prepend(link->headers, link->cur_header);
                link->parser_header_state = k_header_field;
        } else {
                // header_state is k_header_field
                if (http_header_append_name(link->cur_header, data, length) != 0)
                        return -1;
        }
        return 0;
}

static int messagelink_on_header_value(http_parser *parser, const char *data, size_t length)
{
        messagelink_t *link = (messagelink_t *) parser->data;

        if (link->cur_header == NULL
            || link->parser_header_state == k_header_new) {
                r_err("messagelink_on_header_value: got header value before header field");
                return 0;
        } else {
                // header_state is k_header_field or k_header_value
                http_header_append_value(link->cur_header, data, length);
                link->parser_header_state = k_header_value;
        }
        return 0;
}

static http_header_t *messagelink_get_header(messagelink_t *link, const char *name)
{
        list_t * l = link->headers;
        while (l) {
                http_header_t *h = list_get(l, http_header_t);
                if (rstreq(name, h->name))
                        return h;
                l = list_next(l);
        }
        return NULL;
}

static int messagelink_on_headers_complete(http_parser *parser)
{
        messagelink_t *link = (messagelink_t *) parser->data;
        link->cont = 0;
        return 0;
}



/**********************************************
 * close handshake
 */

static void messagelink_close_socket(messagelink_t *link);
static int owner_messagelink_send_close(messagelink_t *link, int code);
static int owner_messagelink_wait_close(messagelink_t *link);

int client_messagelink_disconnect(messagelink_t *link)
{
        mutex_lock(link->state_mutex);
        owner_messagelink_close(link, 1001);
        mutex_unlock(link->state_mutex);
        return 0;
}

static void messagelink_close_socket(messagelink_t *link)
{
        if (link->socket != INVALID_TCP_SOCKET) {
                close_tcp_socket(link->socket);
                link->socket = INVALID_TCP_SOCKET;
                link->state = WS_CLOSED;
                if (link->addr)
                        delete_addr(link->addr);
                link->addr = NULL;
        }
}

// This function is called when the close handshake in initiated by
// the owner of this link.
static void owner_messagelink_close(messagelink_t *link, int code)
{
        //r_debug("owner_messagelink_close: code %d", code);

        if (link->state == WS_CLOSED
            || link->state == WS_CLOSING)
                return;
        
        link->state = WS_CLOSING;
        messagelink_stop_background(link);

        //r_info("owner_messagelink_close: sending close");
        int err = owner_messagelink_send_close(link, code);
        if (err) {
                r_warn("owner_messagelink_close: failed to send close. Not waiting for a reply");
        } else {
                err = owner_messagelink_wait_close(link);
                if (err == -1)
                        r_warn("owner_messagelink_close: error while reading reply");
                else if (err == -2)
                        r_warn("owner_messagelink_close: time-out while reading reply");
                else {
                        //r_debug("owner_messagelink_close: got close reply");
                }
        }
        
        if (link->is_client)
                clock_sleep(4);
        
        messagelink_close_socket(link);
}

static int owner_messagelink_send_close(messagelink_t *link, int code)
{
        //r_debug("owner_messagelink_send_close");
        link->close_code = code;
        return messagelink_send_close(link, code);
}

// Returns
// 0: got close message
// -1: a read error occured
// -2: time-out, waiting too long for reply
static int owner_messagelink_wait_close(messagelink_t *link)
{
        //r_debug("owner_messagelink_wait_close");
        
        double start_time = clock_time();
        ws_frame_t frame;
        int err;
        
        while (1) {
                if (tcp_socket_wait_data(link->socket, 1) == 1) {
                        err = messagelink_read_message(link, &frame);
                        if (err < 0)
                                return -1;
                        if (frame.opcode == WS_CLOSE)
                                return 0;
                }
                if (clock_time() - start_time > 5.0)
                        return -2;        
        }
}

// Send a close message but don't bother waiting for the returning
// close message.
static void owner_messagelink_close_oneway(messagelink_t *link, int code)
{
        //r_debug("owner_messagelink_close_oneway: code %d", code);
        link->state = WS_CLOSED;
        messagelink_send_close(link, code);
        if (link->is_client)
                clock_sleep(4);
        messagelink_close_socket(link);
}

//----

static int remote_messagelink_close_code(messagelink_t *link)
{
        uint16_t code = 1005;
        if (membuf_len(link->in) >= 2) {
                uint16_t tmp;
                char* p = (char *) &tmp;
                memcpy(p, membuf_data(link->in), 2);
                code = ntohs(tmp);
        }
        return (int) code;
}

static int remote_messagelink_close(messagelink_t *link)
{
        //r_debug("remote_messagelink_close");
        
        if (link->state == WS_OPEN)  {
                // If the connection is in the open state, the remote
                // link started the closing handshake. Store the
                // close code and send a close message back.
                link->close_code = remote_messagelink_close_code(link);
                messagelink_send_close(link, link->close_code);
                //r_debug("remote_messagelink_close: code %d", link->close_code);
        }

        // If this is a client link, wait a bit so that the server
        // shuts down the tcp connection first.
        if (link->is_client)
                clock_sleep(4);

        link->state = WS_CLOSED;
        messagelink_close_socket(link);
        if (link->onclose)
                link->onclose(link->userdata, link);

        messagelink_stop_background(link);

        if (link->hub)
                delete_messagelink(link);
        
        return 0;
}

/**********************************************
 * read
 */

static int messagelink_read_frame(messagelink_t *link, ws_frame_t *frame)
{
        unsigned char b[2];

        //r_debug("messagelink_read_frame");
        int received = tcp_socket_read(link->socket, (char*) b, 2);
        if (received != 2) {
                return -1;
        }
        frame->fin = (b[0] & 0x80) >> 7;
        frame->opcode = (b[0] & 0x0f);
        frame->mask = (b[1] & 0x80) >> 7;
        frame->length = (b[1] & 0x7f);
        return 0;
}

static void _print_message(ws_frame_t *frame, membuf_t* m)
{
        switch (frame->opcode) {
        case 0: printf("continuation\n"); break; 
        case 1: printf("text\n"); break; 
        case 2: printf("binary\n"); break; 
        case 3: 
        case 4: 
        case 5: 
        case 6: 
        case 7: printf("** reserved **\n"); break; 
        case 8: printf("close\n"); break; 
        case 9: printf("ping\n"); break; 
        case 10: printf("pong\n"); break; 
        case 11:
        case 12:
        case 13:
        case 14:
        case 15: printf("** reserved **\n"); break; 
        }

        if (frame->mask) 
                printf("masked\n");
                
        printf("length (7-bits) %d\n", frame->length);        
        printf("length %d\n", membuf_len(m));
        if (frame->opcode == WS_TEXT)
                printf("payload: %.*s\n", (int) membuf_len(m), membuf_data(m));
}

// Returns:
// 0: no error 
// -1: read error 
// -2: message too big 
static int messagelink_read_message(messagelink_t *link, ws_frame_t *frame)
{

        // FIXME: user a read_mutex to serialize access
        
        int received;
        uint64_t length;
        int err;
        
        err = messagelink_read_frame(link, frame);
        if (err != 0) return err;
        
        if (frame->length < 126) {
                length = frame->length;
                
        } else if (frame->length == 126) {
                uint16_t netshort;
                received = tcp_socket_read(link->socket, (char*) &netshort, 2);
                if (received != 2) {
                        r_err("messagelink_read_message: tcp_socket_read failed (2), received %d", received);
                        return -1;
                }
                length = ntohs(netshort);
                
        } else if (frame->length == 127) {
                uint64_t netlong;
                received = tcp_socket_read(link->socket, (char*) &netlong, 8);
                if (received != 8) {
                        r_err("messagelink_read_message: tcp_socket_read failed (3), received %d", received);
                        return -1;
                }
                length = ntohll(netlong);
        }
        
        uint8_t mask[4] = {0};
        
        if (frame->mask) {
                received = tcp_socket_read(link->socket, (char*) &mask, 4);
                if (received != 4) {
                        r_err("messagelink_read_message: tcp_socket_read failed (4), received %d", received);
                        return -1;
                }
        }

        if (length > 16777216) {
                r_err("messagelink_read_message: message too large (%lu > 16MB)", length);
                return -2;
        }

        uint64_t num_read = 0, num_to_read;
        char buffer[1024];
        int j = 0;
        
        membuf_clear(link->in);
        
        while (num_read < length) {
                num_to_read = length - num_read;
                if (num_to_read > 1024)
                        num_to_read = 1024;
                
                received = tcp_socket_recv(link->socket, buffer, num_to_read);
                if (received < 0) {
                        r_err("messagelink_read_message: tcp_socket_recv failed (5), received %d", received);
                        printf("received: %.*s\n", (int) received, buffer);
                        return received;
                }
                if (received == 0) {
                        r_err("messagelink_read_message: socket closed while reading message");
                        return -1;
                }
                
                num_read += received;

                if (!link->is_client) {
                        for (int i = 0; i < received; i++) {
                                buffer[i] = (buffer[i] ^ mask[j++]);
                                if (j == 4) j = 0;
                        }
                }
                
                if (membuf_append(link->in, buffer, received) != 0)
                        return -1;
        }
        
        //_print_message(frame, link->in);
        
        return 0;
}

// Returns:
// 0: no error 
// -1: read error 
// -2: message too big 
// -3: connection closed 
// -4: app quitting 
static int messagelink_try_read_message(messagelink_t *link, ws_frame_t *frame)
{
        while (1) {
                if (link->state != WS_OPEN)
                        return -3;
                if (app_quit())
                        return -4;
                if (tcp_socket_wait_data(link->socket, 1))
                        return messagelink_read_message(link, frame);
        }
}

json_object_t messagelink_send_command_f(messagelink_t *link, const char *format, ...)
{
        int err;
        va_list ap;
        int len;
        json_object_t r = json_null();
        
        va_start(ap, format);
        len = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        membuf_lock(link->out);
        membuf_clear(link->out);

        err = membuf_assure(link->out, len+1);

        if (err == 0) {
                va_start(ap, format);
                err = membuf_vprintf(link->out, format, ap);
                va_end(ap);
        }

        if (err == 0) 
                err = messagelink_send_text(link,
                                            membuf_data(link->out),
                                            membuf_len(link->out));

        if (err == 0) 
                r = messagelink_read(link);
        
        membuf_unlock(link->out);
        
        return r;
}

json_object_t messagelink_read(messagelink_t *link)
{
        if (link->onmessage != NULL
            || link->thread != NULL) {
                r_err("messagelink_read called on a link with background event thread");
                return json_null();
        }
        return _messagelink_read(link);
}

static json_object_t _messagelink_read(messagelink_t *link)
{
        if (link->state != WS_OPEN) {
                r_warn("messagelink_read: link not in open or closing state (state=%s).",
                         get_state_str(link->state));
                clock_sleep(0.05);
                return json_null();
        }

        while (link->state == WS_OPEN && !app_quit()) {

                ws_frame_t frame;
                int err = messagelink_try_read_message(link, &frame);
                
                if (err == -1) { 
                        r_warn("messagelink_read: a read error occured, closing connection.");
                        owner_messagelink_close_oneway(link, 1011);
                        return json_null();
                }
                if (err == -2) {
                        r_warn("messagelink_read: message too big, closing connection.");
                        owner_messagelink_close_oneway(link, 1009);
                        return json_null();
                }
                if (err == -3 || err == -4) {
                        //r_debug("messagelink_read: connection close or app quitting.");
                        return json_null();
                }
        
                // https://tools.ietf.org/html/rfc6455#section-5.1
                //
                // "... a client MUST mask all frames that it sends to the
                // server [...].  The server MUST close the connection upon
                // receiving a frame that is not masked.  In this case, a
                // server MAY send a Close frame with a status code of 1002
                // (protocol error) ...
        
                if (!link->is_client && membuf_len(link->in) > 0 && !frame.mask) {
                        r_err("messagelink_read: client sent a frame that was not masked. Closing connection.");
                        owner_messagelink_close(link, 1002);
                        return json_null();
                }
                
                switch (frame.opcode) {
                case WS_TEXT:
                        //r_debug("messagelink_read: received text event.");
                        
                        membuf_append_zero(link->in);
                        //r_debug("messagelink_read: %s", membuf_data(link->in));

                        json_object_t evt = json_parse(membuf_data(link->in));
                        if (json_isnull(evt)) {
                                r_warn("messagelink_read: invalid message");
                                //owner_messagelink_close(link, 1003);
                                return json_null();
                        }
                        return evt;
                        
                case WS_BINARY:
                        r_warn("messagelink_read: received a binary event. Shouldn't happen.");
                        owner_messagelink_close(link, 1003);
                        return json_null();
                        
                case WS_CLOSE:
                        //r_debug("messagelink_read: Received close event.");
                        remote_messagelink_close(link);
                        return json_null();
                        
                case WS_CONTINUTATION:
                        r_warn("messagelink_read: Received a continuation event. Shouldn't happen.");
                        owner_messagelink_close(link, 1003);
                        break;
                        
                case WS_PING:
                        r_info("messagelink_read: ping message: %.*s",
                                 membuf_len(link->in), membuf_data(link->in));
                        r_info("messagelink_read: sending pong");
                        if (messagelink_send_pong(link, link->in) != 0)
                                r_err("messagelink_read: Failed to send pong message");
                        break;
                        
                case WS_PONG:
                        r_info("messagelink_read: got pong message");
                        if (link->onpong)
                                link->onpong(link, link->userdata,
                                                 membuf_data(link->in),
                                                 membuf_len(link->in));
                        break;
                }
        }
        
        return json_null();
}

static void messagelink_run(messagelink_t *link)
{
        //r_debug("messagelink_run");
        
        while (link->state == WS_OPEN && !app_quit()) {
                
                json_object_t message = _messagelink_read(link);
                if (!json_isnull(message)) {
                        link->onmessage(link->userdata, link, message);
                        json_unref(message);
                }
        }
}

void messagelink_read_in_background(messagelink_t *link)
{
        //r_debug("messagelink_read_in_background");
        if (link->thread == NULL && link->onmessage != NULL) {
                link->thread = new_thread((thread_run_t) messagelink_run, link, 0, 0);
        }
}

static void messagelink_stop_background(messagelink_t *link)
{
        //r_debug("messagelink_stop_background");
        if (link->thread) {
                //r_debug("messagelink_stop_background: joining read thread");
                thread_join(link->thread);
                //r_debug("messagelink_stop_background: joined");
                delete_thread(link->thread);
                link->thread = NULL;
        }
}

/**********************************************
 * send
 */

static int frame_make_header(uint8_t *frame, int opcode, int masked,
                             uint8_t *mask, uint64_t length)
{
        int n = 0;
        
        frame[n++] = 0x80 | opcode;
        
        if (length < 126) {
                // https://tools.ietf.org/html/rfc6455#section-5.1
                // A server MUST NOT mask any frames that it sends to
                //the client.
                if (masked)
                        frame[n++] = 0x80 | (length & 0x7f);
                else
                        frame[n++] = (length & 0x7f);
                
        } else if (length <= 65536) {
                uint16_t netshort = htons((uint16_t)length);
                frame[n++] = (masked)? 254 : 126;
                frame[n++] = (netshort & 0x00ff);
                frame[n++] = (netshort & 0xff00) >> 8;
                
        } else {
                uint64_t netlong = htonll(length);
                frame[n++] = (masked)? 255 : 127;
                frame[n++] = (netlong & 0x00000000000000ff);
                frame[n++] = (netlong & 0x000000000000ff00) >> 8;
                frame[n++] = (netlong & 0x0000000000ff0000) >> 16;
                frame[n++] = (netlong & 0x00000000ff000000) >> 24;
                frame[n++] = (netlong & 0x000000ff00000000) >> 32;
                frame[n++] = (netlong & 0x0000ff0000000000) >> 40;
                frame[n++] = (netlong & 0x00ff000000000000) >> 48;                
                frame[n++] = (netlong & 0xff00000000000000) >> 56;
        }

        if (masked) {
                frame[n++] = mask[0];
                frame[n++] = mask[1];
                frame[n++] = mask[2];
                frame[n++] = mask[3];
        }

        return n;
}

static int frame_append_payload(membuf_t *buf, int masked, uint8_t *mask, const char *data, uint64_t length)
{
        int err;
        if (masked) {
                int j = 0;
                char buffer[1024];
                uint64_t sent = 0;
                
                while (sent < length) {
                        uint64_t n = length - sent;
                        if (n > 1024) n = 1024;
                        
                        for (int i = 0; i < n; i++) {
                                buffer[i] = data[sent + i] ^ mask[j++];
                                if (j == 4) j = 0;
                        }
                        err = membuf_append(buf, buffer, n);
                        if (err) break;
                        sent += n;
                }
        } else {
                err = membuf_append(buf, data, length);
        }
        return err;
}

static int frame_make(membuf_t *buf, int opcode, int masked,
                      const char *data, uint64_t length)
{
        uint8_t frame[14];
        int frame_size;
        uint8_t mask[4];
        
        if (masked)
                _make_mask(mask);

        frame_size = frame_make_header(frame, opcode, masked, mask, length);
        int err = membuf_append(buf, frame, frame_size);

        if (err == 0)
                frame_append_payload(buf, masked, mask, data, length);

        return err;
}

static int messagelink_send_close(messagelink_t *link, int code)
{
        char data[2];
        uint16_t c = htons((uint16_t)code);
        data[0] = (c & 0x00ff);
        data[1] = (c & 0xff00) >> 8;

        int masked = link->is_client;
        int err;
        
        membuf_lock(link->out);
        membuf_clear(link->out);
        err = frame_make(link->out, WS_CLOSE, masked, data, 2);

        if (err == 0) {
                mutex_lock(link->send_mutex);
                err = tcp_socket_send(link->socket, membuf_data(link->out), membuf_len(link->out));
                mutex_unlock(link->send_mutex);
        }
        
        membuf_unlock(link->out);
        
        return err;
}

int messagelink_send_ping(messagelink_t *link, const char *data, int len)
{
        int err;
        int masked = link->is_client;
        
        membuf_lock(link->out);
        membuf_clear(link->out);
        err = frame_make(link->out, WS_PING, masked, data, len);
        
        if (err == 0) {
                mutex_lock(link->send_mutex);
                err = tcp_socket_send(link->socket, membuf_data(link->out), membuf_len(link->out));
                mutex_unlock(link->send_mutex);
        }
        
        membuf_unlock(link->out);
        
        return err;
}

static int messagelink_send_pong(messagelink_t *link, membuf_t *payload)
{
        int err;
        int masked = link->is_client;

        r_debug("messagelink_send_pong");
        
        membuf_lock(link->out);
        membuf_clear(link->out);
        err = frame_make(link->out, WS_PONG, masked, membuf_data(payload), membuf_len(payload));

        if (err == 0) {        
                mutex_lock(link->send_mutex);
                err = tcp_socket_send(link->socket, membuf_data(link->out), membuf_len(link->out));
                mutex_unlock(link->send_mutex);
        }
        
        membuf_unlock(link->out);
        
        return err;
}

static int messagelink_send_locked(messagelink_t *link, data_t* data)
{
        //r_debug("messagelink_send_packet");
        
        if (link->socket == INVALID_TCP_SOCKET
            || link->state != WS_OPEN) {
                return -2;
        }
        
}

int messagelink_send_text(messagelink_t *link, const char *data, int length)
{
        uint8_t frame[14];
        int frame_size;
        int masked = link->is_client;
        uint8_t mask[4];
        int n, err;

        //r_debug("messagelink_send_text: %.*s", length, data);

        if (link->socket == INVALID_TCP_SOCKET
            || link->state != WS_OPEN) {
                return -2;
        }
        
        if (masked) _make_mask(mask);

        mutex_lock(link->send_mutex);
        
        frame_size = frame_make_header(frame, WS_TEXT, masked, mask, length);
        if (frame_size > 0)
                err = tcp_socket_send(link->socket, frame, frame_size);

        if (err) goto return_error;

        // Send the data directly in blocks of maximum 1k
        int sent = 0;
        const char *p = data;
        int j = 0;
        char buffer[1024];

        while (sent < length) {
                
                //r_debug("messagelink_send_message: sent=%llu, length=%llu", sent, length);
                
                uint64_t n = sizeof(buffer); 
                if (sent + n > length)
                        n = length - sent;

                if (masked) {
                        for (int i = 0; i < (int) n; i++) {
                                buffer[i] = (p[i] ^ mask[j++]);
                                if (j == 4) j = 0;
                        }
                        if (tcp_socket_send(link->socket, buffer, (int) n) != 0)
                                goto return_error;
                        
                } else {
                        if (tcp_socket_send(link->socket, p, (int) n) != 0)
                                goto return_error;
                }
                
                sent += n;
                p += n;
        }
        
        mutex_unlock(link->send_mutex);
        return 0;

return_error:
        mutex_unlock(link->send_mutex);
        return -1;
}

int messagelink_send_num(messagelink_t *link, double value)
{
        int err;
        membuf_lock(link->out);
        membuf_clear(link->out);
        err = membuf_printf(link->out, "%f", value);
        if (err == 0)
                err = messagelink_send_text(link, membuf_data(link->out), membuf_len(link->out));
        membuf_unlock(link->out);
        return err;
}

int messagelink_send_str(messagelink_t *link, const char* value)
{
        int err;
        membuf_t *t = escape_string(value);
        if (t == NULL) return -1;
        membuf_lock(link->out);
        membuf_clear(link->out);
        err = membuf_printf(link->out, "\"%s\"", membuf_data(t));
        if (err == 0)
                err = messagelink_send_text(link, membuf_data(link->out), membuf_len(link->out));
        membuf_unlock(link->out);
        delete_membuf(t);
        return err;
}

static int32 messagelink_serialise(messagelink_t *link, const char* s, int32 len)
{
        return membuf_append(link->out, s, len);
}

int messagelink_send_obj(messagelink_t *link, json_object_t value)
{
        int err;
        membuf_lock(link->out);
        membuf_clear(link->out);
        err = json_serialise(value, 0, (json_writer_t) messagelink_serialise, link);
        if (err == 0)
                err = messagelink_send_text(link, membuf_data(link->out), membuf_len(link->out));
        membuf_unlock(link->out);
        return err;
}

int messagelink_send_f(messagelink_t *link, const char *format, ...)
{
        int err;
        va_list ap;
        int len;
        
        va_start(ap, format);
        len = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        membuf_lock(link->out);
        membuf_clear(link->out);

        err = membuf_assure(link->out, len+1);

        if (err == 0) {
                va_start(ap, format);
                err = membuf_vprintf(link->out, format, ap);
                va_end(ap);
        }

        if (err == 0) 
                err = messagelink_send_text(link, membuf_data(link->out), membuf_len(link->out));
        
        membuf_unlock(link->out);
        
        return err;
}

int messagelink_send_v(messagelink_t *link, const char* format, va_list ap)
{
        int err;        
        membuf_lock(link->out);
        membuf_clear(link->out);
        // TODO: can we assure that the buffer has enough space, using vnsprintf(NULL,0,format,ap) first?
        err = membuf_vprintf(link->out, format, ap); 
        if (err == 0) 
                err = messagelink_send_text(link, membuf_data(link->out), membuf_len(link->out));
        membuf_unlock(link->out);
        return err;
}

/**********************************************
 * code specific to server-side link
 */

messagelink_t *server_messagelink_connect(messagehub_t *hub, tcp_socket_t socket);

static int server_messagelink_open_socket(messagelink_t *link,
                                          messagehub_t *hub,
                                          tcp_socket_t socket);
static int server_messagelink_open_websocket(messagelink_t *link);
static int server_messagelink_parse_request(messagelink_t *link);
static int server_messagelink_validate_request(messagelink_t *r);
static int server_messagelink_upgrade_connection(messagelink_t *link);

messagelink_t *server_messagelink_connect(messagehub_t *hub, tcp_socket_t socket)
{
        // The callbacks will be set later, in onconnect().
        messagelink_t *link = new_messagelink(NULL, NULL, NULL);
        if (link == NULL) { 
                r_err("server_messagelink_connect: out of memory");
                close_tcp_socket(socket);
                return NULL;
        }
        
        if (server_messagelink_open_socket(link, hub, socket) != 0) {
                messagelink_close_socket(link);
                delete_messagelink(link);
                return NULL;
        }
        
        if (server_messagelink_open_websocket(link) != 0) {
                messagelink_close_socket(link);
                delete_messagelink(link);
                return NULL;
        }
        
        link->state = WS_OPEN;

        return link;
}

static int server_messagelink_open_socket(messagelink_t *link,
                                          messagehub_t *hub,
                                          tcp_socket_t socket)
{
        //r_debug("server_messagelink_open_socket");
        link->is_client = 0;
        link->hub = hub;
        link->socket = socket;
        if (link->addr)
                delete_addr(link->addr);
        link->addr = tcp_socket_addr(link->socket);
        return 0;
}

static int server_messagelink_open_websocket(messagelink_t *link)
{
        //r_debug("server_messagelink_open_websocket");
        
        int err = server_messagelink_parse_request(link);
        if (err != 0) {
                r_err("server_messagelink_open_websocket: parsing failed");
                return -1;
        }

        err = server_messagelink_validate_request(link);
        if (err != 0) {
                r_info("server_messagelink_open_websocket: it's not a valid websocket request");
                http_send_error_headers(link->socket, 400);
                return -1;
        }
                        
        if (server_messagelink_upgrade_connection(link) != 0) {
                r_info("server_messagelink_open_websocket: failed to upgrade the websocket");
                return -1;
        }

        
        
        return 0;
}

static int server_messagelink_parse_request(messagelink_t *link)
{
        http_parser *parser;
        http_parser_settings settings;
        size_t len = 1; // TODO
        char buf[len];
        ssize_t received;
        size_t parsed;
        
        http_parser_settings_init(&settings);
        settings.on_header_field = messagelink_on_header_field;
        settings.on_header_value = messagelink_on_header_value;
        settings.on_headers_complete = messagelink_on_headers_complete;
        
        parser = r_new(http_parser);
        if (parser == NULL) {
                r_err("server_messagelink_parse_request: out of memory");
                return -1;
        }
        http_parser_init(parser, HTTP_REQUEST);
        parser->data = link;

        link->parser_header_state = k_header_new;

        link->cont = 1;
        while (link->cont) {
                received = tcp_socket_recv(link->socket, buf, len);
                if (received < 0) {
                        r_err("server_messagelink_parse_request: tcp_socket_recv failed");
                        r_delete(parser);
                        return -1;
                }
        
                /* Start up / continue the parser.
                 * Note we pass received==0 to signal that EOF has been received.
                 */
                parsed = http_parser_execute(parser, &settings, buf, received);
                if (received == 0) {
                        break;
                }
                
                if (parsed != received
                    && parsed != received - 1
                    && link->cont == 1) {
                        /* Handle error. Usually just close the connection. */
                        r_err("server_messagelink_parse_request: parsed != received (%d != %d)", parsed, received);
                        //r_err("data: '%s'", buf);
                        r_delete(parser);
                        return -1;
                }
        }

        r_delete(parser);

        return 0;
}

static int server_messagelink_validate_request(messagelink_t *r)
{
        http_header_t *key = messagelink_get_header(r, "Sec-WebSocket-Key");
        if (key == NULL) return -1;
        
        http_header_t *version = messagelink_get_header(r, "Sec-WebSocket-Version");
        if (version == NULL) return -1;
        if (!rstreq(version->value, "13")) return 0;
        
        http_header_t *upgrade = messagelink_get_header(r, "Upgrade");
        if (upgrade == NULL) return -1;
        if (!rstreq(upgrade->value, "websocket")) return 0;

        http_header_t *connection = messagelink_get_header(r, "Connection");
        if (connection == NULL) return -1;
        if (strstr(connection->value, "Upgrade") == NULL) return 0;
        
        return 0;
}

static int server_messagelink_upgrade_connection(messagelink_t *link)
{
        http_header_t *origin = messagelink_get_header(link, "Origin");
        http_header_t *key = messagelink_get_header(link, "Sec-WebSocket-Key");
        http_header_t *subprotocol = messagelink_get_header(link, "Sec-WebSocket-Protocol");
        http_header_t *extensions = messagelink_get_header(link, "Sec-WebSocket-Extensions");

        unsigned char buffer[100];
        unsigned char digest[21];
        
        snprintf(buffer, 100, "%s%s", key->value, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        buffer[99] = 0;

        SHA1(buffer, digest);
        digest[20] = 0;
        
        char *accept = encode_base64(digest);
        if (accept == NULL) 
                return -1;
                
        membuf_printf(link->out,
                      "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Accept: %s\r\n"
                      "\r\n", accept);
        r_free(accept);

        int err = tcp_socket_send(link->socket, membuf_data(link->out), membuf_len(link->out));
        if (err != 0)
                return -1;

        membuf_clear(link->out);

        return 0;
}

/**********************************************
 * code specific to client-side link
 */

static int client_messagelink_open_socket(messagelink_t *link, addr_t *addr);
static int client_messagelink_open_websocket(messagelink_t *link, const char *host);
static int client_messagelink_send_request(messagelink_t *link, const char *host, const char *key);
static int client_messagelink_parse_response(messagelink_t *link);
static int client_messagelink_validate_response(messagelink_t *link, const char *accept);


static int client_messagelink_open_socket(messagelink_t *link, addr_t *addr)
{
        //r_debug("client_messagelink_open_socket");
        
        link->socket = open_tcp_socket(addr);
        if (link->socket == INVALID_TCP_SOCKET) {
                r_err("client_messagelink_open_socket: failed to connect the socket");
                return -1;
        }
        if (link->addr)
                delete_addr(link->addr);
        link->addr = tcp_socket_addr(link->socket);
        return 0;
}

static int client_messagelink_open_websocket(messagelink_t *link, const char *host)
{
        char *key = _make_key();
        char *accept = _make_accept(key);
        if (key == NULL || accept == NULL)
                goto cleanup;

        //r_debug("client_messagelink_open_websocket");
        
        int err = client_messagelink_send_request(link, host, key);
        if (err != 0)
                goto cleanup;
        
        err = client_messagelink_parse_response(link);
        if (err != 0)
                goto cleanup;
        
        if (client_messagelink_validate_response(link, accept) != 0)
                goto cleanup;

        r_free(key);
        r_free(accept);
        return 0;

cleanup:        
        if (accept) r_free(accept);
        if (key) r_free(key);
        return -1;
}

static int client_messagelink_send_request(messagelink_t *link,
                                           const char *host,
                                           const char *key)
{
        //r_debug("client_messagelink_send_request");

        char header[2048];
        int len = snprintf(header, 2048,
                       "GET / HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "Connection: Upgrade\r\n"
                       "Upgrade: websocket\r\n"
                       "Sec-WebSocket-Version: 13\r\n"
                       "Sec-WebSocket-Key: %s\r\n"
                       "\r\n",
                       host, key);

        return tcp_socket_send(link->socket, header, len);
}

static int client_messagelink_parse_response(messagelink_t *link)
{
        http_parser *parser;
        http_parser_settings settings;
        size_t len = 1; // TODO
        char buf[len];
        ssize_t received;
        size_t parsed;
        
        http_parser_settings_init(&settings);
        //settings.on_status = messagelink_on_status;
        settings.on_header_field = messagelink_on_header_field;
        settings.on_header_value = messagelink_on_header_value;
        settings.on_headers_complete = messagelink_on_headers_complete;
        
        parser = r_new(http_parser);
        if (parser == NULL) {
                r_err("client_messagelink_parse_response: out of memory");
                return -1;
        }
        http_parser_init(parser, HTTP_RESPONSE);
        parser->data = link;

        link->parser_header_state = k_header_new;

        link->cont = 1;
        while (link->cont) {
                received = tcp_socket_recv(link->socket, buf, len);
                if (received < 0) {
                        r_delete(parser);
                        return -1;
                }
        
                /* Start up / continue the parser.
                 * Note we pass received==0 to signal that EOF has been received.
                 */
                parsed = http_parser_execute(parser, &settings, buf, received);
                if (received == 0) {
                        break;
                }
                
                if (parsed != received
                    && parsed != received - 1
                    && link->cont == 1) {
                        /* Handle error. Usually just close the connection. */
                        r_err("client_messagelink_parse_response: parsed != received (%d != %d)", parsed, received);
                        //r_err("data: '%s'", buf);
                        r_delete(parser);
                        return -1;
                }
        }
        
        link->http_status = parser->status_code;
        
        r_delete(parser);

        return 0;
}

static int client_messagelink_validate_response(messagelink_t *link, const char *accept)
{
        http_header_t *h;

        if (link->http_status != 101) {
                r_warn("client_messagelink_validate_response: expected HTTP status 101 but found %d",
                         link->http_status);
                return -1;
        }
        
        h = messagelink_get_header(link, "Connection");
        if (h == NULL) {
                r_warn("client_messagelink_validate_response: missing Connection header");
                return -1;
        }
        if (!rstreq(h->value, "Upgrade")) {
                r_warn("client_messagelink_validate_response: unknown value of Connection header: %s", h->value);
                return -1;
        }

        h = messagelink_get_header(link, "Upgrade");
        if (h == NULL) {
                r_warn("client_messagelink_validate_response: missing Upgrade header");
                return -1;
        }
        if (!rstreq(h->value, "websocket")) {
                r_warn("client_messagelink_validate_response: unknown value of Upgrade header: %s", h->value);
                return -1;
        }
        
        h = messagelink_get_header(link, "Sec-WebSocket-Accept");
        if (h == NULL) {
                r_warn("client_messagelink_validate_response: missing Sec-WebSocket-Accept header");
                return -1;
        }
        if (!rstreq(h->value, accept)) {
                r_warn("client_messagelink_validate_response: Sec-WebSocket-Accept value doesn't match");
                return -1;
        }
        
        return 0;
}


// -1: error
// -2: already connected
int client_messagelink_connect(messagelink_t *link, addr_t *addr)
{
        int err;
        
        link->is_client = 1;

        mutex_lock(link->state_mutex);
        
        if (link->remote_addr != NULL
            && addr_eq(link->remote_addr, addr)
            && link->state == WS_OPEN) {
                // we're good
                mutex_unlock(link->state_mutex);
                return -2;
        }

        // Close the connection if link->addr != addr or if
        // link->addr == addr but the connection is not not established.
        if (link->remote_addr != NULL
            && addr_eq(link->remote_addr, addr)
            && link->state != WS_OPEN) {
                messagelink_close_socket(link);
        }
        if (link->remote_addr != NULL
            && !addr_eq(link->remote_addr, addr)) {
                messagelink_close_socket(link);
        }
        
        err = client_messagelink_open_socket(link, addr);
        if (err != 0) {
                mutex_unlock(link->state_mutex);
                return -1;
        }

        link->state = WS_CLIENT_CONNECTING;

        char b[64];
        err = client_messagelink_open_websocket(link, addr_ip(addr, b, 64));
        if (err != 0) {
                messagelink_close_socket(link);
                mutex_unlock(link->state_mutex);
                return -1;
        }

        link->remote_addr = addr_clone(addr);
        if (link->remote_addr == NULL) {
                messagelink_close_socket(link);
                mutex_unlock(link->state_mutex);
                return -1;
        }
        
        link->state = WS_OPEN;
        
        mutex_unlock(link->state_mutex);

        messagelink_read_in_background(link);
        
        return 0;
}



