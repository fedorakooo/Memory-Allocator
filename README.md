# Memory-Allocator

Dynamic memory allocator in **C11** with strict warnings (`-Wall -Wextra -Werror`).

This project provides a small `malloc`-like API (`my_malloc`, `my_free`, `my_calloc`, `my_realloc`) plus `allocator_stats()`. Internally it uses **size classes** with a **per-thread cache** for small allocations and **direct OS mappings** for large allocations.

## Public API

Declared in [`allocator/include/allocator.h`](allocator/include/allocator.h):

- `void* my_malloc(size_t size)`
- `void  my_free(void* ptr)`
- `void* my_calloc(size_t nmemb, size_t size)`
- `void* my_realloc(void* ptr, size_t size)`
- `void  allocator_stats(void)`

## High-level design

### Small vs large allocations

The entry points are implemented in [`allocator/src/api/alloc_api.c`](allocator/src/api/alloc_api.c):

- **Small allocations**: if `size <= ALLOC_MAX_SMALL` (64 KiB) and the size maps to a valid class (`my_size_to_class()`), allocation is served from the **thread-local cache** (`tcache_alloc()`), which refills from the **central allocator** as needed.
- **Large allocations**: everything else goes to `my_large_malloc()` / `my_large_free()` / `my_large_realloc()`, which use a dedicated OS mapping per allocation.

### Architecture diagram

```mermaid
flowchart LR
  subgraph api [Public_API]
    myMalloc[my_malloc]
    myFree[my_free]
    myCalloc[my_calloc]
    myRealloc[my_realloc]
    myStats[allocator_stats]
  end

  subgraph small [Small_allocations_(<=64KiB)]
    tcache[tcache_(thread_local)]
    central[central_(per_class_lock)]
    pageHeap[page_heap_(mmap/VirtualAlloc)]
    pagemap[pagemap_(page->span_hash)]
  end

  subgraph large [Large_allocations]
    largeMap[large_(mmap/VirtualAlloc)]
  end

  myMalloc --> tcache
  myFree --> tcache
  myRealloc --> tcache

  tcache --> central
  central --> pageHeap
  pageHeap --> pagemap

  myMalloc --> largeMap
  myFree --> largeMap
  myRealloc --> largeMap
```

### Size classes

Size class logic lives in:

- [`allocator/src/internal/size_classes.h`](allocator/src/internal/size_classes.h)
- [`allocator/src/core/size_classes.c`](allocator/src/core/size_classes.c)

Key constants/behavior:

- **Max “small” request**: `ALLOC_MAX_SMALL = 64 * 1024` bytes.
- **Class count**: `ALLOC_CLASS_COUNT = 100`.
- **Alignment**: `my_alignment()` returns **16 bytes** on 64-bit targets and **8 bytes** on 32-bit targets.
- **Refill batch size**: `my_class_batch_count()` currently returns **32** for all classes.

### Thread-local cache (tcache)

Implemented in [`allocator/src/core/tcache.c`](allocator/src/core/tcache.c):

- Uses `_Thread_local` state with one singly-linked free list per size class.
- On cache miss, refills from central in a batch and returns one object to the caller.
- Keeps an approximate per-thread cache byte counter and applies a **cap of 4 MiB**; when exceeded, it releases a batch back to central.

### Central allocator and spans

Implemented in [`allocator/src/core/central.c`](allocator/src/core/central.c):

- Maintains a **per-size-class mutex** and a list of non-full spans.
- When it needs more memory for a class, it allocates a **span** via `page_heap_alloc_span()`, stores `my_span` metadata at the start of the span, and carves fixed-size objects from the remaining span bytes.
- Span sizing policy:
  - Targets **64 KiB** spans by default.
  - Uses **256 KiB** target spans when `object_size > 4096` bytes.
  - Always rounds span sizes up to the OS page size.

### Pointer → span mapping (pagemap)

Implemented in [`allocator/src/core/pagemap.c`](allocator/src/core/pagemap.c):

- A global hash table maps each OS page base address in a span to the owning `my_span*`.
- Capacity is currently fixed at \(2^{20}\) entries.
- Protected by a mutex and initialized once; cleaned up at process exit.

This pagemap is how `my_free()` decides whether a pointer is part of a small-object span (tcache/central path) or not.

### Large allocations

Implemented in [`allocator/src/core/large.c`](allocator/src/core/large.c):

- Each large allocation is a separate mapping using:
  - `mmap(MAP_PRIVATE|MAP_ANONYMOUS)` on POSIX
  - `VirtualAlloc(MEM_COMMIT|MEM_RESERVE)` on Windows
- A header (`large_hdr`) stores a magic value plus `mapped` and `requested` sizes.
- `my_large_free()` validates the magic and then unmaps the region.

## Semantics notes

- **`my_malloc(0)`** returns `NULL`.
- **`my_free(NULL)`** is a no-op.
- **`my_calloc(nmemb, size)`** checks for multiplication overflow; on overflow it returns `NULL` and sets `errno = ENOMEM`.
- **`my_realloc(NULL, size)`** behaves like `my_malloc(size)`; **`my_realloc(ptr, 0)`** frees and returns `NULL`.

## Thread-safety model

The implementation is intended to be usable from multiple threads:

- **Fast path** for small allocations is mostly thread-local (`_Thread_local` tcache).
- **Central allocator** uses **mutexes** (per size class).
- **Pagemap** uses a **global mutex** for pointer → span lookups/updates.
- API call counters in `allocator_stats()` are stored in **atomics** (see [`allocator/src/api/alloc_api.c`](allocator/src/api/alloc_api.c)).

## Build & run (Makefile)

From the repository root:

```bash
make
./example_usage
```

Run tests:

```bash
make test
```

Run perf micro-benchmark only:

```bash
make perf
```

Clean build artifacts:

```bash
make clean
```

## Tests

The `Makefile` builds and/or runs these binaries under `tests/`:

- `tests/test_basic`: API semantics + alignment + calloc zeroing + large allocation path + overflow cases.
- `tests/test_stress`: randomized allocate/free/realloc loop + prints `allocator_stats()` at the end.
- `tests/test_realloc_preserve`: checks that realloc preserves the old prefix across growth up to 128 KiB.
- `tests/test_perf`: simple wall-clock comparison vs libc for `malloc/free` in a tight loop.
- `tests/test_mt`: multi-threaded randomized workload (uses `-pthread`).

## Repo layout

- `allocator/include/`: public header (`allocator.h`)
- `allocator/src/api/`: public API entry points
- `allocator/src/core/`: tcache, central allocator, pagemap, size classes, large allocations
- `allocator/src/internal/`: internal headers (span metadata, locks, helpers)
- `allocator/src/platform/`: OS mapping + page-size helpers
- `examples/`: minimal usage example
- `tests/`: test programs

## Limitations / notes

- **No span reclamation to central/page heap**: spans are allocated on demand, but `central.c` does not currently return completely free spans back to the OS.
- **Fixed-size pagemap**: `pagemap.c` uses a fixed-capacity table allocated with the host `calloc/free`.
- **Invalid free behavior**: if a pointer does not resolve to a known small span and is not a valid large allocation header, `my_free()` does nothing (it does not attempt to diagnose arbitrary pointers).
- **Large allocation overhead/alignment**: large allocations are rounded up to the OS page size and include `large_hdr` overhead.

## License

Coursework / educational use.
