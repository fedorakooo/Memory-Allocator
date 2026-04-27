#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void* my_malloc(size_t size);
    void my_free(void* ptr);
    void* my_calloc(size_t nmemb, size_t size);
    void* my_realloc(void* ptr, size_t size);

    void allocator_stats(void);

#ifdef __cplusplus
}
#endif

#endif
