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
#include <string.h>

#include <r.h>
#include <rcom.h>

typedef struct _circular_buffer_t
{
        unsigned char* buffer;
        int length;
        int readpos;
        int writepos;
        mutex_t *mutex;
} circular_buffer_t;

circular_buffer_t* new_circular_buffer(int size)
{
        circular_buffer_t* r = r_new(circular_buffer_t);
        if (r == NULL)
                return NULL;

        r->buffer = r_alloc(size);
        if (r->buffer == NULL) {
                r_delete(r);
                return NULL;
        }
        r->length = size;
        r->readpos = 0;
        r->writepos = 0;
        r->mutex = new_mutex();

        return r;        
}

void delete_circular_buffer(circular_buffer_t* r)
{
        if (r) {
                if (r->buffer)
                        r_free(r->buffer);
                if (r->mutex)
                        delete_mutex(r->mutex);
                r_delete(r);
        }
}

static void circular_buffer_lock(circular_buffer_t* r)
{
        mutex_lock(r->mutex);
}

static void circular_buffer_unlock(circular_buffer_t* r)
{
        mutex_unlock(r->mutex);
}

int circular_buffer_size(circular_buffer_t* r)
{
        return r->length;
}

int circular_buffer_available(circular_buffer_t* r)
{
        int available;
        
        circular_buffer_lock(r);
        if (r->writepos >= r->readpos)
                available = r->writepos - r->readpos;
        else 
                available = r->length + r->writepos - r->readpos;
        circular_buffer_unlock(r);

        return available;
}

int circular_buffer_space(circular_buffer_t* r)
{
        int space;
        
        circular_buffer_lock(r);
        if (r->writepos >= r->readpos)
                space = r->length + r->readpos - r->writepos - 1;
        else
                space = r->readpos - r->writepos - 1;
        circular_buffer_unlock(r);
        
        return space;
}

void circular_buffer_write(circular_buffer_t* r, const char* buffer, int len)
{
        int len1, len2;

        len1 = len;
        len2 = 0;

        circular_buffer_lock(r);
        if (r->writepos + len > r->length) {
                len1 = r->length - r->writepos;
                len2 = len - len1;
        }
        if (len1) {
                memcpy(r->buffer + r->writepos, buffer, len1);
                r->writepos += len1;
        }
        if (len2) {
                memcpy(r->buffer, buffer + len1, len2);
                r->writepos = len2;
        }
        circular_buffer_unlock(r);
}

void circular_buffer_read(circular_buffer_t* r, char* buffer, int len)
{
        int len1, len2;

        len1 = len;
        len2 = 0;
        
        circular_buffer_lock(r);
        if (r->readpos + len > r->length) {
                len1 = r->length - r->readpos;
                len2 = len - len1;
        }
        if (len1) {
                memcpy(buffer, r->buffer + r->readpos, len1);
                r->readpos += len1;
        }
        if (len2) {
                memcpy(buffer + len1, r->buffer, len2);
                r->readpos = len2;
        }
        circular_buffer_unlock(r);
}
