#ifndef _RCOM_UTIL_H_
#define _RCOM_UTIL_H_

#include <stdint.h>
#include <r.h>
#include "rcom/util.h"

#ifdef __cplusplus
extern "C" {
#endif

membuf_t *escape_string(const char* s);

char *encode_base64(const unsigned char *s);
        
unsigned char *SHA1(const unsigned char* s, unsigned char *digest);

int urlencode(const unsigned char* s, membuf_t *buf);
int urldecode(const char* s, int len, membuf_t *buf);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_UTIL_H_
