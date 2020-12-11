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
#ifndef _RCOM_MULTIPART_PARSER_H_
#define _RCOM_MULTIPART_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*multipart_onheaders_t)(void *userdata,
                                    int len,
                                    const char *mimetype,
                                    double timestamp,
                                    uint32_t offset);

typedef int (*multipart_onpart_t)(void *userdata,
                                  const unsigned char *data, int len,
                                  const char *mimetype,
                                  double timestamp);

typedef struct _multipart_parser_t multipart_parser_t;

multipart_parser_t *new_multipart_parser(void* userdata,
                                         multipart_onheaders_t onheaders,
                                         multipart_onpart_t onpart);
void delete_multipart_parser(multipart_parser_t *m);

// Parse a buffer of data
int multipart_parser_process(multipart_parser_t *m, response_t *response,
                             const char *data, int len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MULTIPART_PARSER_H_
