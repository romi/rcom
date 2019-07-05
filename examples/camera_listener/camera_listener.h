#include <rcom.h>

int camera_listener_init(int argc, char **argv);
void camera_listener_cleanup();
int camera_listener_ondata(void *userdata, const char *buf, int len);
