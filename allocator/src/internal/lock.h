#ifndef ALLOC_LOCK_H
#define ALLOC_LOCK_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef SRWLOCK my_mutex_t;
static inline void my_mutex_init(my_mutex_t* m)
{
    InitializeSRWLock(m);
}
static inline void my_mutex_lock(my_mutex_t* m)
{
    AcquireSRWLockExclusive(m);
}
static inline void my_mutex_unlock(my_mutex_t* m)
{
    ReleaseSRWLockExclusive(m);
}
#else
#include <pthread.h>
typedef pthread_mutex_t my_mutex_t;
static inline void my_mutex_init(my_mutex_t* m)
{
    (void)pthread_mutex_init(m, NULL);
}
static inline void my_mutex_lock(my_mutex_t* m)
{
    (void)pthread_mutex_lock(m);
}
static inline void my_mutex_unlock(my_mutex_t* m)
{
    (void)pthread_mutex_unlock(m);
}
#endif

#endif
