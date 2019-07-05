#ifndef _RCOM_MEMBUF_H_
#define _RCOM_MEMBUF_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _membuf_t membuf_t;

membuf_t *new_membuf();
void delete_membuf(membuf_t *b);

int membuf_put(membuf_t *b, char c);
int membuf_append(membuf_t *b, const char *data, int len);
int membuf_append_zero(membuf_t *b);
int membuf_printf(membuf_t *b, const char* format, ...);
int membuf_vprintf(membuf_t *b, const char* format, va_list ap);

int membuf_available(membuf_t *b);
int membuf_assure(membuf_t *b, int size);
void membuf_clear(membuf_t *b);

char* membuf_data(membuf_t *b);
int membuf_len(membuf_t *b);

void membuf_lock(membuf_t *b);
void membuf_unlock(membuf_t *b);

// Internal use only!
void membuf_set_len(membuf_t *b, int len);
int membuf_size(membuf_t *b);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MEMBUF_H_
