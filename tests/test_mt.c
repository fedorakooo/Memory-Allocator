#include "allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <pthread.h>

typedef struct worker_arg
{
    unsigned tid;
    unsigned iters;
} worker_arg;

static void* worker(void* vp)
{
    worker_arg* a = (worker_arg*)vp;
    const unsigned slots = 256U;
    void* ptrs[256];
    size_t szs[256];
    memset(ptrs, 0, sizeof(ptrs));
    memset(szs, 0, sizeof(szs));

    for (unsigned i = 0; i < a->iters; i++)
    {
        unsigned idx = (unsigned)((i * 1103515245u + a->tid * 12345u) % slots);
        unsigned op = (unsigned)((i + a->tid) % 3U);
        size_t size = (size_t)(((i * 2654435761u) ^ (a->tid * 97u)) % 65536u) + 1U;

        if (op == 0U)
        {
            if (ptrs[idx] != NULL)
            {
                my_free(ptrs[idx]);
                ptrs[idx] = NULL;
                szs[idx] = 0U;
            }
            void* p = my_malloc(size);
            assert(p != NULL);
            memset(p, (int)(a->tid & 0xFFu), size < 64U ? size : 64U);
            ptrs[idx] = p;
            szs[idx] = size;
        }
        else if (op == 1U)
        {
            if (ptrs[idx] == NULL)
            {
                continue;
            }
            size_t new_sz = size;
            void* q = my_realloc(ptrs[idx], new_sz);
            assert(q != NULL);
            size_t chk = szs[idx] < 64U ? szs[idx] : 64U;
            for (size_t k = 0; k < chk; k++)
            {
                assert(((unsigned char*)q)[k] == (unsigned char)(a->tid & 0xFFu));
            }
            memset(q, (int)(a->tid & 0xFFu), new_sz < 64U ? new_sz : 64U);
            ptrs[idx] = q;
            szs[idx] = new_sz;
        }
        else
        {
            if (ptrs[idx] != NULL)
            {
                my_free(ptrs[idx]);
                ptrs[idx] = NULL;
                szs[idx] = 0U;
            }
        }
    }

    for (unsigned i = 0; i < slots; i++)
    {
        if (ptrs[i] != NULL)
        {
            my_free(ptrs[i]);
        }
    }
    return NULL;
}

int main(void)
{
    const unsigned nthreads = 8U;
    const unsigned iters = 50000U;

    pthread_t th[nthreads];
    worker_arg args[nthreads];
    for (unsigned i = 0; i < nthreads; i++)
    {
        args[i].tid = i;
        args[i].iters = iters;
        int rc = pthread_create(&th[i], NULL, worker, &args[i]);
        assert(rc == 0);
    }
    for (unsigned i = 0; i < nthreads; i++)
    {
        (void)pthread_join(th[i], NULL);
    }

    printf("test_mt: ok\n");
    return 0;
}

#else
int main(void)
{
    printf("test_mt: skipped on Windows\n");
    return 0;
}
#endif
