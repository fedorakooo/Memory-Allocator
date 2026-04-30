#include "central.h"

#include "my_internal.h"
#include "once.h"
#include "page_heap.h"
#include "size_classes.h"
#include "utils.h"

#include <stdint.h>

typedef struct central_bin
{
    my_mutex_t mu;
    my_span* partial;
} central_bin;

static central_bin g_bins[ALLOC_CLASS_COUNT];
static my_once_t g_bins_once = ALLOC_ONCE_INIT;

static void bins_init(void)
{
    for (unsigned i = 0U; i < ALLOC_CLASS_COUNT; i++)
    {
        my_mutex_init(&g_bins[i].mu);
        g_bins[i].partial = NULL;
    }
}

static void ensure_bins_init(void)
{
    my_once_call(&g_bins_once, bins_init, NULL);
}

static void list_unlink(my_span** head, my_span* sp)
{
    if (sp->prev != NULL)
    {
        sp->prev->next = sp->next;
    }
    else
    {
        *head = sp->next;
    }
    if (sp->next != NULL)
    {
        sp->next->prev = sp->prev;
    }
    sp->next = NULL;
    sp->prev = NULL;
}

static void list_push_front(my_span** head, my_span* sp)
{
    sp->prev = NULL;
    sp->next = *head;
    if (*head != NULL)
    {
        (*head)->prev = sp;
    }
    *head = sp;
}

static int span_is_on_list(my_span* head, my_span* sp)
{
    for (my_span* cur = head; cur != NULL; cur = cur->next)
    {
        if (cur == sp)
        {
            return 1;
        }
    }
    return 0;
}

static void span_build_freelist(my_span* sp, size_t object_size)
{
    unsigned char* data = (unsigned char*)util_align_ptr_up(
        (void*)((unsigned char*)sp + sizeof(*sp)), my_alignment());
    size_t offset = (size_t)(data - (unsigned char*)sp);
    size_t usable = (offset < sp->span_bytes) ? (sp->span_bytes - offset) : 0U;
    size_t nobj = (object_size == 0U) ? 0U : (usable / object_size);

    sp->free_list = NULL;
    sp->inuse = 0U;
    for (size_t i = 0U; i < nobj; i++)
    {
        my_obj* obj = (my_obj*)(data + i * object_size);
        obj->next = sp->free_list;
        sp->free_list = obj;
    }
}

static my_span* alloc_class_span(unsigned class_idx)
{
    size_t ps = page_heap_page_size();
    if (ps == 0U)
    {
        return NULL;
    }

    size_t object_size = my_class_to_size(class_idx);
    if (object_size < sizeof(my_obj))
    {
        object_size = sizeof(my_obj);
    }

    size_t target = (object_size > 4096U) ? (256U * 1024U) : (64U * 1024U);
    size_t min_need = sizeof(my_span) + object_size;
    if (target < min_need)
    {
        target = min_need;
    }

    size_t npages = util_align_up(target, ps) / ps;
    if (npages == 0U)
    {
        npages = 1U;
    }

    my_span* sp = page_heap_alloc_span(npages);
    if (sp == NULL)
    {
        return NULL;
    }

    sp->magic = ALLOC_SPAN_MAGIC;
    sp->class_idx = class_idx;
    sp->object_size = object_size;
    sp->next = NULL;
    sp->prev = NULL;
    span_build_freelist(sp, object_size);

    if (sp->free_list == NULL)
    {
        page_heap_free_span(sp);
        return NULL;
    }
    return sp;
}

void* central_refill(unsigned class_idx, unsigned batch_count)
{
    ensure_bins_init();
    if (class_idx >= ALLOC_CLASS_COUNT || batch_count == 0U)
    {
        return NULL;
    }

    central_bin* b = &g_bins[class_idx];
    my_mutex_lock(&b->mu);

    if (b->partial == NULL)
    {
        my_span* fresh = alloc_class_span(class_idx);
        if (fresh != NULL)
        {
            list_push_front(&b->partial, fresh);
        }
    }

    my_obj* head = NULL;
    my_obj* tail = NULL;
    unsigned taken = 0U;
    while (taken < batch_count && b->partial != NULL)
    {
        my_span* sp = b->partial;
        if (sp->free_list == NULL)
        {
            list_unlink(&b->partial, sp);
            continue;
        }

        my_obj* obj = sp->free_list;
        sp->free_list = obj->next;
        obj->next = NULL;
        sp->inuse++;
        taken++;

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

        if (sp->free_list == NULL)
        {
            list_unlink(&b->partial, sp);
        }
    }

    my_mutex_unlock(&b->mu);
    return (void*)head;
}

void central_release(unsigned class_idx, void* batch_head, unsigned batch_count)
{
    ensure_bins_init();
    if (class_idx >= ALLOC_CLASS_COUNT || batch_head == NULL || batch_count == 0U)
    {
        return;
    }

    central_bin* b = &g_bins[class_idx];
    my_mutex_lock(&b->mu);

    my_obj* cur = (my_obj*)batch_head;
    for (unsigned i = 0U; i < batch_count && cur != NULL; i++)
    {
        my_obj* next = cur->next;
        my_span* sp = my_lookup_span(cur);
        if (my_span_is_class(sp, class_idx))
        {
            int was_full = (sp->free_list == NULL);
            cur->next = sp->free_list;
            sp->free_list = cur;
            if (sp->inuse > 0U)
            {
                sp->inuse--;
            }

            if (was_full && !span_is_on_list(b->partial, sp))
            {
                list_push_front(&b->partial, sp);
            }
        }
        cur = next;
    }

    my_mutex_unlock(&b->mu);
}
