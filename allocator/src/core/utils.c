#include "utils.h"

#include <stdint.h>

size_t util_align_up(size_t value, size_t alignment)
{
    if (alignment == 0U)
    {
        return value;
    }
    size_t rem = value % alignment;
    if (rem == 0U)
    {
        return value;
    }
    return value + (alignment - rem);
}

void* util_align_ptr_up(void* ptr, size_t alignment)
{
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t aligned = (uintptr_t)util_align_up((size_t)p, alignment);
    return (void*)aligned;
}

int util_mul_overflow_size(size_t a, size_t b, size_t* out)
{
    if (out == NULL)
    {
        return 1;
    }
    if (a != 0U && b > ((size_t)-1) / a)
    {
        return 1;
    }
    *out = a * b;
    return 0;
}

int util_add_overflow_size(size_t a, size_t b, size_t* out)
{
    if (out == NULL)
    {
        return 1;
    }
    if (b > ((size_t)-1) - a)
    {
        return 1;
    }
    *out = a + b;
    return 0;
}
