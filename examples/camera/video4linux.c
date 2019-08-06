#include "video4linux.h"

static const char *device = "/dev/video0";
static camera_t* camera = NULL;

camera_t *camera_instance()
{
        return camera;
}

int video4linux_init(int argc, char **argv)
{
        if (argc == 2)
                device = argv[1];
        
        camera = new_camera(device, IO_METHOD_MMAP, 640, 480, 90);
        if (camera == NULL) {
                log_err("Failed to open the camera");
                return -1;
        }
        return 0;
}

void video4linux_cleanup()
{
        if (camera)
                delete_camera(camera);
}

int camera_broadcast(camera_t* camera, streamer_t *streamer)
{
        if (streamer_has_clients(streamer)) {
                int error = camera_capture(camera);
                if (error) {
                        log_err("Failed to grab the image");
                        clock_sleep(0.04);
                        return 0;
                }

                double timestamp = clock_time();
        
                streamer_send_multipart(streamer, 
                                        camera_getimagebuffer(camera),
                                        camera_getimagesize(camera),
                                        "image/jpeg", timestamp);
        } else {
                clock_sleep(0.1);
        }
        return 0;
}
