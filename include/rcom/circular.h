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
#ifndef _RCOM_CIRCULAR_H_
#define _RCOM_CIRCULAR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _circular_buffer_t
{
    unsigned char* buffer;
    int length;
    int readpos;
    int writepos;
    mutex_t *mutex;
} circular_buffer_t;

typedef struct _circular_buffer_t circular_buffer_t;

circular_buffer_t* new_circular_buffer(int size);
void delete_circular_buffer(circular_buffer_t* r);

/*
 * The size of the buffer.
 */
int circular_buffer_size(circular_buffer_t* r);

/*
 * The amount of data available for reading.
 */
int circular_buffer_data_available(circular_buffer_t* r);

/*
 * The amount of space available for writing.
 */
int circular_buffer_space_available(circular_buffer_t* r);

void circular_buffer_write(circular_buffer_t* r, const char* buffer, int len);
void circular_buffer_read(circular_buffer_t* r, char* buffer, int len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_CIRCULAR_H_

