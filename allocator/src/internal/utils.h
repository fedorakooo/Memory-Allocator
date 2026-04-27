#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

size_t util_align_up(size_t value, size_t alignment);
void* util_align_ptr_up(void* ptr, size_t alignment);

int util_mul_overflow_size(size_t a, size_t b, size_t* out);
int util_add_overflow_size(size_t a, size_t b, size_t* out);

#endif
