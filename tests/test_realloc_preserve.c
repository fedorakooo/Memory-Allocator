#include "allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void fill(unsigned char* p, size_t n, unsigned char seed)
{
    for (size_t i = 0; i < n; i++)
    {
        p[i] = (unsigned char)(seed + (unsigned char)i);
    }
}

static void check(const unsigned char* p, size_t n, unsigned char seed)
{
    for (size_t i = 0; i < n; i++)
    {
        assert(p[i] == (unsigned char)(seed + (unsigned char)i));
    }
}

int main(void)
{
    for (size_t start = 1U; start <= 8192U; start = start * 2U)
    {
        unsigned char* p = (unsigned char*)my_malloc(start);
        assert(p != NULL);
        fill(p, start, 0x10);

        for (size_t grow = start; grow <= 131072U; grow = grow * 2U)
        {
            unsigned char* q = (unsigned char*)my_realloc(p, grow);
            assert(q != NULL);
            check(q, start, 0x10);
            memset(q + start, 0xA5, grow - start);
            p = q;
        }
        my_free(p);
    }

    printf("test_realloc_preserve: ok\n");
    return 0;
}
