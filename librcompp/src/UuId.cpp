#include <sys/syscall.h>
#include <unistd.h>
#include "StringUtils.h"

namespace UuId
{
    int r_random(void *buf, size_t buflen)
    {
        int flags = 0;
        return (int) syscall(SYS_getrandom, buf, buflen, flags);
    }

    std::string create()
    {
        uint8_t b[16];
        r_random(&b, sizeof(b));

        // RFC 4122 section 4.4
        b[6] = 0x40 | (b[6] & 0x0f);
        b[8] = 0x80 | (b[8] & 0x3f);
        return StringUtils::string_format("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                          b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7],
                                          b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);

    }

}