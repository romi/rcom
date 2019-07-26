#include <string.h>
#include "motorcontroller.h"

static double start_time = 0;
static double encoders[] = { 0.0, 0.0 };
static double speed = 0.0;

messagehub_t *get_messagehub_encoders();

void motorcontroller_onmessage(messagelink_t *link,
                               void *userdata,
                               json_object_t message)
{
        const char *request = json_object_getstr(message, "request");
        if (request != NULL && strcmp(request, "moveat") == 0) {
                json_object_t arg = json_object_get(message, "arg");
                double left = json_array_getnum(arg, 0);
                double right = json_array_getnum(arg, 1);
                if (isnan(left) || isnan(right))
                        return;
                speed = left;
        }
}

void motorcontroller_onconnect(messagehub_t *hub,
                               messagelink_t *link,
                               void *userdata)
{
        messagelink_set_onmessage(link, motorcontroller_onmessage);
}

void motorcontroller_broadcast()
{
        messagehub_t *hub = get_messagehub_encoders();
        if (start_time == 0)
                start_time = clock_time();

        double position;
        double t = clock_time();
        t = fmod((t - start_time), 240.0);
        if (t < 120)
                position = 3.6 * t / 120;
        else
                position = 3.6 - 3.6 * (t - 120) / 120;
        messagehub_broadcast_f(hub, NULL, "[%f,%f]", position, 0.45);
        //messagehub_broadcast_f(hub, NULL, "[%f,%f]", encoders[0], encoders[1]);
        clock_sleep(1);
}
