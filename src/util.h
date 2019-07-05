#ifndef _RCOM_UTIL_H_
#define _RCOM_UTIL_H_

#include <string.h>
#include <netinet/in.h>
#include <stdint.h>
#include "membuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define streq(_s1, _s2) (strcmp(_s1,_s2)==0)

membuf_t *escape_string(const char* s);

char *encode_base64(const unsigned char *s);

void generate_random_buffer(uint8_t *buffer, ssize_t len);

unsigned char *SHA1(const unsigned char* s, unsigned char *digest);
        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_UTIL_H_
