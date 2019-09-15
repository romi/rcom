#include "camera_listener.h"

static multipart_parser_t *parser = NULL;
static double start_time = 0.0;
static double count_images = 0;

static int *camera_listener_onpart(void *userdata,
                                   const unsigned char *data, int len,
                                   const char *mimetype,
                                   double timestamp)
{
        double t = clock_time();
        r_info("Got image. Delay %f", t - timestamp);
        if (start_time == 0) {
                start_time = t;
        }
        count_images += 1;
        if (count_images > 10) {
                r_info("FPS %.1f", (count_images - 1.0) / (t - start_time));
        }
        return 0;
}

int camera_listener_init(int argc, char **argv)
{
        parser = new_multipart_parser(NULL, NULL,
                                      (multipart_onpart_t) camera_listener_onpart);
        return parser == NULL? -1 : 0;
}

void camera_listener_cleanup()
{
        if (parser)
                delete_multipart_parser(parser);
}

int camera_listener_ondata(void *userdata, response_t *response, const char *buf, int len)
{
        return multipart_parser_process(parser, response, buf, len);
}


