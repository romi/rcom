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
#include <string.h>
#include <r.h>
#include <rcom.h>

circular_buffer_t* new_circular_buffer(int size)
{
        circular_buffer_t* r = r_new(circular_buffer_t);
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

int circular_buffer_data_available(circular_buffer_t* r)
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

// TBD: Check with Peter, why was it -1? Its not a string so is fine without!
int circular_buffer_space_available(circular_buffer_t* r)
{
        int space;
        
        circular_buffer_lock(r);
        if (r->writepos >= r->readpos)
                space = r->length + r->readpos - r->writepos;
        else
                space = r->readpos - r->writepos;
        circular_buffer_unlock(r);
        
        return space;
}

void circular_buffer_write(circular_buffer_t* r, const char* buffer, int len)
{
        int len1, len2 = 0;

    // TBD: We shouldn't write greater than length in a single write. That's writing invalid data. Talk to P.
    // return or not?
        int requested_length = len;
        if (requested_length > r->length)
            return;
        len1 = requested_length;

        circular_buffer_lock(r);
        if (r->writepos + requested_length > r->length) {
                len1 = r->length - r->writepos;
                len2 = requested_length - len1;
                // If we are going to over write the read pointer with new data, then reset the read pointer
                // to the beginning of the current write to preserve the latest write. The issue with this is that it
                // will only work once. Example: Buffer size 8. write 5, write 5. Read will be at 0, next read will read 0->2 bytes.
//                if (len1 >= r->readpos)
//                    r->readpos = r->writepos;
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
        int len1, len2 = 0;

        int requested_length = len;
        if (requested_length > r->length)
            requested_length = r->length;
        len1 = requested_length;

        circular_buffer_lock(r);
        if (r->readpos + requested_length > r->length) {
                len1 = r->length - r->readpos;
                len2 = requested_length - len1;
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
