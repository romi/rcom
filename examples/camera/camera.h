#include <rcom.h>
#include "camera_v4l.h"

camera_t *camera_instance();
int camera_init(int argc, char **argv);
void camera_cleanup();
int camera_broadcast(camera_t* camera, streamer_t *streamer);
