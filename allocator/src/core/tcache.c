#include "tcache.h"

#include "central.h"
#include "my_internal.h"
#include "size_classes.h"

#include <stddef.h>

typedef struct tcache_state
{
    my_obj* free_lists[ALLOC_CLASS_COUNT];
    unsigned counts[ALLOC_CLASS_COUNT];
    size_t cached_bytes;
} tcache_state;

static _Thread_local tcache_state g_tc;

static void tcache_flush_class(unsigned class_idx, unsigned n)
{
    if (class_idx >= ALLOC_CLASS_COUNT || n == 0U)
    {
        return;
    }
    my_obj* head = NULL;
    my_obj* tail = NULL;
    unsigned moved = 0U;

    while (moved < n && g_tc.free_lists[class_idx] != NULL)
    {
        my_obj* obj = g_tc.free_lists[class_idx];
        g_tc.free_lists[class_idx] = obj->next;
        obj->next = NULL;
        if (head == NULL)
        {
            head = obj;
            tail = obj;
        }
        else
        {
            tail->next = obj;
            tail = obj;
        }
        moved++;
    }

    if (moved == 0U)
    {
        return;
    }

    g_tc.counts[class_idx] -= moved;
    g_tc.cached_bytes -= (size_t)moved * my_class_to_size(class_idx);
    central_release(class_idx, head, moved);
}

void* tcache_alloc(size_t size)
{
    int cls = my_size_to_class(size);
    if (cls < 0)
    {
        return NULL;
    }
    unsigned class_idx = (unsigned)cls;

    if (g_tc.free_lists[class_idx] == NULL)
    {
        unsigned batch = my_class_batch_count(class_idx);
        my_obj* batch_head = (my_obj*)central_refill(class_idx, batch);
        if (batch_head == NULL)
        {
            return NULL;
        }

        my_obj* first = batch_head;
        batch_head = batch_head->next;
        first->next = NULL;

        unsigned inserted = 0U;
        while (batch_head != NULL)
        {
            my_obj* next = batch_head->next;
            batch_head->next = g_tc.free_lists[class_idx];
            g_tc.free_lists[class_idx] = batch_head;
            inserted++;
            batch_head = next;
        }
        g_tc.counts[class_idx] += inserted;
        g_tc.cached_bytes += (size_t)inserted * my_class_to_size(class_idx);
        return (void*)first;
    }

    my_obj* obj = g_tc.free_lists[class_idx];
    g_tc.free_lists[class_idx] = obj->next;
    obj->next = NULL;
    g_tc.counts[class_idx]--;
    g_tc.cached_bytes -= my_class_to_size(class_idx);
    return (void*)obj;
}

void tcache_free(void* p)
{
    if (p == NULL)
    {
        return;
    }

    my_span* sp = my_lookup_span(p);
    if (!my_span_is_small(sp))
    {
        return;
    }
    unsigned class_idx = sp->class_idx;
    if (class_idx >= ALLOC_CLASS_COUNT)
    {
        return;
    }

    my_obj* obj = (my_obj*)p;
    obj->next = g_tc.free_lists[class_idx];
    g_tc.free_lists[class_idx] = obj;
    g_tc.counts[class_idx]++;
    g_tc.cached_bytes += my_class_to_size(class_idx);

    unsigned batch = my_class_batch_count(class_idx);
    const size_t kTcacheCapBytes = 4U * 1024U * 1024U;
    if (g_tc.cached_bytes > kTcacheCapBytes || g_tc.counts[class_idx] > (batch * 2U))
    {
        tcache_flush_class(class_idx, batch);
    }
}
