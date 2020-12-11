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
#ifndef _RCOM_DATA_H_
#define _RCOM_DATA_H_

#include <stdarg.h>
#include <stdint.h>
#include <r.h>

#ifdef __cplusplus
extern "C" {
#endif


// Minimal MTU is 576 bytes
// Safe size: 576 - 60 (IP header) - 8 (udp header) = 508
// 508 - sizeof(seqnum) - sizeof(timestamp) = 496
// https://stackoverflow.com/questions/1098897/what-is-the-largest-safe-udp-packet-size-on-the-internet
//#define DATA_MAXLEN 508

// Often used MTU is 1500 bytes, 1500 - 60 (IP header) - 8 (udp header) = 1432
// 1432 - sizeof(seqnum) - sizeof(timestamp) = 1420
// or 1472?
#define PACKET_MAXLEN 1432
#define PACKET_HEADER 12
#define DATA_MAXLEN 1420

typedef struct _packet_t packet_t;

struct _packet_t {
        unsigned char seqnum[4];
        unsigned char timestamp[8];
        char data[DATA_MAXLEN];
} __attribute__((packed));


typedef struct _data_t data_t;
struct _data_t {
    int len;
    packet_t p;
};

data_t *new_data();
void delete_data(data_t *m);

int data_len(data_t *m);
const char *data_data(data_t *m);
uint32_t data_seqnum(data_t *m);

// Timestamp in seconds since Unix epoch
// TBD: This is unused at present, changed to be symmetric with the set.
//double data_timestamp(data_t *m);
uint64_t data_timestamp(data_t *m);

void data_set_data(data_t *m, const char *s, int len);
void data_set_len(data_t *m, int len);
void data_append_zero(data_t *m);

int data_printf(data_t *m, const char *format, ...);
int data_vprintf(data_t *m, const char *format, va_list ap);
int data_serialise(data_t *m, json_object_t value);

// If parser==NULL, a new parser will be created to parse the data.
json_object_t data_parse(data_t *m, json_parser_t *parser);

packet_t *data_packet(data_t *m);
void data_clear_timestamp(data_t *m);
void data_set_timestamp(data_t *m);
void data_set_seqnum(data_t *m, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_DATA_H_
