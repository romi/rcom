#include "binwrite.h"

int binwrite_init(int argc, char **argv)
{
        return 0;
}

void binwrite_cleanup()
{
}

int binwrite_broadcast(void *userdata, datahub_t *hub)
{
        char buffer[1024];
        datahub_broadcast_bin(hub, NULL, buffer, 1024);
        return 0;
}
