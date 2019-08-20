#include <stdlib.h>
#include "rcom/log.h"
#include "mem.h"

//#define MEM_DIAGNOSTICS 0
#define MEM_DIAGNOSTICS 1

#if MEM_DIAGNOSTICS
#define SGC_USE_THREADS 1
#include <sgc.h>

int mem_init(int *argc)
{
	if (sgc_init(argc, 0) != 0) {
                log_err("Failed the initialise the SGC memory heap");
                return -1;
        }
        return 0;
}

static int print_memory_leak(int op, void* ptr,
                             unsigned char type,
                             int counter, int size)
{
        if (op == 3) {
                printf("Memory leak: counter=%d, size=%d\n", counter, size);
        } 
        return 1;
}

void mem_cleanup()
{
        log_info("Scanning for memory leaks");
        sgc_search_memory_leaks(print_memory_leak);
        sgc_cleanup();
}

void *safe_malloc(size_t size, int zero)
{
        if (size == 0) {
                log_warn("safe_malloc: size == 0");
                return NULL;
        }
        void *ptr = sgc_new_object(size, SGC_ZERO, 0);
        if (ptr == NULL) {
                // Return NULL. Maybe the app can handle it correctly.
                log_err("safe_malloc: out of memory");
                return NULL;
        }
        return ptr;
}

void safe_free(void *ptr)
{
        if (ptr == NULL)
                log_warn("safe_free: ptr == NULL");
        else
                sgc_free_object(ptr);
}

void *safe_calloc(size_t nmemb, size_t size)
{
        return sgc_new_object(nmemb * size, SGC_ZERO, 0);
}

void *safe_realloc(void *ptr, size_t size)
{
        if (size == 0)
                // Not an error
                log_warn("safe_realloc: size == 0"); 

        ptr = sgc_resize_object(ptr, size, 0, 0);
        if (size > 0 && ptr == NULL) {
                log_err("safe_malloc: out of memory");
                // Return NULL. Maybe the app can handle it correctly.
                return NULL;
        }
        return ptr;
}

#else

int mem_init(int *argc)
{
        return 0;
}

void mem_cleanup()
{
}

void *safe_malloc(size_t size, int zero)
{
        if (size == 0) {
                log_warn("safe_malloc: size == 0");
                return NULL;
        }
        void *ptr = malloc(size);
        if (ptr == NULL) {
                // Return NULL. Maybe the app can handle it correctly.
                log_err("safe_malloc: out of memory");
                return NULL;
        }
        if (zero)
                memset(ptr, 0, size);
        return ptr;
}

void safe_free(void *ptr)
{
        if (ptr == NULL)
                log_warn("safe_free: ptr == NULL");
        else
                free(ptr);
}

void *safe_calloc(size_t nmemb, size_t size)
{
        return safe_malloc(nmemb * size, 1);
}

void *safe_realloc(void *ptr, size_t size)
{
        if (size == 0)
                // Not an error
                log_warn("safe_realloc: size == 0"); 

        ptr = realloc(ptr, size);
        if (size > 0 && ptr == NULL) {
                log_err("safe_malloc: out of memory");
                // Return NULL. Maybe the app can handle it correctly.
                return NULL;
        }
        return ptr;
}

#endif // MEM_DIAGNOSTICS

char *safe_strdup(const char *s)
{
        if (s == NULL) {
                log_warn("safe_strdup: s == NULL!"); 
                return NULL;
        }
        int len = 0;
        while (s[len] != 0) {
                len++;
                if (len >= 1048576) {
                        log_warn("safe_strdup: strlen(s) > 1MB! Too big."); 
                        return NULL;
                }
        }
        char *t = mem_alloc(len + 1);
        memcpy(t, s, len+1);
        return t;
}
