#include "size_classes.h"

#include "once.h"
#include "utils.h"

#include <stddef.h>

static my_once_t g_sizes_once = ALLOC_ONCE_INIT;
static size_t g_class_sizes[ALLOC_CLASS_COUNT];

static void init_class_sizes(void)
{
    const size_t align = (sizeof(void*) >= 8U) ? 16U : 8U;
    const size_t count_minus_1 = (size_t)(ALLOC_CLASS_COUNT - 1U);

    for (size_t i = 0U; i < (size_t)ALLOC_CLASS_COUNT; i++)
    {
        size_t base = align;
        size_t span = ALLOC_MAX_SMALL - align;
        size_t s = base + ((span * i) / count_minus_1);
        g_class_sizes[i] = util_align_up(s, align);
    }
    g_class_sizes[ALLOC_CLASS_COUNT - 1U] = ALLOC_MAX_SMALL;
}

static void ensure_classes_init(void)
{
    my_once_call(&g_sizes_once, init_class_sizes, NULL);
}

int my_size_to_class(size_t size)
{
    if (size == 0U || size > ALLOC_MAX_SMALL)
    {
        return -1;
    }

    ensure_classes_init();

    size_t lo = 0U;
    size_t hi = (size_t)ALLOC_CLASS_COUNT;
    while (lo < hi)
    {
        size_t mid = lo + ((hi - lo) / 2U);
        if (g_class_sizes[mid] < size)
        {
            lo = mid + 1U;
        }
        else
        {
            hi = mid;
        }
    }
    if (lo >= (size_t)ALLOC_CLASS_COUNT)
    {
        return -1;
    }
    return (int)lo;
}

size_t my_class_to_size(unsigned class_idx)
{
    ensure_classes_init();
    if (class_idx >= ALLOC_CLASS_COUNT)
    {
        return 0U;
    }
    return g_class_sizes[class_idx];
}

size_t my_alignment(void)
{
    return (sizeof(void*) >= 8U) ? 16U : 8U;
}

unsigned my_class_batch_count(unsigned class_idx)
{
    size_t s = my_class_to_size(class_idx);
    if (s == 0U)
    {
        return 0U;
    }
    if (s <= 512U)
    {
        return 32U;
    }
    if (s <= 4096U)
    {
        return 16U;
    }
    if (s <= 16384U)
    {
        return 8U;
    }
    return 4U;
}
