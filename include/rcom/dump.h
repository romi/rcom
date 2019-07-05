#ifndef _RCOM_DUMP_H_
#define _RCOM_DUMP_H_

#include <stdio.h>
#include "data.h"

#ifdef __cplusplus
extern "C" {
#endif

void set_dumping(int value);
int get_dumping();
void set_dumping_dir(const char *path);
const char *get_dumping_dir();

FILE* create_dump(const char *name, const char *exp, const char *mimetype, int type);
int dump_data(FILE* fp, data_t *data);
int dump_buffer(FILE* fp, char* buffer, int len);
void close_dump(FILE* fp);


FILE* open_dump(const char *path, char **namep, char **expp, char **mimetypep, int *typep, int *offset);
int dump_get_data(FILE* fp, data_t *data);


int do_replay();
void set_replay_id(const char *id);
const char *get_replay_id();

#ifdef __cplusplus
}
#endif

#endif // _RCOM_DUMP_H_
