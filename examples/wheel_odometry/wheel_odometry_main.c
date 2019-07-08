#include <rcom.h>
#include "wheel_odometry.h"

static messagelink_t *messagelink_wheel_odometry = NULL;

messagelink_t *get_messagelink_wheel_odometry() {
        return messagelink_wheel_odometry;
}


static int init_com()
{
        int err;
        messagelink_wheel_odometry = registry_open_messagelink("wheel_odometry", "encoders",
                (messagelink_onmessage_t) wheel_odometry_onmessage, NULL);
        if (messagelink_wheel_odometry == NULL) {
                log_err("Failed to create the messagelink");
                return -1;
        };
        return 0;
}

static void cleanup_com()
{
        if (messagelink_wheel_odometry)
                registry_close_messagelink(messagelink_wheel_odometry);
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

