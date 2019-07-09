#include <rcom.h>
#include "motorcontroller.h"

static messagehub_t *messagehub_encoders = NULL;

messagehub_t *get_messagehub_encoders() {
        return messagehub_encoders;
}

static messagehub_t *messagehub_motorcontroller = NULL;

messagehub_t *get_messagehub_motorcontroller() {
        return messagehub_motorcontroller;
}


static int init_com()
{
        int err;
        messagehub_encoders = registry_open_messagehub("encoders", "encoders",
                0, (messagehub_onconnect_t) NULL, NULL);
        if (messagehub_encoders == NULL) {
                log_err("Failed to create the messagehub");
                return -1;
        };
        messagehub_motorcontroller = registry_open_messagehub("motorcontroller", "motorcontroller",
                0, (messagehub_onconnect_t) motorcontroller_onconnect, NULL);
        if (messagehub_motorcontroller == NULL) {
                log_err("Failed to create the messagehub");
                return -1;
        };
        return 0;
}

static void cleanup_com()
{
        if (messagehub_encoders)
                registry_close_messagehub(messagehub_encoders);
        if (messagehub_motorcontroller)
                registry_close_messagehub(messagehub_motorcontroller);
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
        motorcontroller_broadcast();
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

