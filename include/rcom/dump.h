#ifndef _RCOM_DUMP_H_
#define _RCOM_DUMP_H_

#include <stdio.h>
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _dump_t dump_t;
        
void set_dumping(int value);
int get_dumping();
void set_dumping_dir(const char *path);
const char *get_dumping_dir();

int do_replay();
void set_replay_id(const char *id);
const char *get_replay_id();

dump_t *dump_create(const char *name,
                    int type,
                    const char *topic,
                    const char *mimetype);

dump_t *dump_open(const char *path);

int dump_type(dump_t *dump);

void delete_dump(dump_t *dump);
int dump_write_data(dump_t *dump, data_t *data);
int dump_read_data(dump_t *dump, data_t *data);

int dump_write_buffer(dump_t *dump, char *data, uint32_t len);
int dump_read_buffer(dump_t *dump, char *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_DUMP_H_
