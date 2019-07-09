#include <rcom.h>
#include "camera_listener.h"

static streamerlink_t *streamerlink_camera_listener = NULL;

streamerlink_t *get_streamerlink_camera_listener() {
        return streamerlink_camera_listener;
}


static int init_com()
{
        int err;
        streamerlink_camera_listener = registry_open_streamerlink("camera_listener",                "camera", "rgb",
                (streamerlink_ondata_t) camera_listener_ondata, NULL);
        if (streamerlink_camera_listener == NULL) {
                log_err("Failed to create the streamerlink");
                return -1;
        };
        return 0;
}

static void cleanup_com()
{
        if (streamerlink_camera_listener)
                registry_close_streamerlink(streamerlink_camera_listener);
}

static int init(int argc, char **argv)
{
        int err = 0;
        
        err = camera_listener_init(argc, argv);
        if (err != 0)
                return err;

        err = init_com();
        if (err != 0)
                return err;

        return 0;
}

static void cleanup()
{
        camera_listener_cleanup();
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

