#include <rcom.h>
#include "camera_impl.h"

camera_t *camera_instance();
int video4linux_init(int argc, char **argv);
void video4linux_cleanup();
int camera_broadcast(camera_t* camera, streamer_t *streamer);
