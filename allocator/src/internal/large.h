#ifndef LARGE_H
#define LARGE_H

#include <stddef.h>

void* my_large_malloc(size_t size);
void my_large_free(void* p);
void* my_large_realloc(void* p, size_t size);

#endif
