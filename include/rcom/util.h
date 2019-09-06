#ifndef _RCOM_UTIL_H
#define _RCOM_UTIL_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define rstreq(_s1, _s2) ((_s1) != NULL && (_s2) != NULL && strcmp(_s1,_s2)==0)
//char *rprintf(char *buffer, int len, const char *format, ...);

const char *filename_to_mimetype(const char *filename);
        
#ifdef __cplusplus
}
#endif

#endif // _RCOM_UTIL_H

