/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPA_TEST_NAIVE
#  include "opa_primitives.h"
#else /* OPA_TEST_NAIVE */
#  define OPA_PRIMITIVES_H_INCLUDED
#  include "opa_config.h"
#  include "opa_util.h"
#  ifndef _opa_inline
#    define _opa_inline inline
#  endif
#endif /* OPA_TEST_NAIVE */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#if defined(OPA_HAVE_PTHREAD_H)
#  include <pthread.h>
#endif /* HAVE_PTHREAD_H */

/* Define the macro to use for yielding the current thread (to others) */
#if defined(OPA_HAVE_PTHREAD_YIELD)
#  define OPA_TEST_YIELD() pthread_yield()
#elif defined(OPA_HAVE_SCHED_YIELD)
#  include <sched.h>
#  define OPA_TEST_YIELD() (void) sched_yield()
#else
#  define OPA_TEST_YIELD() (void) 0
#endif

/*
 * Naive redefinition of OPA functions as simple C operations.  These should
 * pass all the "simple" tests, but fail on the threaded tests (with >1 thread),
 * unless the tested operation is natively atomic.
 */
#ifdef OPA_TEST_NAIVE

#define OPA_UNIVERSAL_PRIMITIVE OPA_CAS
#define OPA_LL_SC_SUPPORTED 1

typedef volatile int OPA_int_t;
typedef void * volatile OPA_ptr_t;

#define OPA_load_int(A) (*(A))
#define OPA_store_int(A, B) ((void) (*(A) = (B)))
#define OPA_load_ptr(A) (*(A))
#define OPA_store_ptr(A, B) ((void) (*(A) = (B)))

#define OPA_add_int(A, B) ((void) (*(A) += (B)))
#define OPA_incr_int(A) ((void) (++(*(A))))
#define OPA_decr_int(A) ((void) (--(*(A))))

#define OPA_decr_and_test_int(A) (0 == --(*(A)))
static _opa_inline int OPA_fetch_and_add_int(OPA_int_t *ptr, int val)
{
    int prev = *ptr;
    *ptr += val;
    return prev;
}
#define OPA_fetch_and_incr_int(A) ((*(A))++)
#define OPA_fetch_and_decr_int(A) ((*(A))--)

#define OPA_cas_ptr(A, B, C) (*(A) == (B) ? (*(A) = (C), (B)) : *(A))
#define OPA_cas_int(A, B, C) (*(A) == (B) ? (*(A) = (C), (B)) : *(A))

static _opa_inline void *OPA_swap_ptr(OPA_ptr_t *ptr, void *val)
{
    void *prev;
    prev = *ptr;
    *ptr = val;
    return prev;
}

static _opa_inline int OPA_swap_int(OPA_int_t *ptr, int val)
{
    int prev;
    prev = *ptr;
    *ptr = val;
    return prev;
}

#define OPA_write_barrier() ((void) 0)
#define OPA_read_barrier() ((void) 0)
#define OPA_read_write_barrier() ((void) 0)

/* For LL/SC only use load/store.  This is more naive than the above
 * implementation for CAS */
#define OPA_LL_int OPA_load_int
#define OPA_SC_int(A, B) (OPA_store_int(A, B), 1)
#define OPA_LL_ptr OPA_load_ptr
#define OPA_SC_ptr(A, B) (OPA_store_ptr(A, B), 1)

#if defined(OPA_HAVE_SCHED_YIELD)
#  include <sched.h>
#  define OPA_busy_wait() sched_yield()
#else
#  define OPA_busy_wait() do { } while (0)
#endif

#endif /* OPA_TEST_NAIVE */

/*
 * Cache line padding.  See note in opa_queue.h.
 */
#define OPA_TEST_CACHELINE_PADDING 128

/*
 * Print the current location on the standard output stream.
 */
#define AT()            printf ("        at %s:%d in %s()...\n",              \
                                __FILE__, __LINE__, __FUNCTION__)

/*
 * The name of the test is printed by saying TESTING("something") which will
 * result in the string `Testing something' being flushed to standard output.
 * If a test passes, fails, or is skipped then the PASSED(), FAILED(), or
 * SKIPPED() macro should be called.  After HFAILED() or SKIPPED() the caller
 * should print additional information to stdout indented by at least four
 * spaces.
 */
#define TESTING(WHAT, NTHREADS)                                                \
do {                                                                           \
    if(NTHREADS) {                                                             \
        int nwritten_chars;                                                    \
        printf("Testing %s with %d thread%s %n",                               \
                WHAT,                                                          \
                (int) NTHREADS,                                                \
                (NTHREADS) > 1 ? "s" : "",                                     \
                &nwritten_chars);                                              \
        printf("%*s", nwritten_chars > 70 ? 0 : 70 - nwritten_chars, "");      \
    } /* end if */                                                             \
    else                                                                       \
        printf("Testing %-62s", WHAT);                                         \
    fflush(stdout);                                                            \
} while(0)
#define PASSED()        do {puts(" PASSED");fflush(stdout);} while(0)
#define FAILED()        do {puts("*FAILED*");fflush(stdout);} while(0)
#define WARNING()       do {puts("*WARNING*");fflush(stdout);} while(0)
#define SKIPPED()       do {puts(" -SKIP-");fflush(stdout);} while(0)
#define TEST_ERROR      do {FAILED(); AT(); goto error;} while(0)
#define FAIL_OP_ERROR(OP) do {FAILED(); AT(); OP; goto error;} while(0)
#define OP_SUPPRESS(OP, COUNTER, LIMIT) do {                                   \
    if((COUNTER) <= (LIMIT)) {                                                 \
        OP;                                                                    \
        if((COUNTER) == (LIMIT))                                               \
            puts("    Suppressing further output...");                         \
        fflush(stdout);                                                        \
    } /* end if */                                                             \
} while(0)

/*
 * Array of number of threads.  Each threaded test is run once for each entry in
 * this array.  Remove the last test if OPA_LIMIT_THREADS is defined.
 */
static const unsigned num_threads[] = {1, 2, 4, 10, 100};
static const unsigned num_thread_tests = sizeof(num_threads) / sizeof(num_threads[0])
#if OPA_MAX_NTHREADS < 10
- 2
#elif OPA_MAX_NTHREADS < 100
- 1
#endif /* OPA_LIMIT_THREADS */
;

/*
 * Factor to reduce the number of iterations by for each test.  Must be the same
 * size as num_threads.
 */
static const unsigned iter_reduction[] = {1, 1, 1, 1, 10};

/*
 * Other global variables.
 */
static unsigned curr_test;   /* Current test number bein run */

