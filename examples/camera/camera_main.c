#include <rcom.h>
#include "camera.h"

static streamer_t *streamer_camera = NULL;

streamer_t *get_streamer_camera() {
        return streamer_camera;
}


static int init_com()
{
        int err;
        streamer_camera = registry_open_streamer("camera",
                "camera",
                0,
                (streamer_onclient_t) NULL,
                (streamer_onbroadcast_t) camera_broadcast,
                camera_instance());
        if (streamer_camera == NULL) {
                log_err("Failed to create the streamer");
                return -1;
        };
        err = streamer_export(streamer_camera,
               "rgb",
               "multipart/x-mixed-replace; boundary=nextimage");
        if (err) return -1;
        return 0;
}

static void cleanup_com()
{
        if (streamer_camera)
                registry_close_streamer(streamer_camera);
}

static int init(int argc, char **argv)
{
        int err = 0;
        
        err = camera_init(argc, argv);
        if (err != 0)
                return err;

        err = init_com();
        if (err != 0)
                return err;

        return 0;
}

static void cleanup()
{
        camera_cleanup();
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

