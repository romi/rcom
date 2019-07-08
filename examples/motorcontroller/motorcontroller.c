#include <string.h>
#include "motorcontroller.h"

static double speed = 0.0;
static double encoders[] = { 0.0, 0.0 };

messagehub_t *get_messagehub_motorcontroller();

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
        messagehub_t *hub = get_messagehub_motorcontroller();
        log_debug("motorcontroller_broadcast: sending values");
        messagehub_broadcast_f(hub, NULL, "[%f,%f]", encoders[0], encoders[1]);
        encoders[0] += speed * 1000;
        encoders[1] += speed * 1000;
        clock_sleep(1);
}



