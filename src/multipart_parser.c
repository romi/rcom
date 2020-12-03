/*
  rcom

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcom is light-weight libary for inter-node communication.

  rcom is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.

 */
#include <stdlib.h>
#include <r.h>

#include "registry.h"
#include "dump.h"
#include "multipart_parser.h"
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

static int multipart_parser_parse_header(multipart_parser_t *m, response_t *r);

multipart_parser_t *new_multipart_parser(void* userdata,
                                         multipart_onheaders_t onheaders,
                                         multipart_onpart_t onpart)
{
        multipart_parser_t *m = NULL;
        
        m = r_new(multipart_parser_t);
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
                r_delete(m);
        }
}

static int multipart_parser_parse_header(multipart_parser_t *m, response_t *r)
{
        static char _eol[2];
#define end_of_line() ((_eol[0] == '\r') && (_eol[1] == '\n'))
        
        char* s = membuf_data(m->header);
        int len = membuf_len(m->header);
        int start = 0;
        int end;
        
        if (response_dumpfile(r)) {
                fprintf(response_dumpfile(r), "#[MULPARS]\n[[%.*s]]\n", len, s);
                fflush(response_dumpfile(r));
        }
        
        memset(_eol, 0, 2);
        
        if (strncmp(s, "--nextimage", 11) != 0) {
                r_err("Invalid chunk separator");
                r_debug("   Got: '%s'", s);
                return -1;
        }
        
        for (end = 0; end < len; end++) {
                char c = s[end];
                _eol[0] = _eol[1];
                _eol[1] = c;
                if (end_of_line()) {
                        s[end-1] = 0;
                        s[end] = 0;
                        if (rstreq(s+start, "--nextimage")) {
                                // Do nothing
                                
                        } else if (strncmp(s+start, "Content-Type: ", 14) == 0) {
                                int n = end - start - 14;
                                if (n < 128) {
                                        strcpy(m->mimetype, s+start+14);
                                } else {
                                        r_err("Invalid mime-type header: %.*s", len, s);
                                        m->status = k_error;
                                        return -1;
                                }
                                
                        } else if (strncmp(s+start, "Content-Length: ", 16) == 0) {
                                char *endptr;
                                m->length = strtol(s+start+16, &endptr, 10);
                                if (m->length == 0) {
                                        r_err("Invalid length header: %.*s", 10, s+start+16);
                                        m->status = k_error;
                                        return -1;
                                }
                                
                        } else if (strncmp(s+start, "X-LT-Timestamp: ", 16) == 0) {
                                char *endptr;
                                m->timestamp = strtod(s+start+16, &endptr);
                                if (m->timestamp == 0) {
                                        r_err("Invalid timestamp: %.*s", len, s);
                                        m->status = k_error;
                                        return -1;
                                }
                                //r_debug("parser: timestamp %f", m->timestamp);
                        } else {
                                // Do nothing
                        }
                        start = end+1;
                        end = end+1;
                }
        }
        return 0;
}

/*
 * This function appends data to the m->header buffer until the end of
 * the header section is reached (\r\n\r\n). At that point, it calls
 * multipart_parser_parse_header() to parse the m->header buffer.
 */
static int multipart_parser_append_header(multipart_parser_t *m, response_t *response,
                                        const char *buf, int len, int offset)
{
        if (response_dumpfile(response)) {
                fprintf(response_dumpfile(response), "#[MULADD]\n[[%.*s]]\n",
                        len - offset, buf + offset);
                fflush(response_dumpfile(response));
        }
        //r_debug("multipart_parser_append_header");
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
                        int err = multipart_parser_parse_header(m, response);
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

static int multipart_parser_append_part(multipart_parser_t *m,
                                      const char *buf, int len,
                                      int offset)
{
        //r_debug("multipart_parser_append_part");
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

int multipart_parser_process(multipart_parser_t *m, response_t *response,
                             const char *buf, int len)
{
        int offset = 0;
        int err = 0;

        //r_debug("multipart_parser_process: %.*s", len, buf);
        
        while (offset < len) {
                switch (m->status) {
                case k_read_header:
                        offset = multipart_parser_append_header(m, response,
                                                                buf, len, offset);
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
                        offset = multipart_parser_append_part(m, buf, len, offset);
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
                case k_error:
                        err = -1;
                }
        }
        return err;
}


