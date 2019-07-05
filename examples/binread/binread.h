#include <rcom.h>

int binread_init(int argc, char **argv);
void binread_cleanup();
int binread_ondata(void *userdata, datalink_t *datalink, data_t *input, data_t *output);
