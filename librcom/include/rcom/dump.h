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
#ifndef _RCOM_DUMP_H_
#define _RCOM_DUMP_H_

#include <stdio.h>
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _dump_t dump_t;
        
void set_dumping(int value);
int get_dumping();
void set_dumping_dir(const char *path);
const char *get_dumping_dir();

int do_replay();
void set_replay_id(const char *id);
const char *get_replay_id();

dump_t *dump_create(const char *name,
                    int type,
                    const char *topic,
                    const char *mimetype);

dump_t *dump_open(const char *path);

int dump_type(dump_t *dump);

void delete_dump(dump_t *dump);
int dump_write_data(dump_t *dump, data_t *data);
int dump_read_data(dump_t *dump, data_t *data);

int dump_write_buffer(dump_t *dump, char *data, uint32_t len);
int dump_read_buffer(dump_t *dump, char *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_DUMP_H_
