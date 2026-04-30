#include "large.h"

#include "my_internal.h"
#include "page_heap.h"
#include "utils.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

typedef struct large_hdr
{
    uint32_t magic;
    uint32_t reserved;
    size_t mapped_bytes;
    size_t requested_bytes;
} large_hdr;

static void* large_os_map(size_t bytes)
{
#ifdef _WIN32
    return VirtualAlloc(NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    void* p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED)
    {
        return NULL;
    }
    return p;
#endif
}

static void large_os_unmap(void* p, size_t bytes)
{
#ifdef _WIN32
    (void)bytes;
    if (p != NULL)
    {
        (void)VirtualFree(p, 0, MEM_RELEASE);
    }
#else
    if (p != NULL && bytes != 0U)
    {
        (void)munmap(p, bytes);
    }
#endif
}

void* my_large_malloc(size_t size)
{
    if (size == 0U)
    {
        return NULL;
    }

    size_t with_hdr = 0U;
    if (util_add_overflow_size(sizeof(large_hdr), size, &with_hdr) != 0)
    {
        return NULL;
    }

    size_t ps = page_heap_page_size();
    if (ps == 0U)
    {
        return NULL;
    }
    size_t mapped = util_align_up(with_hdr, ps);
    if (mapped < with_hdr)
    {
        return NULL;
    }

    large_hdr* h = (large_hdr*)large_os_map(mapped);
    if (h == NULL)
    {
        return NULL;
    }

    h->magic = ALLOC_LARGE_MAGIC;
    h->reserved = 0U;
    h->mapped_bytes = mapped;
    h->requested_bytes = size;
    return (void*)(h + 1);
}

void my_large_free(void* p)
{
    if (p == NULL)
    {
        return;
    }

    large_hdr* h = ((large_hdr*)p) - 1;
    if (h->magic != ALLOC_LARGE_MAGIC)
    {
        return;
    }
    size_t mapped = h->mapped_bytes;
    h->magic = 0U;
    large_os_unmap((void*)h, mapped);
}

void* my_large_realloc(void* p, size_t size)
{
    if (p == NULL)
    {
        return my_large_malloc(size);
    }
    if (size == 0U)
    {
        my_large_free(p);
        return NULL;
    }

    large_hdr* h = ((large_hdr*)p) - 1;
    if (h->magic != ALLOC_LARGE_MAGIC)
    {
        return NULL;
    }

    if (size <= h->requested_bytes)
    {
        h->requested_bytes = size;
        return p;
    }

    void* np = my_large_malloc(size);
    if (np == NULL)
    {
        return NULL;
    }
    (void)memcpy(np, p, h->requested_bytes);
    my_large_free(p);
    return np;
}
