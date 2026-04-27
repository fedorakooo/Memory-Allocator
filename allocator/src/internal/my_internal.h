#ifndef MY_INTERNAL_H
#define MY_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "lock.h"

#define ALLOC_SPAN_MAGIC ((uint32_t)0xA11C0C5Au)
#define ALLOC_LARGE_MAGIC ((uint32_t)0xA11C1A92u)

typedef struct my_obj
{
    struct my_obj* next;
} my_obj;

typedef struct my_span
{
    uint32_t magic;
    uint32_t class_idx;
    size_t span_bytes;
    size_t object_size;
    my_obj* free_list;
    size_t inuse;
    struct my_span* next;
    struct my_span* prev;
} my_span;

static inline int my_span_is_small(const my_span* sp)
{
    return sp != NULL && sp->magic == ALLOC_SPAN_MAGIC && sp->class_idx != UINT32_MAX;
}

static inline int my_span_is_class(const my_span* sp, unsigned class_idx)
{
    return sp != NULL && sp->magic == ALLOC_SPAN_MAGIC && sp->class_idx == class_idx;
}

my_span* my_lookup_span(void* p);
int my_register_span_pages(my_span* sp, void* base, size_t span_bytes);
void my_unregister_span_pages(my_span* sp, void* base, size_t span_bytes);

void* my_small_malloc(size_t size);
void my_small_free(void* p);

void* my_large_malloc(size_t size);
void my_large_free(void* p);
void* my_large_realloc(void* p, size_t size);

#endif
