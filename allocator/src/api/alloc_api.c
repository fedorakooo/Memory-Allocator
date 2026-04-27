#include "allocator.h"

#include "my_internal.h"
#include "size_classes.h"
#include "tcache.h"
#include "utils.h"

#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static atomic_ulong g_stat_malloc = 0UL;
static atomic_ulong g_stat_free = 0UL;
static atomic_ulong g_stat_calloc = 0UL;
static atomic_ulong g_stat_realloc = 0UL;

void* my_malloc(size_t size)
{
    if (size == 0U)
    {
        return NULL;
    }
    (void)atomic_fetch_add_explicit(&g_stat_malloc, 1UL, memory_order_relaxed);
    if (size <= ALLOC_MAX_SMALL && my_size_to_class(size) >= 0)
    {
        void* p = tcache_alloc(size);
        if (p != NULL)
        {
            return p;
        }
    }
    return my_large_malloc(size);
}

void my_free(void* ptr)
{
    if (ptr == NULL)
    {
        return;
    }
    (void)atomic_fetch_add_explicit(&g_stat_free, 1UL, memory_order_relaxed);

    my_span* sp = my_lookup_span(ptr);
    if (my_span_is_small(sp))
    {
        tcache_free(ptr);
        return;
    }
    my_large_free(ptr);
}

void* my_calloc(size_t nmemb, size_t size)
{
    (void)atomic_fetch_add_explicit(&g_stat_calloc, 1UL, memory_order_relaxed);
    size_t total = 0U;
    if (util_mul_overflow_size(nmemb, size, &total) != 0)
    {
        errno = ENOMEM;
        return NULL;
    }
    void* p = my_malloc(total);
    if (p != NULL)
    {
        (void)memset(p, 0, total);
    }
    return p;
}

void* my_realloc(void* ptr, size_t size)
{
    (void)atomic_fetch_add_explicit(&g_stat_realloc, 1UL, memory_order_relaxed);
    if (ptr == NULL)
    {
        return my_malloc(size);
    }
    if (size == 0U)
    {
        my_free(ptr);
        return NULL;
    }

    my_span* sp = my_lookup_span(ptr);
    if (my_span_is_small(sp))
    {
        size_t old_sz = sp->object_size;
        if (size <= old_sz)
        {
            return ptr;
        }
        void* n = my_malloc(size);
        if (n == NULL)
        {
            return NULL;
        }
        (void)memcpy(n, ptr, old_sz);
        my_free(ptr);
        return n;
    }
    return my_large_realloc(ptr, size);
}

void allocator_stats(void)
{
    fprintf(stderr, "allocator stats: malloc=%lu free=%lu calloc=%lu realloc=%lu\n",
            atomic_load_explicit(&g_stat_malloc, memory_order_relaxed),
            atomic_load_explicit(&g_stat_free, memory_order_relaxed),
            atomic_load_explicit(&g_stat_calloc, memory_order_relaxed),
            atomic_load_explicit(&g_stat_realloc, memory_order_relaxed));
}
