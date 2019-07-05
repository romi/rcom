#include <stdio.h>
#include "binread.h"

double total_size = 0.0;
double start_time = 0;
double last_time = 0;
unsigned int count = 0;

int binread_init(int argc, char **argv)
{
        return 0;
}

void binread_cleanup()
{
}

int binread_ondata(void *userdata, datalink_t *datalink, data_t *input, data_t *output)
{
        double t = clock_time();
        total_size += (double) data_len(input);
        count++;

        if (t - last_time > 1.0) {
                if (start_time == 0)
                        start_time = t - 0.0001;
                double bandwidth = total_size / ((double)t - start_time) / 1048576.0;
                printf("total_size %f GB, %u packets, %f MB/s\n",
                       total_size/1024.0/1024.0/1024.0, count, bandwidth);
                last_time = t;
        }
        
        return 0;
}
