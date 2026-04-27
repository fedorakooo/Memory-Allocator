#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "allocator.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
    LOOPS = 200000
};

static double now_sec(void)
{
    struct timespec ts;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(void)
{
    double t0 = now_sec();
    for (int i = 0; i < LOOPS; i++)
    {
        void* p = my_malloc(64U);
        assert(p != NULL);
        my_free(p);
    }
    double t1 = now_sec();

    double t2 = now_sec();
    for (int i = 0; i < LOOPS; i++)
    {
        void* p = malloc(64U);
        assert(p != NULL);
        free(p);
    }
    double t3 = now_sec();

    printf("my_malloc/free  %d loops: %.4f s\n", LOOPS, t1 - t0);
    printf("libc malloc/free %d loops: %.4f s\n", LOOPS, t3 - t2);
    printf("test_perf: ok\n");
    return 0;
}
