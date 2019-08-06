#include <stdio.h>

#include "rcom/log.h"

#include "util_priv.h"
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

void generate_random_buffer(uint8_t *buffer, ssize_t len)
{
        ssize_t n = 0;
        while (n < len) {
                ssize_t m = rcom_getrandom((void *) &buffer[n], len - n, 0);
                n += m;
        }
}

char *rprintf(char *buffer, int buflen, const char *format, ...)
{
        int len;
        va_list ap;
        int ret;
        
        va_start(ap, format);
        len = vsnprintf(NULL, 0, format, ap);
        va_end(ap);

        if (len < 0 || buflen < len+1)
                return NULL;
        
        va_start(ap, format);
        len = vsnprintf(buffer, buflen, format, ap);
        va_end(ap);

        if (len < 0)
                return NULL;
        
        return buffer;
}

int urlencode(const unsigned char* s, membuf_t *buf)
{
        membuf_clear(buf);
        for (; *s; s++) {
                unsigned char c = *s;
                if (c < 32 || c >= 127) {
                        log_err("urlencode: unhandled character. Sorry.");
                        membuf_append_zero(buf);
                        return -1;
                        
                } else if (('0' <= c && c <= '9')
                    || ('a' <= c && c <= 'z')
                    || ('A' <= c && c <= 'Z')
                    || c == '-' || c == '_' || c == '.' || c == '~') {
                        membuf_append(buf, (char*) &c, 1);
                        
                } else {
                        char hex[4];
                        rprintf(hex, 4, "%%%02X", c);
                        membuf_append(buf, hex, 3);
                }
        }
        membuf_append_zero(buf);
        return 0;
}

int hex2c(char c)
{
        if ('0' <= c && c <= '9')
                return c - '0';
        else if ('a' <= c && c <= 'f')
                return 10 + c - 'a';
        else if ('A' <= c && c <= 'F')
                return 10 + c - 'A';
        return -1;
}

int urldecode(const char* s, int len, membuf_t *buf)
{
        int i = 0;
        
        membuf_clear(buf);
        while (i < len) {
                char c = s[i];
                if (c == '%') {
                        if (i + 2 >= len) {
                                log_err("urldecode: unexpected end.");
                                return -1;
                        }
                        int a = hex2c(s[i+1]);
                        if (a == -1) {
                                log_err("urldecode: unexpected character ('%%%c').",
                                        s[i+1]);
                                return -1;
                        }
                        int b = hex2c(s[i+2]);
                        if (b == -1) {
                                log_err("urldecode: unexpected character ('%%%c%c').",
                                        s[i+1], s[i+2]);
                                return -1;
                        }
                        int d = 16 * a + b;
                        c = d & 0xff;
                        i += 2;
                }
                membuf_append(buf, &c, 1);
                i++;
        }
        membuf_append_zero(buf);
        return 0;        
}
