
#ifndef _RCOM_MEM_H_
#define _RCOM_MEM_H_

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define MEM_DIAGNOSTICS 0

#if MEM_DIAGNOSTICS
#define SGC_USE_THREADS 1
#include <sgc.h>
#define new_obj(_type)         (_type*) sgc_new_object(sizeof(_type), SGC_ZERO, 0)
#define delete_obj(_p)         sgc_free_object((void*)_p)
#define mem_alloc(_size)       sgc_new_object(_size, SGC_ZERO, 0)
#define mem_calloc(_n,_size)   sgc_new_object(_size, SGC_ZERO, 0)
#define mem_realloc(_p,_size)  sgc_resize_object((void*)_p,_size, 0, 0)
#define mem_free(_p)           sgc_free_object((void*)_p)

#else
void *safe_malloc(size_t size, int zero);
void safe_free(void *ptr);
void *safe_calloc(size_t nmemb, size_t size);
void *safe_realloc(void *ptr, size_t size);
#define new_obj(_type)         ((_type*)safe_malloc(sizeof(_type), 1))
#define delete_obj(_p)         safe_free(_p)
#define mem_alloc(_size)       safe_malloc(_size, 0)
#define mem_calloc(_n,_size)   safe_calloc(_n,_size)
#define mem_realloc(_p,_size)  safe_realloc(_p,_size)
#define mem_free(_p)           safe_free(_p)
#endif

char *safe_strdup(const char *s);
#define mem_strdup(_s)         safe_strdup(_s)

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MEM_H_
