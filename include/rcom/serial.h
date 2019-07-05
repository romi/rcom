#ifndef _RCOM_SERIAL_H_
#define _RCOM_SERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _serial_t serial_t;

serial_t *new_serial(const char *device, int speed);
void delete_serial(serial_t *s);

int serial_get(serial_t *s);
int serial_read(serial_t *s, char *buf, int len);
const char *serial_readline(serial_t *s);

// returns 0: no error, -1: error
// These functions are not protected by a mutex. 
int serial_put(serial_t *s, char c);
int serial_write(serial_t *s, const char *buf, int len);
int serial_print(serial_t *s, const char *line);
int serial_println(serial_t *s, const char *line);
int serial_printf(serial_t *s, const char *format, ...);

// serial_send_command() is a combination of a println and a
// readline. The sending and reading are performed as a single
// operation protected by a mutex to avoid being interrupted by
// another thread.
const char *serial_command_send(serial_t *s, const char *cmd);
const char *serial_command_sendf(serial_t *s, const char *format, ...);


void serial_lock(serial_t *s);
void serial_unlock(serial_t *s);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_SERIAL_H_
