/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <r.h>

#include "data.h"

data_t* new_data()
{
        data_t* m = r_new(data_t);
        m->len = 0;
        m->p.data[0] = 0;
        return m;
}

void delete_data(data_t* m)
{
        if (m) r_delete(m);
}

int data_len(data_t* m)
{
        if (m == NULL)
            return 0;
        return m->len;
}

const char* data_data(data_t* m)
{       if (m == NULL)
            return NULL;
        return m->p.data;
}

void data_set_data(data_t* m, const char* s, int len)
{
        if (m != NULL && s != NULL)
        {
            if (len > DATA_MAXLEN) {
                r_warn("Data: data truncated: max length=%d!", DATA_MAXLEN);
                len = DATA_MAXLEN;
            }
            memcpy(m->p.data, s, len);
            m->len = len;
        }
        else
            r_err("set_data: NULL pointer parameter.");

}

void data_set_len(data_t* m, int len)
{
        if (len < 0)
            len = 0;
        if (len > DATA_MAXLEN)
                len = DATA_MAXLEN;
        m->len = len;
}

void data_append_zero(data_t* m)
{
        if (m->len <= DATA_MAXLEN-1) {
                m->p.data[m->len++] = 0;
        } else {
                m->p.data[DATA_MAXLEN-1] = 0;
        }
                
}

int data_printf(data_t* m, const char* format, ...)
{
        int ret;
        va_list ap;
        va_start(ap, format);
        ret = data_vprintf(m, format, ap);
        va_end(ap);
        return ret;
}

int data_vprintf(data_t* m, const char* format, va_list ap)
{
        int len;
        
        len = vsnprintf(m->p.data, DATA_MAXLEN, format, ap);
        if (len >= DATA_MAXLEN) {
                r_warn("data_printf: data has been truncated");
                m->len = DATA_MAXLEN-1;
                return -1;
        }
        m->len = len;
        return 0;

}

int data_serialise(data_t* m, json_object_t obj)
{
        int ret;
        ret = json_tostring(obj, k_json_compact, m->p.data, DATA_MAXLEN);
        if (ret != 0) {
                r_err("datasource_send: json_tostring failed");
                return -1;
        }
        m->len = strlen(m->p.data);
        return 0;
}

json_object_t data_parse(data_t* m, json_parser_t* parser)
{
        int destroy_parser = 0;
        json_object_t obj;
        
        if (parser == NULL) {
                destroy_parser = 1;
                parser = json_parser_create();
        }

        data_append_zero(m);
        obj = json_parser_eval(parser, m->p.data);
        
        if (destroy_parser) {
                json_parser_destroy(parser);
        }

        return obj;
}

packet_t *data_packet(data_t* m)
{
        return &m->p;
}

void data_clear_timestamp(data_t* m)
{
        memset(&m->p.timestamp, 0, 8);;
}

//double data_timestamp(data_t* m)
//{
//    unsigned char *p = m->p.timestamp;
//    uint64_t t = (((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48)
//                  | ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32)
//                  | ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16)
//                  | ((uint64_t)p[6] << 8) | (uint64_t)p[7]);
//    return (double) t / 1000000.0;
//}

uint64_t data_timestamp(data_t* m)
{
        unsigned char *p = m->p.timestamp;
        uint64_t t = (((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48)
                      | ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32)
                      | ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16)
                      | ((uint64_t)p[6] << 8) | (uint64_t)p[7]);
        return t;
}

void data_set_timestamp(data_t* m)
{
        uint64_t t = clock_timestamp();
        unsigned char *p = m->p.timestamp;
        p[0] = (unsigned char) ((t & 0xff00000000000000) >> 56);
        p[1] = (unsigned char) ((t & 0x00ff000000000000) >> 48);
        p[2] = (unsigned char) ((t & 0x0000ff0000000000) >> 40);
        p[3] = (unsigned char) ((t & 0x000000ff00000000) >> 32);
        p[4] = (unsigned char) ((t & 0x00000000ff000000) >> 24);
        p[5] = (unsigned char) ((t & 0x0000000000ff0000) >> 16);
        p[6] = (unsigned char) ((t & 0x000000000000ff00) >> 8);
        p[7] = (unsigned char) (t & 0x00000000000000ff);
}

uint32_t data_seqnum(data_t* m)
{
        unsigned char *p = m->p.seqnum;
        uint32_t n = (((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
                      | ((uint32_t)p[2] << 8) | (uint32_t)p[3]);
        return n;
}

void data_set_seqnum(data_t* m, uint32_t n)
{
        unsigned char *p = m->p.seqnum;
        p[0] = (unsigned char) ((n & 0xff000000) >> 24);
        p[1] = (unsigned char) ((n & 0x00ff0000) >> 16);
        p[2] = (unsigned char) ((n & 0x0000ff00) >> 8);
        p[3] = (unsigned char) (n & 0x000000ff);
}
