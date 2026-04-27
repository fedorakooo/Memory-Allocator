#ifndef ALLOC_ONCE_H
#define ALLOC_ONCE_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef INIT_ONCE my_once_t;
#define ALLOC_ONCE_INIT INIT_ONCE_STATIC_INIT
typedef BOOL(CALLBACK* my_once_fn)(PINIT_ONCE, PVOID, PVOID*);
static inline void my_once_call(my_once_t* once, my_once_fn fn, void* param)
{
    (void)InitOnceExecuteOnce(once, fn, param, NULL);
}
#else
#include <pthread.h>
typedef pthread_once_t my_once_t;
#define ALLOC_ONCE_INIT PTHREAD_ONCE_INIT
typedef void (*my_once_fn)(void);
static inline void my_once_call(my_once_t* once, my_once_fn fn, void* param)
{
    (void)param;
    (void)pthread_once(once, fn);
}
#endif

#endif
