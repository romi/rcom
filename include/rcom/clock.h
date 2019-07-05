#ifndef _RCOM_CLOCK_H_
#define _RCOM_CLOCK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Micro-seconds since UNIX epoch
uint64_t clock_timestamp();

// Seconds since UNIX epoch
double clock_time();

// Return date-time as string in the form of "2018-11-14 17:52:38"
char *clock_datetime(char *buf, int len, char sep1, char sep2, char sep3);

void clock_sleep(double seconds);
        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_CLOCK_H_
