#include "wheel_odometry.h"

static int have_values = 0;
static double position = 0.0;
static double last_encoders[] = { 0.0, 0.0 };

void wheel_odometry_onmessage(messagelink_t *link,
                               void *userdata,
                               json_object_t message)
{
        double left = json_array_getnum(message, 0);
        double right = json_array_getnum(message, 1);
        if (have_values) {
                position += (left - last_encoders[0]) / 1000.0;
                log_debug("position %f", position);
        }
        last_encoders[0] = left;
        last_encoders[1] = right;
        have_values = 1;
}

