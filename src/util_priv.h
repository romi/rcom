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
#ifndef _RCOM_UTIL_H_
#define _RCOM_UTIL_H_

#include <stdint.h>
#include <r.h>
#include "rcom/util.h"

#ifdef __cplusplus
extern "C" {
#endif

membuf_t *escape_string(const char* s);

char *encode_base64(const unsigned char *s, int len);
        
unsigned char *SHA1(const unsigned char* s, unsigned char *digest);

int urlencode(const unsigned char* s, membuf_t *buf);
int urldecode(const char* s, int len, membuf_t *buf);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_UTIL_H_
