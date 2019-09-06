#include <errno.h>
#include <string.h>

#include <r.h>

#include "rcom/dump.h"
#include "rcom/util.h"

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
                r_free(_dumping_dir);
                _dumping_dir = NULL;
        }
        if (path == NULL)
                return;
        _dumping_dir = r_strdup(path);
        r_info("Dumping to directory '%s'", _dumping_dir);
}

const char *get_dumping_dir()
{
        return _dumping_dir;
}

void set_replay_id(const char *id)
{
        if (_replay_id != NULL) {
                r_free(_replay_id);
                _replay_id = NULL;
        }
        if (id == NULL)
                return;
        _replay_id = r_strdup(id);
        r_info("Replay ID is '%s'", _replay_id);
}

const char *get_replay_id()
{
        return _replay_id;
}

int do_replay()
{
        return _replay_id != NULL;
}

/***********************************************************/

struct _dump_t {
        FILE *fp;
        char *name;
        int type;
        char *topic;
        char *mimetype;
        uint32_t offset;
};

dump_t *new_dump()
{
        dump_t *dump = r_new(dump_t);
        if (dump == NULL) return NULL;
        return dump;
}

void delete_dump(dump_t *dump)
{
        if (dump) {
                if (dump->fp)
                        fclose(dump->fp);
                if (dump->name)
                        r_free(dump->name);
                if (dump->topic)
                        r_free(dump->topic);
                if (dump->mimetype)
                        r_free(dump->mimetype);
        }
}

int dump_type(dump_t *dump)
{
        return dump->type;
}

dump_t *dump_create(const char *name,
                    int type,
                    const char *topic,
                    const char *mimetype)
{
        char path[1024];
        char timestamp[128];
        const char *stype = registry_type_to_str(type); 
        
        dump_t *dump = new_dump();
        if (dump == NULL) return NULL;

        dump->name = r_strdup(name);
        dump->type = type;
        dump->topic = r_strdup(topic);
        dump->mimetype = r_strdup(mimetype);
        if (dump->name == NULL
            || dump->topic == NULL
            || dump->mimetype == NULL) {
                delete_dump(dump);
                return NULL;
        }
        
        clock_datetime(timestamp, 128, '-', '_', '-');

        if (_dumping_dir == NULL) {
                snprintf(path, 1024, "%s-%s-%s-%s.dump",
                         name, stype, topic, timestamp);
        } else {
                snprintf(path, 1024, "%s/%s-%s-%s-%s.dump",
                         _dumping_dir, name, stype, topic, timestamp);
        }
        dump->fp = fopen(path, "wb");
        if (dump->fp == NULL) {
                char reason[200];
                strerror_r(errno, reason, 200);
                r_err("Failed to open dump '%s': %s", path, reason);
                delete_dump(dump);
                return NULL;
        }
        r_info("Dumping to file '%s'", path);
        fprintf(dump->fp, "%s\n%s\n%s\n%s\n", name, stype, topic, mimetype);
        
        return dump;
}

dump_t *dump_open(const char *path)
{
        char name[128];
        char stype[128];
        char topic[128];
        char mimetype[128];
        char *p;
        
        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
                char reason[200];
                strerror_r(errno, reason, 200);
                r_err("Failed to open dump '%s': %s", path, reason);
                return NULL;
        }

        if (fgets(name, 128, fp) == NULL) {
                r_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(name, '\n')) != NULL) *p = '\0';

        if (fgets(stype, 128, fp) == NULL) {
                r_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(stype, '\n')) != NULL) *p = '\0';

        if (fgets(topic, 128, fp) == NULL) {
                r_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(topic, '\n')) != NULL) *p = '\0';

        if (fgets(mimetype, 128, fp) == NULL) {
                r_err("Failed to read the dump header in '%s'", path);
                fclose(fp);
                return NULL;
        }
        if ((p = strchr(mimetype, '\n')) != NULL) *p = '\0';

        
        dump_t *dump = new_dump();
        if (dump == NULL) return NULL;

        dump->name = r_strdup(name);
        dump->type = registry_str_to_type(stype);
        dump->topic = r_strdup(topic);
        dump->mimetype = r_strdup(mimetype);
        if (dump->name == NULL
            || dump->topic == NULL
            || dump->mimetype == NULL) {
                delete_dump(dump);
                return NULL;
        }
        
        return dump;
}

static inline int dump_write(dump_t *dump, void *ptr, uint32_t size)
{
        uint32_t s = fwrite(ptr, 1, size, dump->fp);
        if (s != size) {
                r_err("Failed to write to the dump");
                return -1;
        }
        return 0;
}

static inline int dump_read(dump_t *dump, char *ptr, uint32_t size)
{
        uint32_t s = fread(ptr, 1, size, dump->fp);
        if (s != size) {
                if (feof(dump->fp))
                        return -1;
                else if (ferror(dump->fp)) {
                        r_err("Dump file has the error indicator set");
                        return -1;
                } else {
                        r_err("Failed to read the data");
                        return -1;
                }
        }
        return 0;
}

int dump_write_data(dump_t *dump, data_t *data)
{
        int err;
        uint32_t n = PACKET_HEADER + data_len(data);
        unsigned char b[4];
        b[0] = (unsigned char) ((n & 0xff000000) >> 24);
        b[1] = (unsigned char) ((n & 0x00ff0000) >> 16);
        b[2] = (unsigned char) ((n & 0x0000ff00) >> 8);
        b[3] = (unsigned char) (n & 0x000000ff);
        err = dump_write(dump, b, 4);
        if (err != 0) return err;
        return dump_write(dump, data_packet(data), n);        
}

int dump_write_buffer(dump_t *dump, char* buffer, uint32_t len)
{
        return dump_write(dump, buffer, len);        
}

int dump_read_data(dump_t *dump, data_t *data)
{
        unsigned char b[4];
        int err;
        uint32_t n;
        
        err = dump_read(dump, b, 4);
        if (err != 0)
                return err;
        
        n = (((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16)
             | ((uint32_t)b[2] << 8) | (uint32_t)b[3]);
        if (n > DATA_MAXLEN) {
                r_err("Invalid data length: %d", n);
                return -1;
        }
        
        err = dump_read_buffer(dump, (void*) data_packet(data), n);
        if (err != 0)
                return err;

        data_set_len(data, n - PACKET_HEADER);

        return 0;
}

int dump_read_buffer(dump_t *dump, char *ptr, uint32_t size)
{
        return dump_read(dump, ptr, size);
}

