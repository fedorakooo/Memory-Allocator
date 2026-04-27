#ifndef CENTRAL_H
#define CENTRAL_H

#include <stddef.h>

void* central_refill(unsigned class_idx, unsigned batch_count);

void central_release(unsigned class_idx, void* batch_head, unsigned batch_count);

#endif
