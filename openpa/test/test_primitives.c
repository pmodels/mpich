/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Uncomment this definition to disable the OPA library and instead use naive
 * (non-atomic) operations.  This should cause failures.
 */
/*
#define OPA_TEST_NAIVE
*/

#include "opa_test.h"

/* Definitions for test_threaded_loadstore_int */
/* LOADSTORE_INT_DIFF is the difference between the unique values of successive
 * threads.  This value contains 0's in the top half of the int and 1's in the
 * bottom half.  Therefore, for 4 byte ints the sequence of unique values is:
 * 0x00000000, 0x0000FFFF, 0x0001FFFE, 0x0002FFFD, etc. */
#define LOADSTORE_INT_DIFF ((1 << (sizeof(int) * CHAR_BIT / 2)) - 1)
#define LOADSTORE_INT_NITER (4000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being read/written by all threads */
    int         unique_val;     /* This thread's unique value to store in shared_val */
} loadstore_int_t;

/* Definitions for test_threaded_loadstore_ptr */
#define LOADSTORE_PTR_DIFF (((unsigned long) 1 << (sizeof(void *) * CHAR_BIT / 2)) - 1)
#define LOADSTORE_PTR_NITER (4000000 / iter_reduction[curr_test])
typedef struct {
    OPA_ptr_t   *shared_val;    /* Shared int being read/written by all threads */
    void        *unique_val;    /* This thread's unique value to store in shared_val */
} loadstore_ptr_t;

/* Definitions for test_threaded_add */
#define ADD_EXPECTED ADD_NITER
#define ADD_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being added to by all threads */
    int         unique_val;     /* This thread's unique value to add to shared_val */
} add_t;

/* Definitions for test_threaded_incr_decr */
#define INCR_DECR_EXPECTED INCR_DECR_NITER
#define INCR_DECR_NITER (2000000 / iter_reduction[curr_test])

/* Definitions for test_threaded_decr_and_test */
#define DECR_AND_TEST_NITER_INNER 20            /* *Must* be even */
#define DECR_AND_TEST_NITER_OUTER (10000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being decr and tested by all threads */
    unsigned    ntrue;          /* # of times decr_and_test returned true */
} decr_test_t;

/* Definitions for test_threaded_faa */
/* Uses definitions from test_threaded_add */

/* Definitions for test_threaded_faa_ret */
#define FAA_RET_EXPECTED (-((nthreads - 1) * FAA_RET_NITER))
#define FAA_RET_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being added to by all threads */
    OPA_int_t   *nerrors;       /* Number of errors */
    OPA_int_t   *n1;            /* # of times faa returned 1 */
} faa_ret_t;


