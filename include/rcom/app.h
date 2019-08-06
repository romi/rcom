#ifndef _RCOM_APP_H_
#define _RCOM_APP_H_

#ifdef __cplusplus
extern "C" {
#endif

void app_init(int *argc, char **argv);

int app_quit();
void app_set_quit();

void app_set_name(const char *path);
const char *app_get_name();
const char *app_ip();

void app_set_logdir(const char *logdir);
const char *app_get_logdir();

const char *app_get_config();

int app_print();
int app_standalone();

#ifdef __cplusplus
}
#endif

#endif // _RCOM_APP_H_
