#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include "rcom/clock.h"

uint64_t clock_timestamp()
{
        uint64_t t;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        t = tv.tv_sec * 1000000 + tv.tv_usec;
        return t;
}

double clock_time()
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (double) tv.tv_sec + (double) tv.tv_usec / 1000000.0;
}

char *clock_datetime(char *buf, int len, char sep1, char sep2, char sep3)
{
        struct timeval tv;
        struct tm r;
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &r);
        snprintf(buf, len, "%04d%c%02d%c%02d%c%02d%c%02d%c%02d",
                 1900 + r.tm_year, sep1, 1 + r.tm_mon, sep1, r.tm_mday,
                 sep2,
                 r.tm_hour, sep3, r.tm_min, sep3, r.tm_sec);
        return buf;
}

void clock_sleep(double seconds)
{
        useconds_t usec = (useconds_t) (1000000.0 * seconds);
        usleep(usec);
}
