#ifndef _RCOM_CIRCULAR_H_
#define _RCOM_CIRCULAR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _circular_buffer_t circular_buffer_t;

circular_buffer_t* new_circular_buffer(int size);
void delete_circular_buffer(circular_buffer_t* r);

int circular_buffer_available(circular_buffer_t* r);
int circular_buffer_space(circular_buffer_t* r);
void circular_buffer_write(circular_buffer_t* r, const char* buffer, int len);
void circular_buffer_read(circular_buffer_t* r, char* buffer, int len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_CIRCULAR_H_

