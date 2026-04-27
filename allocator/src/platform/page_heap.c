#include "page_heap.h"

#include "my_internal.h"
#include "utils.h"

#include <errno.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
static size_t win_page_size(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    size_t ps = (size_t)si.dwPageSize;
    return ps == 0U ? 4096U : ps;
}
#else
#include <sys/mman.h>
#include <unistd.h>
static size_t posix_page_size(void)
{
    long ps = sysconf(_SC_PAGESIZE);
    return ps <= 0 ? 4096U : (size_t)ps;
}
#endif

size_t page_heap_page_size(void)
{
#ifdef _WIN32
    return win_page_size();
#else
    return posix_page_size();
#endif
}

static void* os_map(size_t bytes)
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

static void os_unmap(void* p, size_t bytes)
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

my_span* page_heap_alloc_span(size_t npages)
{
    size_t ps = page_heap_page_size();
    if (npages == 0U || ps == 0U)
    {
        errno = EINVAL;
        return NULL;
    }
    size_t bytes = 0U;
    if (util_mul_overflow_size(npages, ps, &bytes) != 0)
    {
        errno = ENOMEM;
        return NULL;
    }

    void* base = os_map(bytes);
    if (base == NULL)
    {
        return NULL;
    }

    my_span* sp = (my_span*)base;
    memset(sp, 0, sizeof(*sp));
    sp->magic = ALLOC_SPAN_MAGIC;
    sp->class_idx = UINT32_MAX;
    sp->span_bytes = bytes;
    sp->object_size = 0U;
    sp->free_list = NULL;
    sp->inuse = 0U;
    sp->next = NULL;
    sp->prev = NULL;

    if (my_register_span_pages(sp, base, bytes) != 0)
    {
        os_unmap(base, bytes);
        return NULL;
    }
    return sp;
}

void page_heap_free_span(my_span* sp)
{
    if (sp == NULL)
    {
        return;
    }
    void* base = (void*)sp;
    size_t bytes = sp->span_bytes;
    my_unregister_span_pages(sp, base, bytes);
    os_unmap(base, bytes);
}
