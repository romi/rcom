#include <errno.h>
#include <string.h>

#include "rcom/log.h"
#include "rcom/dump.h"
#include "rcom/clock.h"
#include "rcom/util.h"

#include "mem.h"
#include "registry_priv.h"

static int _dumping = 0;
static char *_dumping_dir = NULL;
static char *_replay_id = NULL;

void set_dumping(int value)
{
        _dumping = value;
}

int get_dumping()
{
        return _dumping;
}

void set_dumping_dir(const char *path)
{
        if (_dumping_dir != NULL) {
                mem_free(_dumping_dir);
                _dumping_dir = NULL;
        }
        if (path == NULL)
                return;
        _dumping_dir = mem_strdup(path);
        log_info("Dumping to directory '%s'", _dumping_dir);
}

const char *get_dumping_dir()
{
        return _dumping_dir;
}

void set_replay_id(const char *id)
{
        if (_replay_id != NULL) {
                mem_free(_replay_id);
                _replay_id = NULL;
        }
        if (id == NULL)
                return;
        _replay_id = mem_strdup(id);
        log_info("Replay ID is '%s'", _replay_id);
}

const char *get_replay_id()
{
        return _replay_id;
}

int do_replay()
{
        return _replay_id != NULL;
}

FILE* create_dump(const char *name, const char *exp, const char *mimetype, int type)
{
        char path[1024];
        char timestamp[128];

        clock_datetime(timestamp, 128, '-', '_', '-');

        if (_dumping_dir == NULL) {
                snprintf(path, 1024, "%s-%s-%s.dump",
                         name, exp, timestamp);
        } else {
                snprintf(path, 1024, "%s/%s-%s-%s.dump",
                         _dumping_dir, name, exp, timestamp);
        }
        FILE *fp = fopen(path, "wb");
        if (fp == NULL) {
                char reason[200];
                strerror_r(errno, reason, 200);
                log_err("Failed to open dump '%s': %s", path, reason);
        }
        log_info("Dumping to file '%s'", path);
        fprintf(fp, "%s\n%s\n%s\n%s\n", name, exp, mimetype, registry_type_to_str(type));
        return fp;
}

FILE* open_dump(const char *path, char **namep, char **expp, char **mimetypep, int *typep, int *offset)
{
        char name[128];
        char exp[128];
        char mimetype[128];
        char stype[128];
        char *p;
        
        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
                char reason[200];
                strerror_r(errno, reason, 200);
                log_err("Failed to open dump '%s': %s", path, reason);
        }

        if (fgets(name, 128, fp) == NULL) {
                log_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(name, '\n')) != NULL) *p = '\0';


        if (fgets(exp, 128, fp) == NULL) {
                log_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(exp, '\n')) != NULL) *p = '\0';

        if (fgets(mimetype, 128, fp) == NULL) {
                log_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(mimetype, '\n')) != NULL) *p = '\0';

        if (fgets(stype, 128, fp) == NULL) {
                log_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(stype, '\n')) != NULL) *p = '\0';

        /* int n = fscanf(fp, "%127s\n%127s\n%127s\n%127s\n", name, exp, mimetype, stype); */
        /* if (n != 4) { */
        /*         if (ferror(fp)) { */
        /*                 char reason[200]; */
        /*                 strerror_r(errno, reason, 200); */
        /*                 log_err("Failed to read the dump header in '%s': %s", path, reason); */
        /*         } */
        /*         fclose(fp); */
        /*         return NULL; */
        /* } */
        
        *namep = mem_strdup(name);
        *expp = mem_strdup(exp);
        *mimetypep = mem_strdup(mimetype);
        *typep = registry_str_to_type(stype);
        *offset = ftell(fp);
        
        return fp;
}

static inline int write_(FILE* fp, void *ptr, size_t size)
{
        size_t s = fwrite(ptr, 1, size, fp);
        if (s != size) {
                log_err("Failed to write to the dump");
                return -1;
        }
        return 0;
}

int dump_data(FILE* fp, data_t *data)
{
        int err;
        uint32_t n = PACKET_HEADER + data_len(data);
        unsigned char b[4];
        b[0] = (unsigned char) ((n & 0xff000000) >> 24);
        b[1] = (unsigned char) ((n & 0x00ff0000) >> 16);
        b[2] = (unsigned char) ((n & 0x0000ff00) >> 8);
        b[3] = (unsigned char) (n & 0x000000ff);
        err = write_(fp, b, 4);
        if (err != 0) return err;
        return write_(fp, data_packet(data), n);        
}

int dump_buffer(FILE* fp, char* buffer, int len)
{
        return write_(fp, buffer, len);        
}

static inline int read_(FILE* fp, void *ptr, size_t size)
{
        size_t s = fread(ptr, 1, size, fp);
        if (s != size) {
                if (feof(fp))
                        return -1;
                else if (ferror(fp)) {
                        log_err("Dump file has the error indicator set");
                        return -1;
                } else {
                        log_err("Failed to read the data");
                        return -1;
                }
        }
        return 0;
}

int dump_get_data(FILE* fp, data_t *data)
{
        unsigned char b[4];
        int err;
        uint32_t n;
        
        err = read_(fp, b, 4);
        if (err != 0)
                return err;
        
        n = (((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16)
             | ((uint32_t)b[2] << 8) | (uint32_t)b[3]);
        if (n > DATA_MAXLEN) {
                log_err("Invalid data length: %d", n);
                return -1;
        }
        
        err = read_(fp, (void*) data_packet(data), n);
        if (err != 0)
                return err;

        data_set_len(data, n - PACKET_HEADER);

        return 0;
}

void close_dump(FILE* fp)
{
        fclose(fp);
}
