CC ?= cc
AR ?= ar

OPT ?= -O2
EXTRA_SAN ?=

CPPFLAGS := -Iallocator/include -Iallocator/src/internal
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic $(OPT) $(EXTRA_SAN)
LDFLAGS := $(EXTRA_SAN)

OBJ_DIR := build

ALLOC_SRCS := \
	allocator/src/api/alloc_api.c \
	allocator/src/core/central.c \
	allocator/src/core/large.c \
	allocator/src/core/pagemap.c \
	allocator/src/core/size_classes.c \
	allocator/src/core/tcache.c \
	allocator/src/core/utils.c \
	allocator/src/platform/page_heap.c

ALLOC_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(ALLOC_SRCS))
LIBALLOC := liballocator.a

TEST_BINS := test_basic test_stress test_realloc_preserve test_perf test_mt

.PHONY: all clean example tests test perf debug release asan tsan ubsan

all: $(LIBALLOC) example_usage $(TEST_BINS)

$(LIBALLOC): $(ALLOC_OBJS)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@mkdir -p "$(dir $@)"
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

example_usage: examples/example_usage.c $(LIBALLOC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBALLOC) $(LDFLAGS) -o $@

test_basic: tests/test_basic.c $(LIBALLOC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBALLOC) $(LDFLAGS) -o $@

test_stress: tests/test_stress.c $(LIBALLOC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBALLOC) $(LDFLAGS) -o $@

test_realloc_preserve: tests/test_realloc_preserve.c $(LIBALLOC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBALLOC) $(LDFLAGS) -o $@

test_perf: tests/test_perf.c $(LIBALLOC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBALLOC) $(LDFLAGS) -o $@

test_mt: tests/test_mt.c $(LIBALLOC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LIBALLOC) $(LDFLAGS) -pthread -o $@

tests: $(TEST_BINS)

test: $(TEST_BINS)
	./test_basic
	./test_stress
	./test_realloc_preserve
	./test_mt

perf: test_perf
	./test_perf

debug:
	$(MAKE) clean all OPT="-O0 -g3"

release:
	$(MAKE) clean all OPT="-O2"

asan:
	$(MAKE) clean all OPT="-O1 -g3" EXTRA_SAN="-fsanitize=address,undefined -fno-omit-frame-pointer"

tsan:
	$(MAKE) clean all OPT="-O1 -g3" EXTRA_SAN="-fsanitize=thread -fno-omit-frame-pointer"

ubsan:
	$(MAKE) clean all OPT="-O1 -g3" EXTRA_SAN="-fsanitize=undefined -fno-omit-frame-pointer"

clean:
	rm -rf "$(OBJ_DIR)" "$(LIBALLOC)" example_usage $(TEST_BINS)
