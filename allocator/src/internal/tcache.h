#ifndef TCACHE_H
#define TCACHE_H

#include <stddef.h>

void* tcache_alloc(size_t size);
void tcache_free(void* p);

#endif
