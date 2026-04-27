#include "allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t alignment_expect(void)
{
    return sizeof(void*) >= 8U ? 16U : 8U;
}

int main(void)
{
    assert(my_malloc(0U) == NULL);

    void* a = my_malloc(1U);
    assert(a != NULL);
    assert(((uintptr_t)a % alignment_expect()) == 0U);
    memset(a, 0xAB, 1);
    my_free(a);

    void* b = my_malloc(1024U);
    void* c = my_malloc(2048U);
    assert(b != NULL && c != NULL);
    memset(b, 0xCD, 1024U);
    memset(c, 0xEF, 2048U);
    my_free(b);
    my_free(c);

    int* z = (int*)my_calloc(100U, sizeof(int));
    assert(z != NULL);
    for (size_t i = 0; i < 100U; i++)
    {
        assert(z[i] == 0);
    }
    my_free(z);

    char* r = (char*)my_malloc(32U);
    assert(r != NULL);
    memset(r, 'x', 32U);
    char* r2 = (char*)my_realloc(r, 256U);
    assert(r2 != NULL);
    for (size_t i = 0; i < 32U; i++)
    {
        assert(r2[i] == 'x');
    }
    my_free(r2);

    assert(my_realloc(NULL, 48U) != NULL);
    my_free(my_realloc(NULL, 48U));

    void* s = my_malloc(16U);
    assert(s != NULL);
    my_free(s);

    void* large = my_malloc(200U * 1024U);
    assert(large != NULL);
    memset(large, 0, 200U * 1024U);
    my_free(large);

    assert(my_calloc(SIZE_MAX, SIZE_MAX) == NULL);
    assert(my_calloc(SIZE_MAX / 2U + 1U, 2U) == NULL);

    printf("test_basic: ok\n");
    return 0;
}