/*-------------------------------------------------------------------------
 * Function: test_simple_loadstore_int
 *
 * Purpose: Tests basic functionality of OPA_load and OPA_store with a
 *          single thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, March 19, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_loadstore_int(void)
{
    OPA_int_t   a, b;

    TESTING("simple integer load/store functionality", 0);

    /* Store 0 in a, -1 in b.  Verify that these values are returned by
     * OPA_load. */
    OPA_store(&a, 0);
    if(0 != OPA_load(&a)) TEST_ERROR;
    OPA_store(&b, -1);
    if(-1 != OPA_load(&b)) TEST_ERROR;
    if(0 != OPA_load(&a)) TEST_ERROR;
    if(-1 != OPA_load(&b)) TEST_ERROR;

    /* Store INT_MIN in a and INT_MAX in b.  Verify that these values are
     * returned by OPA_LOAD. */
    OPA_store(&a, INT_MIN);
    if(INT_MIN != OPA_load(&a)) TEST_ERROR;
    OPA_store(&b, INT_MAX);
    if(INT_MAX != OPA_load(&b)) TEST_ERROR;
    if(INT_MIN != OPA_load(&a)) TEST_ERROR;
    if(INT_MAX != OPA_load(&b)) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_simple_loadstore_int() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_loadstore_int_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_loadstore_int
 *
 * Return: Success: NULL
 *         Failure: non-NULL
 *
 * Programmer: Neil Fortner
 *             Thursday, March 19, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_loadstore_int_helper(void *_udata)
{
    loadstore_int_t     *udata = (loadstore_int_t *)_udata;
    int                 loaded_val;
    unsigned            niter = LOADSTORE_INT_NITER;
    unsigned            nerrors = 0;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Store the unique value into the shared value */
        OPA_store(udata->shared_val, udata->unique_val);

        /* Load the shared_value, and check if it is valid */
        if((loaded_val = OPA_load(udata->shared_val)) % LOADSTORE_INT_DIFF) {
            printf("    Unexpected load: %d is not a multiple of %d\n",
                    loaded_val, LOADSTORE_INT_DIFF);
            nerrors++;
        } /* end if */
    } /* end for */

    /* Any non-NULL exit value indicates an error, we use &i here */
    pthread_exit(nerrors ? &i : NULL);
} /* end threaded_loadstore_int_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_loadstore_int
 *
 * Purpose: Tests atomicity of OPA_load and OPA_store.  Launches nthreads
 *          threads, each of which repeatedly loads and stores a shared
 *          variable.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, March 19, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_loadstore_int(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    loadstore_int_t     *thread_data = NULL; /* User data structs for each thread */
    OPA_int_t           shared_int;     /* Integer shared between threads */
    void                *ret;           /* Thread return value */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            nerrors = 0;    /* number of errors */
    unsigned            i;

    TESTING("integer load/store", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (loadstore_int_t *) malloc(nthreads *
            sizeof(loadstore_int_t)))) TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_int;
        thread_data[i].unique_val = i * LOADSTORE_INT_DIFF;
        if(pthread_create(&threads[i], &ptattr, threaded_loadstore_int_helper,
                &thread_data[i])) TEST_ERROR;
    } /* end for */

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<nthreads; i++) {
        if(pthread_join(threads[i], &ret)) TEST_ERROR;
        if(ret)
            nerrors++;
    } /* end for */

    /* Check for errors */
    if(nerrors)
        FAIL_OP_ERROR(printf("    Unexpected return from %d thread%s\n", nerrors,
                nerrors == 1 ? "" : "s"));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("integer load/store", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_loadstore_int() */


/*-------------------------------------------------------------------------
 * Function: test_simple_loadstore_ptr
 *
 * Purpose: Tests basic functionality of OPA_load and OPA_store with a
 *          single thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, March 20, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_loadstore_ptr(void)
{
    OPA_ptr_t   a, b;

    TESTING("simple pointer load/store functionality", 0);

    /* Store 0 in a, 1 in b.  Verify that these values are returned by
     * OPA_load. */
    OPA_store_ptr(&a, (void *) 0);
    if((void *) 0 != OPA_load_ptr(&a)) TEST_ERROR;
    OPA_store_ptr(&b, (void *) 1);
    if((void *) 1 != OPA_load_ptr(&b)) TEST_ERROR;
    if((void *) 0 != OPA_load_ptr(&a)) TEST_ERROR;
    if((void *) 1 != OPA_load_ptr(&b)) TEST_ERROR;

    /* Store -1 in a and -2 in b.  Verify that these values are returned by
     * OPA_LOAD. */
    OPA_store_ptr(&a, (void *) -2);
    if((void *) -2 != OPA_load_ptr(&a)) TEST_ERROR;
    OPA_store_ptr(&b, (void *) -1);
    if((void *) -1 != OPA_load_ptr(&b)) TEST_ERROR;
    if((void *) -2 != OPA_load_ptr(&a)) TEST_ERROR;
    if((void *) -1 != OPA_load_ptr(&b)) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_simple_loadstore_ptr() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_loadstore_ptr_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_loadstore_ptr
 *
 * Return: Success: NULL
 *         Failure: non-NULL
 *
 * Programmer: Neil Fortner
 *             Thursday, March 20, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_loadstore_ptr_helper(void *_udata)
{
    loadstore_ptr_t     *udata = (loadstore_ptr_t *)_udata;
    unsigned long       loaded_val;
    int                 niter = LOADSTORE_PTR_NITER;
    unsigned            nerrors = 0;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Store the unique value into the shared value */
        OPA_store_ptr(udata->shared_val, udata->unique_val);

        /* Load the shared_value, and check if it is valid */
        if((loaded_val = (unsigned long) OPA_load_ptr(udata->shared_val)) % LOADSTORE_PTR_DIFF) {
            printf("    Unexpected load: %lu is not a multiple of %lu\n",
                    loaded_val, LOADSTORE_PTR_DIFF);
            nerrors++;
        } /* end if */
    } /* end for */

    /* Any non-NULL exit value indicates an error, we use &i here */
    pthread_exit(nerrors ? &i : NULL);
} /* end threaded_loadstore_ptr_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_loadstore_ptr
 *
 * Purpose: Tests atomicity of OPA_load and OPA_store.  Launches nthreads
 *          threads, each of which repeatedly loads and stores a shared
 *          variable.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, March 20, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_loadstore_ptr(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    loadstore_ptr_t     *thread_data = NULL; /* User data structs for each thread */
    OPA_ptr_t           shared_ptr;     /* Integer shared between threads */
    void                *ret;           /* Thread return value */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            nerrors = 0;    /* number of errors */
    unsigned            i;

    TESTING("pointer load/store", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (loadstore_ptr_t *) malloc(nthreads *
            sizeof(loadstore_ptr_t)))) TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_ptr;
        thread_data[i].unique_val = (void *) ((unsigned long) i * LOADSTORE_PTR_DIFF);
        if(pthread_create(&threads[i], &ptattr, threaded_loadstore_ptr_helper,
                &thread_data[i])) TEST_ERROR;
    } /* end for */

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<nthreads; i++) {
        if(pthread_join(threads[i], &ret)) TEST_ERROR;
        if(ret)
            nerrors++;
    } /* end for */

    /* Check for errors */
    if(nerrors)
        FAIL_OP_ERROR(printf("    Unexpected return from %d thread%s\n", nerrors,
                nerrors == 1 ? "" : "s"));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("pointer load/store", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_loadstore_ptr() */


/*-------------------------------------------------------------------------
 * Function: test_simple_add_incr_decr
 *
 * Purpose: Tests basic functionality of OPA_add, OPA_incr and OPA_decr
 *          with a single thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, March 20, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_add_incr_decr(void)
{
    OPA_int_t   a;


    TESTING("simple add/incr/decr functionality", 0);

    /* Store 0 in a */
    OPA_store(&a, 0);

    /* Add INT_MIN */
    OPA_add(&a, INT_MIN);

    /* Increment */
    OPA_incr(&a);

    /* Add INT_MAX */
    OPA_add(&a, INT_MAX);

    /* Decrement */
    OPA_decr(&a);

    /* Load the result, verify it is correct */
    if(OPA_load(&a) != INT_MIN + 1 + INT_MAX - 1) TEST_ERROR;

    /* Store 0 in a */
    OPA_store(&a, 0);

    /* Add INT_MAX */
    OPA_add(&a, INT_MAX);

    /* Decrement */
    OPA_decr(&a);

    /* Add INT_MIN */
    OPA_add(&a, INT_MIN);

    /* Increment */
    OPA_incr(&a);

    /* Load the result, verify it is correct */
    if(OPA_load(&a) != INT_MAX - 1 + INT_MIN + 1) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_simple_add_incr_decr() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_add_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_add
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Thursday, March 20, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_add_helper(void *_udata)
{
    add_t               *udata = (add_t *)_udata;
    unsigned            niter = ADD_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Add the unique value to the shared value */
        OPA_add(udata->shared_val, udata->unique_val);

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_add_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_add
 *
 * Purpose: Tests atomicity of OPA_add.  Launches nthreads threads, each
 *          of which repeatedly adds a unique number to a shared variable.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, March 20, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_add(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    add_t               *thread_data = NULL; /* User data structs for each thread */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("add", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (add_t *) malloc(nthreads * sizeof(add_t))))
        TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Set the initial state of the shared value (0) */
    OPA_store(&shared_val, 0);

    /* Create the threads.  All the unique values must add up to 0. */
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].unique_val = i - (nthreads - 1) / 2
                - (!(nthreads % 2) && (i >= nthreads / 2))
                + (i == nthreads - 1);
        if(pthread_create(&threads[i], &ptattr, threaded_add_helper,
                &thread_data[i])) TEST_ERROR;
    } /* end for */

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<nthreads; i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load(&shared_val) != ADD_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load(&shared_val), ADD_EXPECTED));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("add", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_add() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_incr_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_incr_decr
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, April 21, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_incr_helper(void *_shared_val)
{
    OPA_int_t           *shared_val = (OPA_int_t *)_shared_val;
    unsigned            niter = INCR_DECR_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Add the unique value to the shared value */
        OPA_incr(shared_val);

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_incr_helper() */


/*-------------------------------------------------------------------------
 * Function: threaded_decr_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_incr_decr
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, April 21, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_decr_helper(void *_shared_val)
{
    OPA_int_t           *shared_val = (OPA_int_t *)_shared_val;
    unsigned            niter = INCR_DECR_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Add the unique value to the shared value */
        OPA_decr(shared_val);

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_decr_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_incr_decr
 *
 * Purpose: Tests atomicity of OPA_incr and OPA_decr.  Launches nthreads
 *          threads, each of which repeatedly either increments or
 *          decrements a shared variable.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, April 21, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_incr_decr(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    /* Must use an odd number of threads */
    if(!(nthreads & 1))
        nthreads++;

    TESTING("incr and decr", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Set the initial state of the shared value (0) */
    OPA_store(&shared_val, 0);

    /* Create the threads.  All the unique values must add up to 0. */
    for(i=0; i<nthreads; i++) {
        if(i & 1) {
            if(pthread_create(&threads[i], &ptattr, threaded_decr_helper,
                    &shared_val)) TEST_ERROR;
        } else
            if(pthread_create(&threads[i], &ptattr, threaded_incr_helper,
                    &shared_val)) TEST_ERROR;
    } /* end for */

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<nthreads; i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load(&shared_val) != INCR_DECR_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load(&shared_val), INCR_DECR_EXPECTED));

    /* Free memory */
    free(threads);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("incr and decr", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_incr_decr() */


/*-------------------------------------------------------------------------
 * Function: test_simple_decr_and_test
 *
 * Purpose: Tests basic functionality of OPA_decr_and_test with a single
 *          thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 22, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_decr_and_test(void)
{
    OPA_int_t   a;
    int         i;

    TESTING("simple decr and test functionality", 0);

    /* Store 10 in a */
    OPA_store(&a, 10);

    for(i = 0; i<20; i++)
        if(OPA_decr_and_test(&a))
            if(i != 9) TEST_ERROR;

    if(OPA_load(&a) != -10) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_simple_decr_and_test() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_decr_and_test_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_decr_and_test
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 22, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_decr_and_test_helper(void *_udata)
{
    decr_test_t         *udata = (decr_test_t *)_udata;
    unsigned            i;

    /* Main loop */
    for(i=0; i<DECR_AND_TEST_NITER_INNER; i++)
        /* Add the unique value to the shared value */
        if(OPA_decr_and_test(udata->shared_val))
            udata->ntrue++;

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_decr_and_test_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_decr_and_test
 *
 * Purpose: Tests atomicity of OPA_add and OPA_store.  Launches nthreads
 *          threads, each of which repeatedly adds a unique number to a
 *          shared variable.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, April 21, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_decr_and_test(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    decr_test_t         *thread_data = NULL; /* User data structs for each thread */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            *ntrue;         /* Number of times decr_and_test returned true */
    unsigned            ntrue_total;
    int                 starting_val;
    unsigned            niter = DECR_AND_TEST_NITER_OUTER;
    unsigned            nthreads = num_threads[curr_test];
    unsigned            nerrors = 0;
    unsigned            i, j;

    TESTING("decr and test", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (decr_test_t *) malloc(nthreads * sizeof(decr_test_t))))
        TEST_ERROR;

    /* Initialize the "shared_val" fields (ntrue will be initialized in the
     * loop */
    for(i=0; i<nthreads; i++)
        thread_data[i].shared_val = &shared_val;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Calculate the starting value for the shared int */
    starting_val = DECR_AND_TEST_NITER_INNER * nthreads / 2;

    /* Outer loop (perform the threaded test multiple times */
    for(i=0; i<niter; i++) {
        /* Set the initial state of the shared value (INCR_DECR_EXPECTED) */
        OPA_store(&shared_val, starting_val);

        /* Create the threads.  Initialize each thread's "ntrue" field. */
        for(j=0; j<nthreads; j++) {
            thread_data[j].ntrue = 0;
            if(pthread_create(&threads[j], &ptattr, threaded_decr_and_test_helper,
                    &thread_data[j])) TEST_ERROR;
        } /* end for */

        /* Join the threads */
        ntrue_total = 0;
        for (j=0; j<nthreads; j++) {
            /* Join thread j */
            if(pthread_join(threads[j], NULL)) TEST_ERROR;

            /* Verify that OPA_decr_and_test returned true at most once */
            if(thread_data[j].ntrue > 1) {
                printf("\n    Unexpected return from thread %u: OPA_decr_and_test returned true %u times",
                        j, thread_data[j].ntrue);
                nerrors++;
            } /* end if */

            /* Update ntrue_total and free ntrue (which was allocated in the
             * thread routine) */
            ntrue_total += thread_data[j].ntrue;
        } /* end for */

        /* Verify that OPA_decr_and_test returned true exactly once over all the
         * threads. */
        if(ntrue_total != 1) {
            printf("\n    Unexpected result: OPA_decr_and_test returned true %u times total",
                    ntrue_total);
            nerrors++;
        } /* end if */

        /* Verify that the shared value contains the expected result */
        if(OPA_load(&shared_val) != -starting_val) {
            printf("\n    Unexpected result: %d expected: %d",
                    OPA_load(&shared_val), -starting_val);
            nerrors++;
        } /* end if */
    } /* end for */

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    if(nerrors) {
        puts("");
        TEST_ERROR;
    } /* end if */

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("decr and test", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_decr_and_test() */


/*-------------------------------------------------------------------------
 * Function: test_simple_faa_fai_fad
 *
 * Purpose: Tests basic functionality of OPA_fetch_and_add,
 *          OPA_fetch_and_incr and OPA_fetch_and_decr with a single
 *          thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, April 23, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_faa_fai_fad(void)
{
    OPA_int_t   a;
    int         result, expected;


    TESTING("simple fetch and add/incr/decr functionality", 0);

    /* Store 0 in a */
    OPA_store(&a, 0);
    expected = 0;

    /* Add INT_MIN */
    result = OPA_fetch_and_add(&a, INT_MIN);
    if(result != expected) TEST_ERROR;
    expected += INT_MIN;

    /* Increment */
    result = OPA_fetch_and_incr(&a);
    if(result != expected) TEST_ERROR;
    expected++;

    /* Add INT_MAX */
    result = OPA_fetch_and_add(&a, INT_MAX);
    if(result != expected) TEST_ERROR;
    expected += INT_MAX;

    /* Decrement */
    result = OPA_fetch_and_decr(&a);
    if(result != expected) TEST_ERROR;
    expected--;

    /* Load the result, verify it is correct */
    if(OPA_load(&a) != INT_MIN + 1 + INT_MAX - 1) TEST_ERROR;

    /* Store 0 in a */
    OPA_store(&a, 0);
    expected = 0;

    /* Add INT_MAX */
    result = OPA_fetch_and_add(&a, INT_MAX);
    if(result != expected) TEST_ERROR;
    expected += INT_MAX;

    /* Decrement */
    result = OPA_fetch_and_decr(&a);
    if(result != expected) TEST_ERROR;
    expected--;

    /* Add INT_MIN */
    result = OPA_fetch_and_add(&a, INT_MIN);
    if(result != expected) TEST_ERROR;
    expected += INT_MIN;

    /* Increment */
    result = OPA_fetch_and_incr(&a);
    if(result != expected) TEST_ERROR;
    expected++;

    /* Load the result, verify it is correct */
    if(OPA_load(&a) != INT_MAX - 1 + INT_MIN + 1) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_simple_faa_fai_fad() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_faa_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_faa
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Thursday, April 23, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_faa_helper(void *_udata)
{
    add_t               *udata = (add_t *)_udata;
    unsigned            niter = ADD_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Add the unique value to the shared value */
        (void) OPA_fetch_and_add(udata->shared_val, udata->unique_val);

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_add_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_faa
 *
 * Purpose: Tests atomicity of OPA_fetch_and_add.  Launches nthreads
 *          threads, each of which repeatedly adds a unique number to a
 *          shared variable.  Does not test return value of
 *          OPA_fetch_and_add.  This is basically a copy of
 *          test_threaded_add.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, April 23, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_faa(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    add_t               *thread_data = NULL; /* User data structs for each thread */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("fetch and add", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (add_t *) malloc(nthreads * sizeof(add_t))))
        TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Set the initial state of the shared value (0) */
    OPA_store(&shared_val, 0);

    /* Create the threads.  All the unique values must add up to 1. */
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].unique_val = i - (nthreads - 1) / 2
                - (!(nthreads % 2) && (i >= nthreads / 2))
                + (i == nthreads - 1);
        if(pthread_create(&threads[i], &ptattr, threaded_faa_helper,
                &thread_data[i])) TEST_ERROR;
    } /* end for */

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<nthreads; i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load(&shared_val) != ADD_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load(&shared_val), ADD_EXPECTED));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("add", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_faa() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_faa_ret_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_faa_ret
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Thursday, April 23, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_faa_ret_helper(void *_udata)
{
    faa_ret_t           *udata = (faa_ret_t *)_udata;
    int                 ret, prev = INT_MAX;
    unsigned            niter = FAA_RET_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Add -1 to the shared value */
        ret = OPA_fetch_and_add(udata->shared_val, -1);

        /* Verify that the value returned is less than the previous return */
        if(ret >= prev) {
            printf("\n    Unexpected return: %d is not less than %d  ", ret, prev);
            OPA_incr(udata->nerrors);
        } /* end if */

        /* Check if the return value is 1 */
        if(ret == 1)
            OPA_incr(udata->n1);

        /* update prev */
        prev - ret;
    } /* end for */

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_faa_ret_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_faa_ret
 *
 * Purpose: Tests atomicity of OPA_fetch_and_add.  Launches nthreads
 *          threads, each of which repeatedly adds -1 to a shared
 *          variable.  Verifies that the value returned is always
 *          decreasing, and that it returns 1 exactly once.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Thursday, April 23, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_faa_ret(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    faa_ret_t           thread_data;    /* User data struct for all threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    OPA_int_t           nerrors;        /* Number of errors */
    OPA_int_t           n1;             /* # of times faa returned 1 */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("fetch and add return values", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Initialize thread data struct */
    OPA_store(&shared_val, FAA_RET_NITER);
    OPA_store(&nerrors, 0);
    OPA_store(&n1, 0);
    thread_data.shared_val = &shared_val;
    thread_data.nerrors = &nerrors;
    thread_data.n1 = &n1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads.  All the unique values must add up to 0. */
    for(i=0; i<nthreads; i++)
        if(pthread_create(&threads[i], &ptattr, threaded_faa_ret_helper,
                &thread_data)) TEST_ERROR;

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<nthreads; i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that no errors were reported */
    if(OPA_load(&nerrors))
        FAIL_OP_ERROR(printf("    %d unexpected returns from OPA_fetch_and_add\n",
                OPA_load(&nerrors)));

    /* Verify that OPA_fetch_and_add returned 1 expactly once */
    if(OPA_load(&n1) != 1)
        FAIL_OP_ERROR(printf("    OPA_fetch_and_add returned 1 %d times.  Expected: 1\n",
                OPA_load(&n1)));

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load(&shared_val) != FAA_RET_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load(&shared_val), FAA_RET_EXPECTED));

    /* Free memory */
    free(threads);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("add", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_faa_ret() */


/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     Tests the opa primitives
 *
 * Return:      Success:        exit(0)
 *
 *              Failure:        exit(1)
 *
 * Programmer:  Neil Fortner
 *              Thursday, March 19, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int main(int argc, char **argv)
{
    unsigned nerrors = 0;
#if defined(OPA_USE_LOCK_BASED_PRIMITIVES)
    pthread_mutex_t shm_lock;
    OPA_Interprocess_lock_init(&shm_lock, 1/*isLeader*/);
#endif

    /* Simple tests */
    nerrors += test_simple_loadstore_int();
    nerrors += test_simple_loadstore_ptr();
    nerrors += test_simple_add_incr_decr();
    nerrors += test_simple_decr_and_test();
    nerrors += test_simple_faa_fai_fad();

    /* Loop over test configurations */
    for(curr_test=0; curr_test<num_thread_tests; curr_test++) {
        /* Threaded tests */
        nerrors += test_threaded_loadstore_int();
        nerrors += test_threaded_loadstore_ptr();
        nerrors += test_threaded_add();
        nerrors += test_threaded_incr_decr();
        nerrors += test_threaded_decr_and_test();
        nerrors += test_threaded_faa();
        nerrors += test_threaded_faa_ret();
    }

    if(nerrors)
        goto error;
    printf("All primitives tests passed.\n");

    return 0;

error:
    if(!nerrors)
        nerrors = 1;
    printf("***** %d PRIMITIVES TEST%s FAILED! *****\n",
            nerrors, 1 == nerrors ? "" : "S");
    return 1;
} /* end main() */

