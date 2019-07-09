#include <rcom.h>
#include "binread.h"

static datalink_t *datalink_binread = NULL;

datalink_t *get_datalink_binread() {
        return datalink_binread;
}


static int init_com()
{
        int err;
        datalink_binread = registry_open_datalink("binread", "binwrite.data",
                (datalink_ondata_t) binread_ondata, NULL);
        if (datalink_binread == NULL) {
                log_err("Failed to create the datalink");
                return -1;
        };
        return 0;
}

static void cleanup_com()
{
        if (datalink_binread)
                registry_close_datalink(datalink_binread);
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

