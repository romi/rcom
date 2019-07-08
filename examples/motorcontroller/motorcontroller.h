#include <rcom.h>

void motorcontroller_onconnect(messagehub_t *hub,
                               messagelink_t *link,
                               void *userdata);
void motorcontroller_broadcast();
