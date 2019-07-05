#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "rcom/log.h"

#include "util.h"
#include "mem.h"
#include "sha1.h"

membuf_t *escape_string(const char* s)
{
        membuf_t *t = new_membuf();
        if (t == NULL) return NULL;
        while (*s != '\0') {
                switch (*s) {
                case '\t': membuf_put(t, '\\'); membuf_put(t, 't'); break;
                case '\n': membuf_put(t, '\\'); membuf_put(t, 'n'); break;
                case '\r': membuf_put(t, '\\'); membuf_put(t, 'r'); break;
                case '"': membuf_put(t, '\\'); membuf_put(t, '"'); break;
                case '\\': membuf_put(t, '\\'); membuf_put(t, '\\'); break;
                default: membuf_put(t, *s); break;
                }
                s++;
        }
        membuf_append_zero(t);
        return t;
}

unsigned char *SHA1(const unsigned char* s, unsigned char *digest)
{
        SHA1_CTX context;        
        SHA1Init(&context);
        SHA1Update(&context, s, strlen(s));
        SHA1Final(digest, &context);
        return digest;
}

char *encode_base64(const unsigned char *s)
{
        static const char table[] = {
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                '4', '5', '6', '7', '8', '9', '+', '/'
        };

        int ilen = strlen(s);
        int olen = 4 * ((ilen + 2) / 3);

        char *t = mem_alloc(olen+1);
        if (t == NULL) {
                log_err("encode_base64: out of memory");
                return NULL;
        }
        t[olen] = 0;

        unsigned char a, b, c;
        unsigned int p;
        
        for (int i = 0, j = 0; i < ilen;) {
                int n = ilen - i;
                if (n > 3) n = 3;

                switch (n) {
                case 3:
                        a = (i < ilen)? s[i++] : 0;
                        b = (i < ilen)? s[i++] : 0;
                        c = (i < ilen)? s[i++] : 0;
                        p = (a << 0x10) + (b << 0x08) + c;
                        t[j++] = table[(p >> 18) & 0x3F];
                        t[j++] = table[(p >> 12) & 0x3F];
                        t[j++] = table[(p >> 6) & 0x3F];
                        t[j++] = table[p & 0x3F];
                        break;
                case 2:
                        a = (i < ilen)? s[i++] : 0;
                        b = (i < ilen)? s[i++] : 0;
                        p = (a << 0x10) + (b << 0x08);
                        t[j++] = table[(p >> 18) & 0x3F];
                        t[j++] = table[(p >> 12) & 0x3F];
                        t[j++] = table[(p >> 6) & 0x3F];
                        t[j++] = '=';
                        break;
                case 1:
                        a = (i < ilen)? s[i++] : 0;
                        p = (a << 0x10);
                        t[j++] = table[(p >> 18) & 0x3F];
                        t[j++] = table[(p >> 12) & 0x3F];
                        t[j++] = '=';
                        t[j++] = '=';
                        break;
                }
        }
                
        return t;
}

int rcom_getrandom(void *buf, size_t buflen, unsigned int flags)
{
    return (int)syscall(SYS_getrandom, buf, buflen, flags);
}

void generate_random_buffer(uint8_t *buffer, ssize_t len)
{
        ssize_t n = 0;
        while (n < len) {
                ssize_t m = rcom_getrandom((void *) &buffer[n], len - n, 0);
                n += m;
        }
}
