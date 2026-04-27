#include "allocator.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NPTRS 4096U
#define ITERS 50000U

int main(void)
{
    void* ptrs[NPTRS];
    size_t lens[NPTRS];
    memset(ptrs, 0, sizeof(ptrs));
    memset(lens, 0, sizeof(lens));

    srand((unsigned int)time(NULL) ^ (unsigned int)clock());

    for (unsigned iter = 0; iter < ITERS; iter++)
    {
        unsigned idx = (unsigned)rand() % NPTRS;
        if (ptrs[idx] != NULL)
        {
            my_free(ptrs[idx]);
            ptrs[idx] = NULL;
            lens[idx] = 0U;
        }

        size_t sz = (size_t)((unsigned)rand() % 2048U) + 1U;
        int op = (int)((unsigned)rand() % 4U);
        if (op == 0)
        {
            ptrs[idx] = my_malloc(sz);
            lens[idx] = (ptrs[idx] != NULL) ? sz : 0U;
        }
        else if (op == 1)
        {
            size_t n = (size_t)((unsigned)rand() % 32U) + 1U;
            size_t es = (sz / n) + 1U;
            size_t total = 0U;
            if (n != 0U && es > SIZE_MAX / n)
            {
                ptrs[idx] = NULL;
                lens[idx] = 0U;
            }
            else
            {
                total = n * es;
                ptrs[idx] = my_calloc(n, es);
                lens[idx] = (ptrs[idx] != NULL) ? total : 0U;
            }
        }
        else
        {
            ptrs[idx] = my_malloc(sz);
            lens[idx] = (ptrs[idx] != NULL) ? sz : 0U;
            if (ptrs[idx] != NULL)
            {
                size_t nsz = (size_t)((unsigned)rand() % 4096U) + 1U;
                void* nptr = my_realloc(ptrs[idx], nsz);
                if (nptr == NULL && nsz != 0U)
                {
                }
                else
                {
                    ptrs[idx] = nptr;
                    lens[idx] = (nptr != NULL) ? nsz : 0U;
                }
            }
        }

        if (ptrs[idx] != NULL && lens[idx] > 0U)
        {
            (void)memset(ptrs[idx], (int)(idx & 0xFFU), lens[idx]);
        }
    }

    for (unsigned i = 0; i < NPTRS; i++)
    {
        if (ptrs[i] != NULL)
        {
            my_free(ptrs[i]);
            ptrs[i] = NULL;
            lens[i] = 0U;
        }
    }

    allocator_stats();
    printf("test_stress: ok\n");
    return 0;
}
