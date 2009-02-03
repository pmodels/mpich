#ifndef MPIDU_ATOMICS_BY_LOCK_H_INCLUDED
#define MPIDU_ATOMICS_BY_LOCK_H_INCLUDED

/* FIXME For now we rely on pthreads for our IPC locks.  This is fairly
   portable, although it is obviously not 100% portable.  Some day when we
   refactor the MPIDU_Process_locks code we should be able to use that again. */
#if defined(HAVE_PTHREAD_H)
#include <pthread.h>

/* defined in mpidu_atomic_primitives.c */
pthread_mutex_t *MPIDU_Atomic_emulation_lock;

/*
    These macros are analogous to the MPIDU_THREAD_XXX_CS_{ENTER,EXIT} macros.
    TODO Consider putting debugging macros in here that utilize 'msg'.
*/
#define MPIDU_IPC_SINGLE_CS_ENTER(msg)          \
    do {                                        \
        MPIDU_Atomic_assert(MPIDU_Atomic_emulation_lock);    \
        pthread_mutex_lock(MPIDU_Atomic_emulation_lock);     \
    } while (0)

#define MPIDU_IPC_SINGLE_CS_EXIT(msg)           \
    do {                                        \
        MPIDU_Atomic_assert(MPIDU_Atomic_emulation_lock);    \
        pthread_mutex_unlock(MPIDU_Atomic_emulation_lock);   \
    } while (0)

typedef struct { volatile int v;  } MPIDU_Atomic_t;
typedef struct { int * volatile v; } MPIDU_Atomic_ptr_t;

/*
    Emulated atomic primitives
    --------------------------

    These are versions of the atomic primitives that emulate the proper behavior
    via the use of an inter-process lock.  For more information on their
    individual behavior, please see the comment on the corresponding top level
    function.

    In general, these emulated primitives should _not_ be used.  Most algorithms
    can be more efficiently implemented by putting most or all of the algorithm
    inside of a single critical section.  These emulated primitives exist to
    ensure that there is always a fallback if no machine-dependent version of a
    particular operation has been defined.  They also serve as a very readable
    reference for the exact semantics of our MPIDU_Atomic ops.
*/

/* We don't actually implement CAS natively, but we do support all the non-LL/SC
   primitives including CAS. */
#define MPIDU_ATOMIC_UNIVERSAL_PRIMITIVE MPIDU_ATOMIC_CAS

static inline int MPIDU_Atomic_load(MPIDU_Atomic_t *ptr)
{
    int retval;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_add");
    retval = ptr->v;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_add");
    return retval;
}

static inline void MPIDU_Atomic_store(MPIDU_Atomic_t *ptr, int val)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_add");
    ptr->v = val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_add");
}

static inline void *MPIDU_Atomic_load_ptr(MPIDU_Atomic_ptr_t *ptr)
{
    int *retval;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_add");
    retval = ptr->v;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_add");
    return retval;
}

static inline void MPIDU_Atomic_store_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_add");
    ptr->v = val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_add");
}


static inline void MPIDU_Atomic_add(MPIDU_Atomic_t *ptr, int val)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_add");
    ptr->v += val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_add");
}

static inline void *MPIDU_Atomic_cas_ptr(MPIDU_Atomic_ptr_t *ptr, int *oldv, int *newv)
{
    int *prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_cas");
    prev = ptr->v;
    if (prev == oldv) {
        ptr->v = newv;
    }
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_cas");
    return prev;
}

static inline int MPIDU_Atomic_cas_int(MPIDU_Atomic_t *ptr, int oldv, int newv)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_cas");
    prev = ptr->v;
    if (prev == oldv) {
        ptr->v = newv;
    }
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_cas");
    return prev;
}

static inline int MPIDU_Atomic_decr_and_test(MPIDU_Atomic_t *ptr)
{
    int new_val;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_decr_and_test");
    new_val = --(ptr->v);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_decr_and_test");
    return (0 == new_val);
}

static inline void MPIDU_Atomic_decr(MPIDU_Atomic_t *ptr)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_decr");
    --(ptr->v);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_decr");
}

static inline int MPIDU_Atomic_fetch_and_add(MPIDU_Atomic_t *ptr, int val)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_fetch_and_add");
    prev = ptr->v;
    ptr->v += val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_fetch_and_add");
    return prev;
}

static inline int MPIDU_Atomic_fetch_and_decr(MPIDU_Atomic_t *ptr)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_fetch_and_decr");
    prev = ptr->v;
    --(ptr->v);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_fetch_and_decr");
    return prev;
}

static inline int MPIDU_Atomic_fetch_and_incr(MPIDU_Atomic_t *ptr)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_fetch_and_incr");
    prev = ptr->v;
    ++(ptr->v);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_fetch_and_incr");
    return prev;
}

static inline void MPIDU_Atomic_incr(MPIDU_Atomic_t *ptr)
{
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_incr");
    ++(ptr->v);
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_incr");
}

static inline void *MPIDU_Atomic_swap_ptr(MPIDU_Atomic_ptr_t *ptr, void *val)
{
    int *prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_swap_ptr");
    prev = ptr->v;
    ptr->v = val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_swap_ptr");
    return prev;
}

static inline int MPIDU_Atomic_swap_int(MPIDU_Atomic_t *ptr, int val)
{
    int prev;
    MPIDU_IPC_SINGLE_CS_ENTER("atomic_swap_int");
    prev = ptr->v;
    ptr->v = val;
    MPIDU_IPC_SINGLE_CS_EXIT("atomic_swap_int");
    return (int)prev;
}

#endif /* defined(HAVE_PTHREAD_H) */
#endif /* !defined(MPIDU_ATOMICS_BY_LOCK_H_INCLUDED) */
