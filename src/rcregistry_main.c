#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <rcom.h>

#include "rcregistry_priv.h"

static rcregistry_t *rcregistry = NULL;

int main(int argc, char **argv)
{
        app_init(&argc, argv);

        rcregistry = new_rcregistry();
        if (rcregistry == NULL)
                return 1;
        
        while (!app_quit()) {
                clock_sleep(1);
        }

        delete_rcregistry(rcregistry);
        return 0;
}
