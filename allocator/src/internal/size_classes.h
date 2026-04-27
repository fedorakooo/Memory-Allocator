#ifndef SIZE_CLASSES_H
#define SIZE_CLASSES_H

#include <stddef.h>
#include <stdint.h>

#define ALLOC_MAX_SMALL ((size_t)64U * 1024U)

#define ALLOC_CLASS_COUNT 100U

int my_size_to_class(size_t size);

size_t my_class_to_size(unsigned class_idx);

size_t my_alignment(void);

unsigned my_class_batch_count(unsigned class_idx);

#endif
