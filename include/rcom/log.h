#include <stdio.h>

#ifndef _RCOM_LOG_H
#define _RCOM_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_DEBUG 0
#define LOG_INFO 1
/* App can continue: */
#define LOG_WARNING 2
/* App cannot continue: */
#define LOG_ERROR 3
/* Call the developer on her phone */
#define LOG_PANIC 4

int log_get_level();
void log_set_level(int level);
int log_set_file(const char* path);
void log_set_filep(FILE* file);
void log_set_app(const char* name);
const char *log_get_file();

void log_err(const char* format, ...);
void log_warn(const char* format, ...);
void log_info(const char* format, ...);
void log_debug(const char* format, ...);
void log_panic(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_LOG_H
