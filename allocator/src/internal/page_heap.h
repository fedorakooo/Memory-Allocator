#ifndef PAGE_HEAP_H
#define PAGE_HEAP_H

#include <stddef.h>

typedef struct my_span my_span;

my_span* page_heap_alloc_span(size_t npages);

void page_heap_free_span(my_span* sp);

size_t page_heap_page_size(void);

#endif
