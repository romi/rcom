#include <rcom.h>

int binwrite_init(int argc, char **argv);
void binwrite_cleanup();
int binwrite_broadcast(void *userdata, datahub_t *hub);
