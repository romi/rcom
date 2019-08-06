
#ifndef _RCOM_MEM_H_
#define _RCOM_MEM_H_

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int mem_init(int *argc);
void mem_cleanup();
        
void *safe_malloc(size_t size, int zero);
void safe_free(void *ptr);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);

#define new_obj(_type)         ((_type*)safe_malloc(sizeof(_type), 1))
#define delete_obj(_p)         safe_free(_p)
#define mem_alloc(_size)       safe_malloc(_size, 0)
#define mem_calloc(_n,_size)   safe_calloc(_n,_size)
#define mem_realloc(_p,_size)  safe_realloc(_p,_size)
#define mem_free(_p)           safe_free(_p)
#define mem_strdup(_s)         safe_strdup(_s)

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MEM_H_
