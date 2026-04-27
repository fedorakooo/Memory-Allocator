#include "allocator.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    char* p = (char*)my_malloc(64U);
    if (p == NULL)
    {
        return 1;
    }
    (void)memcpy(p, "hello", 6);

    char* q = (char*)my_realloc(p, 128U);
    if (q == NULL)
    {
        my_free(p);
        return 1;
    }
    p = q;

    int* z = (int*)my_calloc(10U, sizeof(int));
    if (z == NULL)
    {
        my_free(p);
        return 1;
    }

    for (int i = 0; i < 10; i++)
    {
        if (z[i] != 0)
        {
            my_free(z);
            my_free(p);
            return 1;
        }
    }

    allocator_stats();

    my_free(z);
    my_free(p);
    return 0;
}
