#include <rcom.h>

int camera_listener_init(int argc, char **argv);
void camera_listener_cleanup();
int camera_listener_ondata(void *userdata, response_t *response, const char *buf, int len);
