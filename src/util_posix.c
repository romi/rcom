#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <uuid/uuid.h>

#include "rcom/log.h"

#include "util_priv.h"
#include "mem.h"

int rcom_getrandom(void *buf, size_t buflen, unsigned int flags)
{
    return (int)syscall(SYS_getrandom, buf, buflen, flags);
}

char *generate_uuid()
{
        uuid_t uuid;
        char s[37];
        uuid_generate(uuid);
        uuid_unparse_lower(uuid, s);
        return mem_strdup(s);
}
