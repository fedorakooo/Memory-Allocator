#include "my_internal.h"

#include "once.h"
#include "page_heap.h"
#include "utils.h"

#include <stdint.h>
#include <stdlib.h>

#define PAGEMAP_CAPACITY ((size_t)1U << 20U)

typedef struct pagemap
{
    my_mutex_t mu;
    uintptr_t* keys;
    my_span** vals;
    unsigned char* state;
    size_t cap;
} pagemap;

static my_once_t g_pagemap_once = ALLOC_ONCE_INIT;
static pagemap g_pagemap;

static size_t pagemap_hash(uintptr_t k)
{
    k ^= k >> 33U;
    k *= (uintptr_t)0xff51afd7ed558ccdULL;
    k ^= k >> 33U;
    return (size_t)k;
}

static void pagemap_init(void)
{
    g_pagemap.cap = PAGEMAP_CAPACITY;
    my_mutex_init(&g_pagemap.mu);
    g_pagemap.keys = (uintptr_t*)calloc(g_pagemap.cap, sizeof(uintptr_t));
    g_pagemap.vals = (my_span**)calloc(g_pagemap.cap, sizeof(my_span*));
    g_pagemap.state = (unsigned char*)calloc(g_pagemap.cap, sizeof(unsigned char));
}

static void ensure_pagemap_init(void)
{
    my_once_call(&g_pagemap_once, pagemap_init, NULL);
}

static my_span* pagemap_lookup_locked(uintptr_t key)
{
    size_t cap = g_pagemap.cap;
    if (cap == 0U || g_pagemap.state == NULL)
    {
        return NULL;
    }
    size_t i = pagemap_hash(key) % cap;
    for (size_t probes = 0U; probes < cap; probes++)
    {
        if (g_pagemap.state[i] == 0U)
        {
            return NULL;
        }
        if (g_pagemap.state[i] == 1U && g_pagemap.keys[i] == key)
        {
            return g_pagemap.vals[i];
        }
        i = (i + 1U) % cap;
    }
    return NULL;
}

static int pagemap_insert_locked(uintptr_t key, my_span* sp)
{
    size_t cap = g_pagemap.cap;
    if (cap == 0U || g_pagemap.state == NULL)
    {
        return -1;
    }

    size_t i = pagemap_hash(key) % cap;
    size_t first_deleted = cap;
    for (size_t probes = 0U; probes < cap; probes++)
    {
        if (g_pagemap.state[i] == 0U)
        {
            size_t slot = (first_deleted == cap) ? i : first_deleted;
            g_pagemap.state[slot] = 1U;
            g_pagemap.keys[slot] = key;
            g_pagemap.vals[slot] = sp;
            return 0;
        }
        if (g_pagemap.state[i] == 2U && first_deleted == cap)
        {
            first_deleted = i;
        }
        else if (g_pagemap.state[i] == 1U && g_pagemap.keys[i] == key)
        {
            g_pagemap.vals[i] = sp;
            return 0;
        }
        i = (i + 1U) % cap;
    }
    return -1;
}

static void pagemap_remove_locked(uintptr_t key)
{
    size_t cap = g_pagemap.cap;
    if (cap == 0U || g_pagemap.state == NULL)
    {
        return;
    }
    size_t i = pagemap_hash(key) % cap;
    for (size_t probes = 0U; probes < cap; probes++)
    {
        if (g_pagemap.state[i] == 0U)
        {
            return;
        }
        if (g_pagemap.state[i] == 1U && g_pagemap.keys[i] == key)
        {
            g_pagemap.state[i] = 2U;
            g_pagemap.keys[i] = 0U;
            g_pagemap.vals[i] = NULL;
            return;
        }
        i = (i + 1U) % cap;
    }
}

my_span* my_lookup_span(void* p)
{
    if (p == NULL)
    {
        return NULL;
    }

    ensure_pagemap_init();
    size_t ps = page_heap_page_size();
    if (ps == 0U)
    {
        return NULL;
    }
    uintptr_t key = ((uintptr_t)p / (uintptr_t)ps) * (uintptr_t)ps;

    my_mutex_lock(&g_pagemap.mu);
    my_span* sp = pagemap_lookup_locked(key);
    my_mutex_unlock(&g_pagemap.mu);
    return sp;
}

int my_register_span_pages(my_span* sp, void* base, size_t span_bytes)
{
    if (sp == NULL || base == NULL || span_bytes == 0U)
    {
        return -1;
    }
    ensure_pagemap_init();
    size_t ps = page_heap_page_size();
    if (ps == 0U || (span_bytes % ps) != 0U)
    {
        return -1;
    }

    uintptr_t b = (uintptr_t)base;
    size_t npages = span_bytes / ps;

    my_mutex_lock(&g_pagemap.mu);
    for (size_t i = 0U; i < npages; i++)
    {
        uintptr_t key = b + (uintptr_t)(i * ps);
        if (pagemap_insert_locked(key, sp) != 0)
        {
            for (size_t j = 0U; j < i; j++)
            {
                uintptr_t rollback = b + (uintptr_t)(j * ps);
                pagemap_remove_locked(rollback);
            }
            my_mutex_unlock(&g_pagemap.mu);
            return -1;
        }
    }
    my_mutex_unlock(&g_pagemap.mu);
    return 0;
}

void my_unregister_span_pages(my_span* sp, void* base, size_t span_bytes)
{
    (void)sp;
    if (base == NULL || span_bytes == 0U)
    {
        return;
    }
    ensure_pagemap_init();
    size_t ps = page_heap_page_size();
    if (ps == 0U || (span_bytes % ps) != 0U)
    {
        return;
    }

    uintptr_t b = (uintptr_t)base;
    size_t npages = span_bytes / ps;

    my_mutex_lock(&g_pagemap.mu);
    for (size_t i = 0U; i < npages; i++)
    {
        uintptr_t key = b + (uintptr_t)(i * ps);
        pagemap_remove_locked(key);
    }
    my_mutex_unlock(&g_pagemap.mu);
}
