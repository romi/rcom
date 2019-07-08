#include <rcom.h>
#include "binwrite.h"

static datahub_t *datahub_binwrite = NULL;

datahub_t *get_datahub_binwrite() {
        return datahub_binwrite;
}


static int init_com()
{
        int err;
        datahub_binwrite = registry_open_datahub("binwrite", "binwrite.data",
                (datahub_onbroadcast_t) binwrite_broadcast,
                (datahub_ondata_t) NULL,
                NULL);
        if (datahub_binwrite == NULL) {
                log_err("Failed to create the datahub");
                return -1;
        };
        return 0;
}

static void cleanup_com()
{
        if (datahub_binwrite)
                registry_close_datahub(datahub_binwrite);
}

static int init(int argc, char **argv)
{
        int err = 0;
        
        err = init_com();
        if (err != 0)
                return err;

        return 0;
}

static void cleanup()
{
        cleanup_com();
}

static void idle()
{
        clock_sleep(1.0);
}

int main(int argc, char** argv)
{
        int err = 0;

        app_init(&argc, argv);

        err = init(argc, argv);
        if (err != 0) {
                log_err("init() failed");
                goto cleanup_and_exit;
        }

        while (!app_quit())
                idle();

cleanup_and_exit:

        cleanup();

        return err? 1 : 0;
}

