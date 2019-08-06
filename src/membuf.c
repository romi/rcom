
#include "rcom/log.h"
#include "rcom/membuf.h"
#include "rcom/thread.h"

#include "mem.h"

struct _membuf_t
{
        char* buffer;
        int length;
        int index;
        mutex_t *mutex;
};

static int membuf_grow(membuf_t *b)
{
        int len = 2 * b->length;
        if (len == 0)
                len = 128;
        b->buffer = mem_realloc(b->buffer, len);
        if (b->buffer == NULL) {
                log_err("membuf: out of memory: length=%d", len);
                return -1;
        }
        b->length = len;
        return 0;
}

membuf_t *new_membuf()
{
        membuf_t* membuf = new_obj(membuf_t);
        if (membuf == NULL) {
                log_err("membuf: out of memory");
                return NULL;
        }
        membuf->mutex = new_mutex();
        return membuf;
}

void delete_membuf(membuf_t *b)
{
        if (b) {
                if (b->mutex)
                        delete_mutex(b->mutex);
                if (b->buffer) mem_free(b->buffer);
                delete_obj(b);
        }
}

int membuf_put(membuf_t *b, char c)
{
        int ret;
        if (b->index + 1 > b->length) {
                ret = membuf_grow(b);
                if (ret != 0)
                        return -1;
        }
        b->buffer[b->index++] = c;
        return 0;
}

void membuf_lock(membuf_t *b)
{
        mutex_lock(b->mutex);
}

void membuf_unlock(membuf_t *b)
{
        mutex_unlock(b->mutex);
}

int membuf_append(membuf_t *b, const char *data, int len)
{
        int ret;
        while (b->index + len > b->length) {
                ret = membuf_grow(b);
                if (ret != 0)
                        return -1;
        }
        memcpy(b->buffer + b->index, data, len);
        b->index += len;
        return 0;
}

int membuf_append_zero(membuf_t *b)
{
        return membuf_put(b, 0);
}

void membuf_clear(membuf_t *b)
{
        b->index = 0;
}

char* membuf_data(membuf_t *b)
{
        return b->buffer;
}

int membuf_len(membuf_t *b)
{
        return b->index;
}

void membuf_set_len(membuf_t *b, int len)
{
        b->index = len;
}

int membuf_available(membuf_t *b)
{
        return b->length - b->index;
}

int membuf_size(membuf_t *b)
{
        return b->length;
}

int membuf_assure(membuf_t *b, int size)
{
        int r;
        while (b->index + size > b->length) {
                r = membuf_grow(b);
                if (r != 0) return -1;
        }
        return 0;
}

int membuf_printf(membuf_t *b, const char* format, ...)
{
        int len;
        va_list ap;
        int ret;
        
        va_start(ap, format);
        len = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        if (len < 0)
                return -1;

        if (membuf_assure(b, len+1) != 0)
                return -1;
        
        va_start(ap, format);
        ret = membuf_vprintf(b, format, ap);
        va_end(ap);

        return ret;        
}

int membuf_vprintf(membuf_t* m, const char* format, va_list ap)
{
        int len;
        int available = m->length - m->index;
        len = vsnprintf(m->buffer + m->index, available, format, ap);
        if (len < available) {
                m->index += len;
                return 0;
        } else {
                return -1;
        }
}

/* int membuf_vprintf(membuf_t* m, const char* format, va_list ap) */
/* { */
/*         int len; */

/*         va_start(ap, format); */
/*         len = vsnprintf(NULL, 0, format, ap); */
/*         va_end(ap); */

/*         if (len < 0) */
/*                 return -1; */

/*         if (membuf_assure(req, m->index + len + 1) != 0) */
/*                 return -1; */
        
/*         va_start(ap, format); */
/*         len = vsnprintf(m->buffer + m->index, len, format, ap); */
/*         va_end(ap); */
        
/*         if (len < 0) */
/*                 return -1; */

/*         return 0; */
/* } */
