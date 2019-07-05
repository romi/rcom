#include <stdlib.h>

#include "rcom/registry.h"
#include "rcom/log.h"
#include "rcom/dump.h"
#include "rcom/multipart_parser.h"

#include "mem.h"
#include "membuf.h"
#include "util.h"

enum {
        k_read_header,
        k_header_complete,
        k_read_body,
        k_body_complete,
        k_error
};
        
struct _multipart_parser_t {
        int status;
        membuf_t *body;
        membuf_t *header;
        char mimetype[128];
        double timestamp;
        int length;
        void *userdata;
        multipart_onheaders_t onheaders;
        multipart_onpart_t onpart;
        uint32_t filepos;
};

static int multipart_parser_callback(void *data, const char *buf, int len);
static int multipart_parser_parse_header(multipart_parser_t *m);

multipart_parser_t *new_multipart_parser(void* userdata,
                                         multipart_onheaders_t onheaders,
                                         multipart_onpart_t onpart)
{
        multipart_parser_t *m = NULL;
        
        m = new_obj(multipart_parser_t);
        if (m == NULL) {
                return NULL;
        }

        m->body = new_membuf();
        if (m->body == NULL) {
                delete_multipart_parser(m);
                return NULL;
        }

        m->header = new_membuf();
        if (m->header == NULL) {
                delete_multipart_parser(m);
                return NULL;
        }

        m->status = k_read_header;
        m->userdata = userdata;
        m->onheaders = onheaders;
        m->onpart = onpart;
        return m;
}

void delete_multipart_parser(multipart_parser_t *m)
{
        if (m) {
                if (m->header) delete_membuf(m->header);
                if (m->body) delete_membuf(m->body);
                delete_obj(m);
        }
}

static int multipart_parser_parse_header(multipart_parser_t *m)
{
        static char _eol[2];
#define end_of_line() ((_eol[0] == '\r') && (_eol[1] == '\n'))
        
        char* s = membuf_data(m->header);
        int len = membuf_len(m->header);
        int start = 0;
        int end;
        
        memset(_eol, 0, 2);
        
        if (strncmp(s, "--nextimage", 11) != 0) {
                log_err("Invalid chunk separator");
                return -1;
        }
        
        for (end = 0; end < len; end++) {
                char c = s[end];
                _eol[0] = _eol[1];
                _eol[1] = c;
                if (end_of_line()) {
                        s[end-1] = 0;
                        s[end] = 0;
                        if (streq(s+start, "--nextimage")) {
                                // Do nothing
                                
                        } else if (strncmp(s+start, "Content-Type: ", 14) == 0) {
                                int n = end - start - 14;
                                if (n < 128) {
                                        strcpy(m->mimetype, s+start+14);
                                } else {
                                        log_err("Invalid mime-type header");
                                        m->status = k_error;
                                        return -1;
                                }
                                
                        } else if (strncmp(s+start, "Content-Length: ", 16) == 0) {
                                char *endptr;
                                m->length = strtol(s+start+16, &endptr, 10);
                                if (m->length == 0) {
                                        log_err("Invalid length header");
                                        m->status = k_error;
                                        return -1;
                                }
                                
                        } else if (strncmp(s+start, "X-LT-Timestamp: : ", 16) == 0) {
                                char *endptr;
                                m->timestamp = strtod(s+start+16, &endptr);
                                if (m->timestamp == 0) {
                                        log_err("Invalid timestamp");
                                        m->status = k_error;
                                        return -1;
                                }
                        } else {
                                // Do nothing
                        }
                        start = end+1;
                        end = end+1;
                }
        }
        return 0;
}

static int multipart_parser_read_header(multipart_parser_t *m, const char *buf, int len, int offset)
{
        static char _eoh[4];
#define end_of_header() ((_eoh[0] == '\r')    \
                         && (_eoh[1] == '\n')  \
                         && (_eoh[2] == '\r')  \
                         && (_eoh[3] == '\n'))
        
        while (offset < len) {
                char c = buf[offset++];
                m->filepos++;
                membuf_append(m->header, &c, 1);
                _eoh[0] = _eoh[1];
                _eoh[1] = _eoh[2];
                _eoh[2] = _eoh[3];
                _eoh[3] = c;

                if (end_of_header()) {
                        int err = multipart_parser_parse_header(m);
                        if (err != 0)
                                m->status = k_error;
                        else {
                                m->status = k_header_complete;
                                memset(_eoh, 0, 4);
                                membuf_clear(m->header);
                        }
                        break;
                }
        }
        
        return offset;
}

static int multipart_parser_read_image(multipart_parser_t *m, const char *buf, int len, int offset)
{
        int read = membuf_len(m->body); 
        int needed = m->length - read;
        int provided = len - offset;
        
        if (needed > provided) {
                membuf_append(m->body, buf + offset, provided);
                m->filepos += provided;
                return len;
        } else {
                membuf_append(m->body, buf + offset, needed);
                m->status = k_body_complete;
                m->filepos += needed;
                return offset + needed;
        }
}

static int multipart_parser_callback(void *data, const char *buf, int len)
{
        multipart_parser_t *m = (multipart_parser_t*) data;
        return multipart_parser_process(m, buf, len);
}

int multipart_parser_process(multipart_parser_t *m, const char *buf, int len)
{
        int offset = 0;
        int err = 0;
        
        while (offset < len) {
                switch (m->status) {
                case k_read_header:
                        offset = multipart_parser_read_header(m, buf, len, offset);
                        break;
                case k_header_complete:
                        if (m->onheaders)
                                err = m->onheaders(m->userdata,
                                                   m->length,
                                                   m->mimetype,
                                                   m->timestamp,
                                                   m->filepos);
                        m->status = k_read_body;
                        membuf_clear(m->header);
                        break;
                        
                case k_read_body:
                        offset = multipart_parser_read_image(m, buf, len, offset);
                        break;
                case k_body_complete:
                        if (m->onpart)
                                err = m->onpart(m->userdata,
                                                (unsigned char*) membuf_data(m->body),
                                                membuf_len(m->body),
                                                m->mimetype,
                                                m->timestamp);
                        m->status = k_read_header;
                        membuf_clear(m->body);
                        break;
                }
        }
        return err;
}


