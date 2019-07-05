#define _DEFAULT_SOURCE
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include "rcom/log.h"
#include "mem.h"

static int _log_level = LOG_DEBUG;
static FILE* _log_file = NULL;
static char* _log_app = NULL;
static char* _log_path = NULL;

void log_set_app(const char* name)
{
        if (_log_app != NULL) 
                mem_free(_log_app);   
        _log_app = mem_strdup(name);   // FIXME: memory leak...
}

int log_set_file(const char* path)
{
        log_info("Changing log to '%s'", path);
        if (_log_path != NULL) {
                mem_free(_log_path);
                _log_path = NULL;
        }
        if (_log_file != NULL) {
                fclose(_log_file);
                _log_file = NULL;
        }
        
        _log_path = mem_strdup(path);   // FIXME: memory leak...
        if (_log_path == NULL)
                return -1;
        
        if (strcmp(_log_path, "-") == 0) {
                _log_file = stdout;

        } else {
                _log_file = fopen(_log_path, "a");
                if (_log_file == NULL)
                        return -1;
        }
        log_info("Log started ----------------------------------------");
        return 0;
}

const char *log_get_file()
{
        return _log_path;
}

void log_set_filep(FILE* file)
{
        _log_file = file;
}

int log_get_level()
{
        return _log_level;
}

void log_set_level(int level)
{
        _log_level = level;
        if (_log_level < LOG_DEBUG) 
                _log_level = LOG_DEBUG;
        else if (_log_level > LOG_ERROR)
                _log_level = LOG_ERROR;
}

// Not re-entrant!
static const char* get_timestamp(char *buffer, int len)
{
        struct timeval tv;
        struct tm r;
        
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &r);
        snprintf(buffer, len, "%04d-%02d-%02d %02d:%02d:%02d",
                 1900 + r.tm_year, 1 + r.tm_mon, r.tm_mday, 
                 r.tm_hour, r.tm_min, r.tm_sec);
        
        return buffer;
}

static void log_(int level, const char* s)
{
        static char timestamp[256];
        const char* time = get_timestamp(timestamp, 256);
        const char* type = "Unknown";
        const char* name = _log_app? _log_app : "?";
        switch (level) {
        case LOG_DEBUG: type = "DD"; break;
        case LOG_INFO: type = "II"; break;
        case LOG_WARNING: type = "WW"; break;
        case LOG_ERROR: type = "EE"; break;
        case LOG_PANIC: type = "!!"; break;
        }

        fprintf(_log_file, "[%s] [%s] [%s] %s\n", time, type, name, s);
        fflush(_log_file);

        if (level == LOG_PANIC && _log_file != stdout) {
                fprintf(stdout, "[%s] [%s] [%s] %s\n", time, type, name, s);
                fflush(stdout);
        }
}

void log_err(const char* format, ...)
{
        char buffer[1024];
        va_list ap;

        if (_log_level > LOG_ERROR)
                return;
        if (_log_file == NULL)
                _log_file = stderr;

        va_start(ap, format);
        vsnprintf(buffer, 1024, format, ap);
        buffer[1023] = 0;
        va_end(ap);

        log_(LOG_ERROR, buffer);
}

void log_warn(const char* format, ...)
{
        char buffer[1024];
        va_list ap;

        if (_log_level > LOG_WARNING)
                return;
        if (_log_file == NULL)
                _log_file = stderr;

        va_start(ap, format);
        vsnprintf(buffer, 1024, format, ap);
        buffer[1023] = 0;
        va_end(ap);

        log_(LOG_WARNING, buffer);
}

void log_info(const char* format, ...)
{
        char buffer[1024];
        va_list ap;

        if (_log_level > LOG_INFO)
                return;
        if (_log_file == NULL)
                _log_file = stderr;

        va_start(ap, format);
        vsnprintf(buffer, 1024, format, ap);
        buffer[1023] = 0;
        va_end(ap);

        log_(LOG_INFO, buffer);
}

void log_debug(const char* format, ...)
{
        char buffer[1024];
        va_list ap;

        if (_log_level > LOG_DEBUG) 
                return;
        if (_log_file == NULL)
                _log_file = stderr;

        va_start(ap, format);
        vsnprintf(buffer, 1024, format, ap);
        buffer[1023] = 0;
        va_end(ap);

        log_(LOG_DEBUG, buffer);
}

void log_panic(const char* format, ...)
{
        char buffer[1024];
        va_list ap;

        if (_log_file == NULL)
                _log_file = stderr;

        va_start(ap, format);
        vsnprintf(buffer, 1024, format, ap);
        buffer[1023] = 0;
        va_end(ap);

        log_(LOG_PANIC, buffer);
}

