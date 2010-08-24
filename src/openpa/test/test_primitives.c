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
    int         nerrors;        /* Number of errors */
    int         master_thread;  /* Whether this is the master thread */
} loadstore_int_t;

/* Definitions for test_threaded_loadstore_ptr */
#define LOADSTORE_PTR_DIFF (((unsigned long) 1 << (sizeof(void *) * CHAR_BIT / 2)) - 1)
#define LOADSTORE_PTR_NITER (4000000 / iter_reduction[curr_test])
typedef struct {
    OPA_ptr_t   *shared_val;    /* Shared int being read/written by all threads */
    void        *unique_val;    /* This thread's unique value to store in shared_val */
    int         nerrors;        /* Number of errors */
    int         master_thread;  /* Whether this is the master thread */
} loadstore_ptr_t;

/* Definitions for test_threaded_add */
#define ADD_EXPECTED ADD_NITER
#define ADD_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being added to by all threads */
    int         unique_val;     /* This thread's unique value to add to shared_val */
    int         master_thread;  /* Whether this is the master thread */
} add_t;

/* Definitions for test_threaded_incr_decr */
#define INCR_DECR_EXPECTED INCR_DECR_NITER
#define INCR_DECR_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being manipulated by all threads */
    int         master_thread;  /* Whether this is the master thread */
} incr_decr_t;

/* Definitions for test_threaded_decr_and_test */
#define DECR_AND_TEST_NITER_INNER 20            /* *Must* be even */
#define DECR_AND_TEST_NITER_OUTER (10000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being decr and tested by all threads */
    unsigned    ntrue;          /* # of times decr_and_test returned true */
    int         master_thread;  /* Whether this is the master thread */
} decr_test_t;

/* Definitions for test_threaded_faa */
/* Uses definitions from test_threaded_add */

/* Definitions for test_threaded_faa_ret */
#define FAA_RET_EXPECTED (-((nthreads - 1) * FAA_RET_NITER))
#define FAA_RET_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being added to by all threads */
    int         nerrors;        /* Number of errors */
    int         n1;             /* # of times faa returned 1 */
    int         master_thread;  /* Whether this is the master thread */
} faa_ret_t;

/* Definitions for test_threaded_fai_fad */
/* Uses definitions from test_threaded_incr_decr */

/* Definitions for test_threaded_fai_ret */
#define FAI_RET_EXPECTED ((nthreads - 1) * FAA_RET_NITER)
#define FAI_RET_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being added to by all threads */
    int         nerrors;        /* Number of errors */
    int         nm1;            /* # of times faa returned -1 */
    int         master_thread;  /* Whether this is the master thread */
} fai_ret_t;

/* Definitions for test_threaded_fad_red */
/* Uses definitions from test_threaded_faa_ret */

/* Definitions for test_threaded_cas_int */
#define CAS_INT_NITER (5000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being manipulated by all threads */
    int         threadno;       /* Unique thread number */
    int         nsuccess;       /* # of times cas succeeded */
    int         master_thread;  /* Whether this is the master thread */
} cas_int_t;

/* Definitions for test_threaded_cas_ptr */
#define CAS_PTR_NITER (5000000 / iter_reduction[curr_test])
typedef struct {
    OPA_ptr_t   *shared_val;    /* Shared ptr being manipulated by all threads */
    int         *threadno;      /* Unique thread number */
    int         *max_threadno;  /* Maximum unique thread number */
    int         nsuccess;       /* # of times cas succeeded */
    int         master_thread;  /* Whether this is the master thread */
} cas_ptr_t;

/* Definitions for test_grouped_cas_int */
#define GROUPED_CAS_INT_NITER (5000000 / iter_reduction[curr_test])
#define GROUPED_CAS_INT_TPG 4
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being manipulated by all threads */
    int         groupno;        /* Unique group number */
    int         ngroups;        /* Number of groups */
    int         nsuccess;       /* # of times cas succeeded */
    int         master_thread;  /* Whether this is the master thread */
} grouped_cas_int_t;

/* Definitions for test_grouped_cas_ptr */
#define GROUPED_CAS_PTR_NITER (5000000 / iter_reduction[curr_test])
#define GROUPED_CAS_PTR_TPG 4
typedef struct {
    OPA_ptr_t   *shared_val;    /* Shared ptr being manipulated by all threads */
    int         *groupno;       /* Unique group number */
    int         *max_groupno;   /* Maximum unique group number */
    int         nsuccess;       /* # of times cas succeeded */
    int         master_thread;  /* Whether this is the master thread */
} grouped_cas_ptr_t;

/* Definitions for test_threaded_cas_int_fairness */
#define CAS_INT_FAIRNESS_MIN_SUCCESS (100 / iter_reduction[curr_test])
#define CAS_INT_FAIRNESS_MAX_ITER (10000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being manipulated by all threads */
    int         nerrors;        /* Number of errors */
    int         threadno;       /* Unique thread number */
    int         nthreads;       /* Number of threads */
    OPA_int_t   *successful_threads; /* # of threads that have completed */
    int         terminated;     /* Whether this thread was terminated by MAX_ITER */
    int         master_thread;  /* Whether this is the master thread */
} cas_int_fairness_t;

/* Definitions for test_threaded_cas_ptr_fairness */
#define CAS_PTR_FAIRNESS_MIN_SUCCESS (100 / iter_reduction[curr_test])
#define CAS_PTR_FAIRNESS_MAX_ITER (10000000 / iter_reduction[curr_test])
typedef struct {
    OPA_ptr_t   *shared_val;    /* Shared ptr being manipulated by all threads */
    int         nerrors;        /* Number of errors */
    int         *threadno;      /* Unique thread number */
    int         nthreads;       /* Number of threads */
    OPA_int_t   *successful_threads; /* # of threads that have completed */
    int         terminated;     /* Whether this thread was terminated by MAX_ITER */
    int         master_thread;  /* Whether this is the master thread */
} cas_ptr_fairness_t;

/* Definitions for test_threaded_swap_int */
#define SWAP_INT_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_int_t   *shared_val;    /* Shared int being manipulated by all threads */
    int         local_val;      /* Local int value for this thread */
    int         master_thread;  /* Whether this is the master thread */
} swap_int_t;

/* Definitions for test_threaded_swap_ptr */
#define SWAP_PTR_NITER (2000000 / iter_reduction[curr_test])
typedef struct {
    OPA_ptr_t   *shared_val;    /* Shared ptr being manipulated by all threads */
    void        *local_val;     /* Local ptr value for this thread */
    unsigned    threadno;       /* Thread number */
    int         master_thread;  /* Whether this is the master thread */
} swap_ptr_t;

/* Definitions for test_threaded_llsc_int_aba */
#define LLSC_INT_ABA_NITER 2000000
typedef struct {
    char        padding1[OPA_TEST_CACHELINE_PADDING];
    OPA_int_t   shared_val;     /* Shared value being manipulated by both threads */
    char        padding2[OPA_TEST_CACHELINE_PADDING - sizeof(OPA_int_t)];
    OPA_int_t   pass_point_0;   /* Whether we have passed point 0 */
    OPA_int_t   pass_point_1;   /* Whether we have passed point 1 */
    OPA_int_t   pass_point_2;   /* Whether we have passed point 2 */
    OPA_int_t   change_val;     /* Whether we will change/have changed shared_val */
    int         false_positives; /* Number of times SC failed unnecessarily */
    int         nunchanged_val; /* Number of times change_val was false */
} llsc_int_aba_t;

/* Definitions for test_threaded_llsc_ptr_aba */
#define LLSC_PTR_ABA_NITER 2000000
typedef struct {
    char        padding1[OPA_TEST_CACHELINE_PADDING];
    OPA_ptr_t   shared_val;     /* Shared value being manipulated by both threads */
    char        padding2[OPA_TEST_CACHELINE_PADDING - sizeof(OPA_ptr_t)];
    OPA_int_t   pass_point_0;   /* Whether we have passed point 0 */
    OPA_int_t   pass_point_1;   /* Whether we have passed point 1 */
    OPA_int_t   pass_point_2;   /* Whether we have passed point 2 */
    OPA_int_t   change_val;     /* Whether we will change/have changed shared_val */
    int         false_positives; /* Number of times SC failed unnecessarily */
    int         nunchanged_val; /* Number of times change_val was false */
} llsc_ptr_aba_t;

/* Definitions for test_threaded_llsc_int_stack */
#define LLSC_INT_STACK_NITER (100000 / iter_reduction[curr_test])
#define LLSC_INT_STACK_NLOOP 100000
#define LLSC_INT_STACK_TPG 3
typedef struct {
    OPA_int_t   on_stack;       /* Whether this object is on the stack */
    OPA_int_t   next;           /* Index of next object in stack */
} llsc_int_stack_obj_t;
typedef struct {
    OPA_int_t   *head;          /* Head of stack (index into objs) */
    llsc_int_stack_obj_t **objs; /* Array of pointers to objects */
    int         obj;            /* Object for currect pusher thread (index into objs) */
    OPA_int_t   *npushers;      /* Number of pusher threads running */
    int         nsuccess;       /* Number of successful pops or pushes */
    int         nerrors;        /* Number of errors */
    OPA_int_t   *failed;        /* Whether the test has failed and should be terminated */
    int         master_thread;  /* Whether this is the master thread */
} llsc_int_stack_t;

/* Definitions for test_threaded_llsc_ptr_stack */
#define LLSC_PTR_STACK_NITER (100000 / iter_reduction[curr_test])
#define LLSC_PTR_STACK_NLOOP 100000
#define LLSC_PTR_STACK_TPG 3
typedef struct {
    OPA_int_t   on_stack;       /* Whether this object is on the stack */
    OPA_ptr_t   next;           /* Next object in stack */
} llsc_ptr_stack_obj_t;
typedef struct {
    OPA_ptr_t   *head;          /* Head of stack (index into objs) */
    llsc_ptr_stack_obj_t *obj;  /* Object for currect pusher thread */
    OPA_int_t   *npushers;      /* Number of pusher threads running */
    int         nsuccess;       /* Number of successful pops or pushes */
    int         nerrors;        /* Number of errors */
    OPA_int_t   *failed;        /* Whether the test has failed and should be terminated */
    int         master_thread;  /* Whether this is the master thread */
} llsc_ptr_stack_t;


/*-------------------------------------------------------------------------
 * Function: test_simple_loadstore_int
 *
 * Purpose: Tests basic functionality of OPA_load_int and OPA_store_int with a
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
     * OPA_load_int. */
    OPA_store_int(&a, 0);
    if(0 != OPA_load_int(&a)) TEST_ERROR;
    OPA_store_int(&b, -1);
    if(-1 != OPA_load_int(&b)) TEST_ERROR;
    if(0 != OPA_load_int(&a)) TEST_ERROR;
    if(-1 != OPA_load_int(&b)) TEST_ERROR;

    /* Store INT_MIN in a and INT_MAX in b.  Verify that these values are
     * returned by OPA_load_int. */
    OPA_store_int(&a, INT_MIN);
    if(INT_MIN != OPA_load_int(&a)) TEST_ERROR;
    OPA_store_int(&b, INT_MAX);
    if(INT_MAX != OPA_load_int(&b)) TEST_ERROR;
    if(INT_MIN != OPA_load_int(&a)) TEST_ERROR;
    if(INT_MAX != OPA_load_int(&b)) TEST_ERROR;

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
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Store the unique value into the shared value */
        OPA_store_int(udata->shared_val, udata->unique_val);

        /* Load the shared_value, and check if it is valid */
        if((loaded_val = OPA_load_int(udata->shared_val)) % LOADSTORE_INT_DIFF) {
            printf("    Unexpected load: %d is not a multiple of %d\n",
                    loaded_val, LOADSTORE_INT_DIFF);
            udata->nerrors++;
        } /* end if */
    } /* end for */

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_loadstore_int_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_loadstore_int
 *
 * Purpose: Tests atomicity of OPA_load_int and OPA_store_int.  Launches nthreads
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
    unsigned            nthreads = num_threads[curr_test];
    unsigned            nerrors = 0;    /* number of errors */
    unsigned            i;

    TESTING("integer load/store", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (loadstore_int_t *) calloc(nthreads,
            sizeof(loadstore_int_t)))) TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Initialize thread data structs */
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_int;
        thread_data[i].unique_val = i * LOADSTORE_INT_DIFF;;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_loadstore_int_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_loadstore_int_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Check for errors */
    for(i=0; i<nthreads; i++)
        nerrors += thread_data[i].nerrors;
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d unexpected return%s from OPA_load_int\n", nerrors,
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
 * Purpose: Tests basic functionality of OPA_load_int and OPA_store_int with a
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
     * OPA_load_int. */
    OPA_store_ptr(&a, (void *) 0);
    if((void *) 0 != OPA_load_ptr(&a)) TEST_ERROR;
    OPA_store_ptr(&b, (void *) 1);
    if((void *) 1 != OPA_load_ptr(&b)) TEST_ERROR;
    if((void *) 0 != OPA_load_ptr(&a)) TEST_ERROR;
    if((void *) 1 != OPA_load_ptr(&b)) TEST_ERROR;

    /* Store -1 in a and -2 in b.  Verify that these values are returned by
     * OPA_load_int. */
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
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Store the unique value into the shared value */
        OPA_store_ptr(udata->shared_val, udata->unique_val);

        /* Load the shared_value, and check if it is valid */
        if((loaded_val = (unsigned long) OPA_load_ptr(udata->shared_val)) % LOADSTORE_PTR_DIFF) {
            printf("    Unexpected load: %lu is not a multiple of %lu\n",
                    loaded_val, LOADSTORE_PTR_DIFF);
            udata->nerrors++;
        } /* end if */
    } /* end for */

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_loadstore_ptr_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_loadstore_ptr
 *
 * Purpose: Tests atomicity of OPA_load_int and OPA_store_int.  Launches nthreads
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
    unsigned            nthreads = num_threads[curr_test];
    unsigned            nerrors = 0;    /* number of errors */
    unsigned            i;

    TESTING("pointer load/store", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (loadstore_ptr_t *) calloc(nthreads,
            sizeof(loadstore_ptr_t)))) TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Initialize thread data structs */
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_ptr;
        thread_data[i].unique_val = (void *) ((unsigned long) i * LOADSTORE_PTR_DIFF);
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_loadstore_ptr_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_loadstore_ptr_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;


    /* Check for errors */
    for(i=0; i<nthreads; i++)
        nerrors += thread_data[i].nerrors;
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d unexpected return%s from OPA_load_ptr\n", nerrors,
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
 * Purpose: Tests basic functionality of OPA_add_int, OPA_incr_int and
 *          OPA_decr_int with a single thread.  Does not test atomicity
 *          of operations.
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
    OPA_store_int(&a, 0);

    /* Add INT_MIN */
    OPA_add_int(&a, INT_MIN);

    /* Increment */
    OPA_incr_int(&a);

    /* Add INT_MAX */
    OPA_add_int(&a, INT_MAX);

    /* Decrement */
    OPA_decr_int(&a);

    /* Load the result, verify it is correct */
    if(OPA_load_int(&a) != INT_MIN + 1 + INT_MAX - 1) TEST_ERROR;

    /* Store 0 in a */
    OPA_store_int(&a, 0);

    /* Add INT_MAX */
    OPA_add_int(&a, INT_MAX);

    /* Decrement */
    OPA_decr_int(&a);

    /* Add INT_MIN */
    OPA_add_int(&a, INT_MIN);

    /* Increment */
    OPA_incr_int(&a);

    /* Load the result, verify it is correct */
    if(OPA_load_int(&a) != INT_MAX - 1 + INT_MIN + 1) TEST_ERROR;

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
        OPA_add_int(udata->shared_val, udata->unique_val);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_add_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_add
 *
 * Purpose: Tests atomicity of OPA_add_int.  Launches nthreads threads
 *          each of which repeatedly adds a unique number to a shared
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
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (add_t *) calloc(nthreads, sizeof(add_t))))
        TEST_ERROR;

    /* Initialize thread data structs.  All the unique values must add up to
     * 0. */
    OPA_store_int(&shared_val, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].unique_val = i - (nthreads - 1) / 2
                - (!(nthreads % 2) && (i >= nthreads / 2))
                + (i == nthreads - 1);
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_add_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_add_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load_int(&shared_val) != ADD_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load_int(&shared_val), ADD_EXPECTED));

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
static void *threaded_incr_helper(void *_udata)
{
    incr_decr_t         *udata = (incr_decr_t *)_udata;
    unsigned            niter = INCR_DECR_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Increment the shared value */
        OPA_incr_int(udata->shared_val);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
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
static void *threaded_decr_helper(void *_udata)
{
    incr_decr_t         *udata = (incr_decr_t *)_udata;
    unsigned            niter = INCR_DECR_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Decrement the shared value */
        OPA_decr_int(udata->shared_val);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_decr_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_incr_decr
 *
 * Purpose: Tests atomicity of OPA_incr_int and OPA_decr_int.  Launches
 *          nthreads threads, each of which repeatedly either increments
 *          or decrements a shared variable.
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
    incr_decr_t         *thread_data = NULL; /* User data structs for each thread */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    /* Must use an odd number of threads */
    if(!(nthreads & 1))
        nthreads--;

    TESTING("incr and decr", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (incr_decr_t *) calloc(nthreads, sizeof(incr_decr_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, 0);
    for(i=0; i<nthreads; i++)
        thread_data[i].shared_val = &shared_val;
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Set the initial state of the shared value (0) */
    OPA_store_int(&shared_val, 0);

    /* Create the threads.  All the unique values must add up to 0. */
    for(i=0; i<(nthreads - 1); i++) {
        if(i & 1) {
            if(pthread_create(&threads[i], &ptattr, threaded_decr_helper,
                    &thread_data[i])) TEST_ERROR;
        } else
            if(pthread_create(&threads[i], &ptattr, threaded_incr_helper,
                    &thread_data[i])) TEST_ERROR;
    } /* end for */
    if(i & 1)
        (void)threaded_decr_helper(&thread_data[i]);
    else
        (void)threaded_incr_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load_int(&shared_val) != INCR_DECR_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load_int(&shared_val), INCR_DECR_EXPECTED));

    /* Free memory */
    free(threads);
    free(thread_data);

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
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_incr_decr() */


/*-------------------------------------------------------------------------
 * Function: test_simple_decr_and_test
 *
 * Purpose: Tests basic functionality of OPA_decr_and_test_int with a
 *          single thread.  Does not test atomicity of operations.
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
    OPA_store_int(&a, 10);

    for(i = 0; i<20; i++)
        if(OPA_decr_and_test_int(&a))
            if(i != 9) TEST_ERROR;

    if(OPA_load_int(&a) != -10) TEST_ERROR;

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
        if(OPA_decr_and_test_int(udata->shared_val))
            udata->ntrue++;

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_decr_and_test_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_decr_and_test
 *
 * Purpose: Tests atomicity of OPA_decr_and_test_int.  Launches nthreads
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
    unsigned            ntrue_total;
    int                 starting_val;
    unsigned            niter = DECR_AND_TEST_NITER_OUTER;
    unsigned            nthreads = num_threads[curr_test];
    unsigned            nerrors = 0;
    unsigned            i, j;

    TESTING("decr and test", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (decr_test_t *) calloc(nthreads, sizeof(decr_test_t))))
        TEST_ERROR;

    /* Initialize the "shared_val" and "master thread" fields (ntrue will be
     * initialized in the loop) */
    for(i=0; i<nthreads; i++)
        thread_data[i].shared_val = &shared_val;
    thread_data[nthreads-1].master_thread = 1;

    /* Calculate the starting value for the shared int */
    starting_val = DECR_AND_TEST_NITER_INNER * nthreads / 2;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Outer loop (perform the threaded test multiple times */
    for(i=0; i<niter; i++) {
        /* Initialize thread data structs */
        OPA_store_int(&shared_val, starting_val);
        for(j=0; j<nthreads; j++)
            thread_data[j].ntrue = 0;

        /* Create the threads */
        for(j=0; j<(nthreads - 1); j++)
            if(pthread_create(&threads[j], &ptattr, threaded_decr_and_test_helper,
                    &thread_data[j])) TEST_ERROR;
        (void)threaded_decr_and_test_helper(&thread_data[j]);

        /* Join the threads */
        for (j=0; j<(nthreads - 1); j++)
            if(pthread_join(threads[j], NULL)) TEST_ERROR;

        /* Verify the results */
        ntrue_total = 0;
        for (j=0; j<nthreads; j++) {
            /* Verify that OPA_decr_and_test_int returned true at most once */
            if(thread_data[j].ntrue > 1) {
                printf("\n    Unexpected return from thread %u: OPA_decr_and_test_int returned true %u times",
                        j, thread_data[j].ntrue);
                nerrors++;
            } /* end if */

            /* Update ntrue_total */
            ntrue_total += thread_data[j].ntrue;
        } /* end for */

        /* Verify that OPA_decr_and_test_int returned true exactly once over all
         * the threads. */
        if(ntrue_total != 1) {
            printf("\n    Unexpected result: OPA_decr_and_test_int returned true %u times total",
                    ntrue_total);
            nerrors++;
        } /* end if */

        /* Verify that the shared value contains the expected result */
        if(OPA_load_int(&shared_val) != -starting_val) {
            printf("\n    Unexpected result: %d expected: %d",
                    OPA_load_int(&shared_val), -starting_val);
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
 * Purpose: Tests basic functionality of OPA_fetch_and_add_int,
 *          OPA_fetch_and_incr_int and OPA_fetch_and_decr_int with a 
 *          single thread.  Does not test atomicity of operations.
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
    OPA_store_int(&a, 0);
    expected = 0;

    /* Add INT_MIN */
    result = OPA_fetch_and_add_int(&a, INT_MIN);
    if(result != expected) TEST_ERROR;
    expected += INT_MIN;

    /* Increment */
    result = OPA_fetch_and_incr_int(&a);
    if(result != expected) TEST_ERROR;
    expected++;

    /* Add INT_MAX */
    result = OPA_fetch_and_add_int(&a, INT_MAX);
    if(result != expected) TEST_ERROR;
    expected += INT_MAX;

    /* Decrement */
    result = OPA_fetch_and_decr_int(&a);
    if(result != expected) TEST_ERROR;
    expected--;

    /* Load the result, verify it is correct */
    if(OPA_load_int(&a) != INT_MIN + 1 + INT_MAX - 1) TEST_ERROR;

    /* Store 0 in a */
    OPA_store_int(&a, 0);
    expected = 0;

    /* Add INT_MAX */
    result = OPA_fetch_and_add_int(&a, INT_MAX);
    if(result != expected) TEST_ERROR;
    expected += INT_MAX;

    /* Decrement */
    result = OPA_fetch_and_decr_int(&a);
    if(result != expected) TEST_ERROR;
    expected--;

    /* Add INT_MIN */
    result = OPA_fetch_and_add_int(&a, INT_MIN);
    if(result != expected) TEST_ERROR;
    expected += INT_MIN;

    /* Increment */
    result = OPA_fetch_and_incr_int(&a);
    if(result != expected) TEST_ERROR;
    expected++;

    /* Load the result, verify it is correct */
    if(OPA_load_int(&a) != INT_MAX - 1 + INT_MIN + 1) TEST_ERROR;

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
        (void) OPA_fetch_and_add_int(udata->shared_val, udata->unique_val);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_add_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_faa
 *
 * Purpose: Tests atomicity of OPA_fetch_and_add_int.  Launches nthreads
 *          threads, each of which repeatedly adds a unique number to a
 *          shared variable.  Does not test return value of
 *          OPA_fetch_and_add_int.  This is basically a copy of
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
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (add_t *) calloc(nthreads, sizeof(add_t))))
        TEST_ERROR;

    /* Initialize thread data structs.  All the unique values must add up to
     * 0. */
    OPA_store_int(&shared_val, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].unique_val = i - (nthreads - 1) / 2
                - (!(nthreads % 2) && (i >= nthreads / 2))
                + (i == nthreads - 1);
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_faa_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_faa_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load_int(&shared_val) != ADD_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load_int(&shared_val), ADD_EXPECTED));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("fetch and add", 0);
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
        ret = OPA_fetch_and_add_int(udata->shared_val, -1);

        /* Verify that the value returned is less than the previous return */
        if(ret >= prev) {
            printf("\n    Unexpected return: %d is not less than %d  ", ret, prev);
            (udata->nerrors)++;
        } /* end if */

        /* Check if the return value is 1 */
        if(ret == 1)
            (udata->n1)++;

        /* update prev */
        prev = ret;
    } /* end for */

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_faa_ret_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_faa_ret
 *
 * Purpose: Tests atomicity of OPA_fetch_and_add_int.  Launches nthreads
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
    faa_ret_t           *thread_data = NULL; /* User data structs for threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    int                 nerrors = 0;    /* Number of errors */
    int                 n1 = 0;         /* # of times faa returned 1 */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("fetch and add return values", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (faa_ret_t *) calloc(nthreads, sizeof(faa_ret_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, FAA_RET_NITER);
    for(i=0; i<nthreads; i++)
        thread_data[i].shared_val = &shared_val;
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_faa_ret_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_faa_ret_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Count number of errors and number of times 1 was returned */
    for(i=0; i<nthreads; i++) {
        nerrors += thread_data[i].nerrors;
        n1 += thread_data[i].n1;
    } /* end for */

    /* Verify that no errors were reported */
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d unexpected return%s from OPA_fetch_and_add_int\n",
                nerrors, nerrors == 1 ? "" : "s"));

    /* Verify that OPA_fetch_and_add_int returned 1 expactly once */
    if(n1 != 1)
        FAIL_OP_ERROR(printf("    OPA_fetch_and_add_int returned 1 %d times.  Expected: 1\n",
                n1));

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load_int(&shared_val) != FAA_RET_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load_int(&shared_val), FAA_RET_EXPECTED));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("fetch and add return values", 0);
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
} /* end test_threaded_faa_ret() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_fai_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_fai_fad
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Friday, May 8, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_fai_helper(void *_shared_val)
{
    OPA_int_t           *shared_val = (OPA_int_t *)_shared_val;
    unsigned            niter = INCR_DECR_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Add the unique value to the shared value */
        (void) OPA_fetch_and_incr_int(shared_val);

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_fai_helper() */


/*-------------------------------------------------------------------------
 * Function: threaded_fad_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_fai_fad
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Friday, May 8, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_fad_helper(void *_shared_val)
{
    OPA_int_t           *shared_val = (OPA_int_t *)_shared_val;
    unsigned            niter = INCR_DECR_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        /* Add the unique value to the shared value */
        (void) OPA_fetch_and_decr_int(shared_val);

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_fad_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_fai_fad
 *
 * Purpose: Tests atomicity of OPA_fetch_and_incr_int and
 *          OPA_fetch_and_decr_int.  Launches nthreads threads, each of
 *          which repeatedly either increments or decrements a shared
 *          variable.  Does not test return values.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Friday, May 8, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_fai_fad(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    /* Must use an odd number of threads */
    if(!(nthreads & 1))
        nthreads--;

    TESTING("fetch and incr/decr", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Set the initial state of the shared value (0) */
    OPA_store_int(&shared_val, 0);

    /* Create the threads.  All the unique values must add up to 0. */
    for(i=0; i<nthreads; i++) {
        if(i & 1) {
            if(pthread_create(&threads[i], &ptattr, threaded_fad_helper,
                    (void *)&shared_val)) TEST_ERROR;
        } else
            if(pthread_create(&threads[i], &ptattr, threaded_fai_helper,
                    (void *)&shared_val)) TEST_ERROR;
    } /* end for */

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<nthreads; i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load_int(&shared_val) != INCR_DECR_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load_int(&shared_val), INCR_DECR_EXPECTED));

    /* Free memory */
    free(threads);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("fetch and incr/decr", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_fai_fad() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_fai_ret_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_fai_ret
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Friday, June 5, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_fai_ret_helper(void *_udata)
{
    fai_ret_t           *udata = (fai_ret_t *)_udata;
    int                 ret, prev = INT_MIN;
    unsigned            niter = FAI_RET_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Add -1 to the shared value */
        ret = OPA_fetch_and_incr_int(udata->shared_val);

        /* Verify that the value returned is greater than the previous return */
        if(ret <= prev) {
            printf("\n    Unexpected return: %d is not greater than %d  ", ret, prev);
            (udata->nerrors)++;
        } /* end if */

        /* Check if the return value is -1 */
        if(ret == -1)
            (udata->nm1)++;

        /* update prev */
        prev = ret;
    } /* end for */

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_fai_ret_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_fai_ret
 *
 * Purpose: Tests atomicity of OPA_fetch_and_incr_int.  Launches nthreads
 *          threads, each of which repeatedly adds -1 to a shared
 *          variable.  Verifies that the value returned is always
 *          decreasing, and that it returns 1 exactly once.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Friday, June 5, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_fai_ret(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    fai_ret_t           *thread_data = NULL; /* User data structs for threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    int                 nerrors = 0;    /* Number of errors */
    int                 n1 = 0;         /* # of times fai returned 1 */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("fetch and incr return values", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (fai_ret_t *) calloc(nthreads, sizeof(fai_ret_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, -FAI_RET_NITER);
    for(i=0; i<nthreads; i++)
        thread_data[i].shared_val = &shared_val;
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_fai_ret_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_fai_ret_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Count number of errors and number of times 1 was returned */
    for(i=0; i<nthreads; i++) {
        nerrors += thread_data[i].nerrors;
        n1 += thread_data[i].nm1;
    } /* end for */

    /* Verify that no errors were reported */
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d unexpected return%s from OPA_fetch_and_incr_int\n",
                nerrors, nerrors == 1 ? "" : "s"));

    /* Verify that OPA_fetch_and_add_int returned 1 expactly once */
    if(n1 != 1)
        FAIL_OP_ERROR(printf("    OPA_fetch_and_add_int returned -1 %d times.  Expected: 1\n",
                n1));

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load_int(&shared_val) != FAI_RET_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load_int(&shared_val), FAI_RET_EXPECTED));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("fetch and incr return values", 0);
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
} /* end test_threaded_fai_ret() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_fad_ret_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_fad_ret
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Friday, June 5, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_fad_ret_helper(void *_udata)
{
    faa_ret_t           *udata = (faa_ret_t *)_udata;
    int                 ret, prev = INT_MAX;
    unsigned            niter = FAA_RET_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Add -1 to the shared value */
        ret = OPA_fetch_and_decr_int(udata->shared_val);

        /* Verify that the value returned is less than the previous return */
        if(ret >= prev) {
            printf("\n    Unexpected return: %d is not less than %d  ", ret, prev);
            (udata->nerrors)++;
        } /* end if */

        /* Check if the return value is 1 */
        if(ret == 1)
            (udata->n1)++;

        /* update prev */
        prev = ret;
    } /* end for */

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_fad_ret_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_fad_ret
 *
 * Purpose: Tests atomicity of OPA_fetch_and_decr_int.  Launches nthreads
 *          threads, each of which repeatedly adds -1 to a shared
 *          variable.  Verifies that the value returned is always
 *          decreasing, and that it returns 1 exactly once.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Friday, June 5, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_fad_ret(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    faa_ret_t           *thread_data = NULL; /* User data structs for threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    int                 nerrors = 0;    /* Number of errors */
    int                 n1 = 0;         /* # of times faa returned 1 */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("fetch and decr return values", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (faa_ret_t *) calloc(nthreads, sizeof(faa_ret_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, FAA_RET_NITER);
    for(i=0; i<nthreads; i++)
        thread_data[i].shared_val = &shared_val;
    thread_data[nthreads-1].master_thread = 1;


    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_fad_ret_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_fad_ret_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Count number of errors and number of times 1 was returned */
    for(i=0; i<nthreads; i++) {
        nerrors += thread_data[i].nerrors;
        n1 += thread_data[i].n1;
    } /* end for */

    /* Verify that no errors were reported */
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d unexpected return%s from OPA_fetch_and_decr_int\n",
                nerrors, nerrors == 1 ? "" : "s"));

    /* Verify that OPA_fetch_and_add_int returned 1 expactly once */
    if(n1 != 1)
        FAIL_OP_ERROR(printf("    OPA_fetch_and_add_int returned 1 %d times.  Expected: 1\n",
                n1));

    /* Verify that the shared value contains the expected result (0) */
    if(OPA_load_int(&shared_val) != FAA_RET_EXPECTED)
        FAIL_OP_ERROR(printf("    Unexpected result: %d expected: %d\n",
                OPA_load_int(&shared_val), FAA_RET_EXPECTED));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("fetch and decr return values", 0);
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
} /* end test_threaded_fad_ret() */


/*-------------------------------------------------------------------------
 * Function: test_simple_cas_int
 *
 * Purpose: Tests basic functionality of OPA_cas_int with a single thread.
 *          Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_cas_int(void)
{
    OPA_int_t   a;

    TESTING("simple integer compare-and-swap functionality", 0);

    /* Store 0 in a */
    OPA_store_int(&a, 0);

    /* Compare and swap multiple times, verify return value and final result */
    if(0 != OPA_cas_int(&a, 1, INT_MAX)) TEST_ERROR;
    if(0 != OPA_cas_int(&a, 0, INT_MAX)) TEST_ERROR;
    if(INT_MAX != OPA_cas_int(&a, INT_MAX, INT_MIN)) TEST_ERROR;
    if(INT_MIN != OPA_cas_int(&a, INT_MAX, 1)) TEST_ERROR;
    if(INT_MIN != OPA_cas_int(&a, INT_MIN, 1)) TEST_ERROR;
    if(1 != OPA_load_int(&a)) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_simple_cas_int() */


/*-------------------------------------------------------------------------
 * Function: test_simple_cas_ptr
 *
 * Purpose: Tests basic functionality of OPA_cas_ptr with a single thread.
 *          Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_cas_ptr(void)
{
    OPA_ptr_t   a;
    void        *ptr1 = malloc(1);      /* Pointers to assign to a */
    void        *ptr2 = malloc(1);
    void        *ptr3 = malloc(1);
    void        *ptr4 = malloc(1);

    TESTING("simple pointer compare-and-swap functionality", 0);

    /* Store ptr1 in a */
    OPA_store_ptr(&a, ptr1);

    /* Compare and swap multiple times, verify return value and final result */
    if(ptr1 != OPA_cas_ptr(&a, ptr2, ptr3)) TEST_ERROR;
    if(ptr1 != OPA_cas_ptr(&a, ptr1, ptr3)) TEST_ERROR;
    if(ptr3 != OPA_cas_ptr(&a, ptr3, ptr4)) TEST_ERROR;
    if(ptr4 != OPA_cas_ptr(&a, ptr3, ptr2)) TEST_ERROR;
    if(ptr4 != OPA_cas_ptr(&a, ptr4, ptr2)) TEST_ERROR;
    if(ptr2 != OPA_load_ptr(&a)) TEST_ERROR;

    if(ptr1) free(ptr1);
    if(ptr2) free(ptr2);
    if(ptr3) free(ptr3);
    if(ptr4) free(ptr4);

    PASSED();
    return 0;

error:
    if(ptr1) free(ptr1);
    if(ptr2) free(ptr2);
    if(ptr3) free(ptr3);
    if(ptr4) free(ptr4);

    return 1;
} /* end test_simple_cas_ptr() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_cas_int_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_cas_int
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_cas_int_helper(void *_udata)
{
    cas_int_t           *udata = (cas_int_t *)_udata;
    int                 thread_id = udata->threadno;
    int                 next_id = (thread_id + 1) % num_threads[curr_test];
    unsigned            niter = CAS_INT_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        if(OPA_cas_int(udata->shared_val, thread_id, next_id) == thread_id)
            udata->nsuccess++;

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_cas_int_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_cas_int
 *
 * Purpose: Tests atomicity of OPA_cas_int.  Launches nthreads threads,
 *          each of which continually tries to compare-and-swap a shared
 *          value with its thread id (oldv) and thread id + 1 % n (newv).
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_cas_int(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    cas_int_t           *thread_data = NULL; /* User data structs for threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("integer compare-and-swap", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (cas_int_t *) calloc(nthreads, sizeof(cas_int_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].threadno = i;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_cas_int_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_cas_int_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that cas succeeded at least once */
    if(thread_data[0].nsuccess == 0)
        FAIL_OP_ERROR(printf("    Compare-and-swap never succeeded\n"));

    /* Verify that the number of successes does not increase with increasing
     * thread number, and also that the shared value's final value is consistent
     * with the numbers of successes */
    if(nthreads > 1)
        for(i=1; i<nthreads; i++) {
            if(thread_data[i].nsuccess > thread_data[i-1].nsuccess)
                FAIL_OP_ERROR(printf("    Thread %d succeeded more times than thread %d\n",
                        i, i-1));

            if((thread_data[i].nsuccess < thread_data[i-1].nsuccess)
                    && (OPA_load_int(&shared_val) != i))
                FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));
        } /* end for */
    if((thread_data[0].nsuccess == thread_data[nthreads-1].nsuccess)
            && (OPA_load_int(&shared_val) != 0))
        FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));

    /* Verify that the number of successes for the first threads is equal to or
     * one greater than the number of successes for the last thread.  Have
     * already determined that it is not less in the previous test. */
    if(thread_data[0].nsuccess > (thread_data[nthreads-1].nsuccess + 1))
        FAIL_OP_ERROR(printf("    Thread 0 succeeded %d times more than thread %d\n",
                thread_data[0].nsuccess - thread_data[nthreads-1].nsuccess,
                nthreads - 1));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("integer compare-and-swap", 0);
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
} /* end test_threaded_cas_int() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_cas_ptr_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_cas_ptr
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Monday, August 3, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_cas_ptr_helper(void *_udata)
{
    cas_ptr_t           *udata = (cas_ptr_t *)_udata;
    int                 *thread_id = udata->threadno;
    int                 *next_id = thread_id + 1;
    unsigned            niter = CAS_PTR_NITER;
    unsigned            i;

    /* This is the equivalent of the "modulus" operation, but for pointers */
    if(next_id > udata->max_threadno)
        next_id = (int *) 0;

    /* Main loop */
    for(i=0; i<niter; i++)
        if(OPA_cas_ptr(udata->shared_val, (void *) thread_id, (void *) next_id) == (void *) thread_id)
            udata->nsuccess++;

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_cas_ptr_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_cas_ptr
 *
 * Purpose: Tests atomicity of OPA_cas_ptr.  Launches nthreads threads,
 *          each of which continually tries to compare-and-swap a shared
 *          value with its thread id (oldv) and thread id + 1 % n (newv).
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Monday, August 3, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_cas_ptr(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    cas_ptr_t           *thread_data = NULL; /* User data structs for threads */
    OPA_ptr_t           shared_val;     /* Integer shared between threads */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("pointer compare-and-swap", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (cas_ptr_t *) calloc(nthreads, sizeof(cas_ptr_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_ptr(&shared_val, (void *) 0);
    thread_data[0].shared_val = &shared_val;
    thread_data[0].threadno = (int *) 0;
    for(i=1; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].threadno = (int *) 0 + i;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;
    for(i=0; i<nthreads; i++)
        thread_data[i].max_threadno = thread_data[nthreads-1].threadno;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_cas_ptr_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_cas_ptr_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Verify that cas succeeded at least once */
    if(thread_data[0].nsuccess == 0)
        FAIL_OP_ERROR(printf("    Compare-and-swap never succeeded\n"));

    /* Verify that the number of successes does not increase with increasing
     * thread number, and also that the shared value's final value is consistent
     * with the numbers of successes */
    if(nthreads > 1)
        for(i=1; i<nthreads; i++) {
            if(thread_data[i].nsuccess > thread_data[i-1].nsuccess)
                FAIL_OP_ERROR(printf("    Thread %d succeeded more times than thread %d\n",
                        i, i-1));

            if((thread_data[i].nsuccess < thread_data[i-1].nsuccess)
                    && (OPA_load_ptr(&shared_val) != thread_data[i].threadno))
                FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));
        } /* end for */
    if((thread_data[0].nsuccess == thread_data[nthreads-1].nsuccess)
            && (OPA_load_ptr(&shared_val) != (void *) 0))
        FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));

    /* Verify that the number of successes for the first threads is equal to or
     * one greater than the number of successes for the last thread.  Have
     * already determined that it is not less in the previous test. */
    if(thread_data[0].nsuccess > (thread_data[nthreads-1].nsuccess + 1))
        FAIL_OP_ERROR(printf("    Thread 0 succeeded %d times more than thread %d\n",
                thread_data[0].nsuccess - thread_data[nthreads-1].nsuccess,
                nthreads - 1));

    /* Free memory */
    free(threads);
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("pointer compare-and-swap", 0);
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
} /* end test_threaded_cas_ptr() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: grouped_cas_int_helper
 *
 * Purpose: Helper (thread) routine for test_grouped_cas_int
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, August 4, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *grouped_cas_int_helper(void *_udata)
{
    grouped_cas_int_t   *udata = (grouped_cas_int_t *)_udata;
    int                 group_id = udata->groupno;
    int                 next_id = (group_id + 1) % udata->ngroups;
    unsigned            niter = GROUPED_CAS_INT_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        if(OPA_cas_int(udata->shared_val, group_id, next_id) == group_id)
            udata->nsuccess++;

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end grouped_cas_int_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_grouped_cas_int
 *
 * Purpose: Tests atomicity of OPA_cas_int.  Launches nthreads threads,
 *          subdivided into nthreads/4 or 2 groups, whichever is greater.
 *          Each thread continually tries to compare-and-swap a shared
 *          value with its group id (oldv) and group id + 1 % ngroups
 *          (newv).
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, August 4, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_grouped_cas_int(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    grouped_cas_int_t   *thread_data = NULL; /* User data structs for threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    int                 ngroups;        /* Number of groups of threads */
    int                 threads_per_group; /* Threads per group */
    int                 *group_success = NULL; /* Number of successes for each group */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("grouped integer compare-and-swap", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data =
            (grouped_cas_int_t *) calloc(nthreads, sizeof(grouped_cas_int_t))))
        TEST_ERROR;

    /* Calculate number of groups and threads per group */
    if((nthreads / GROUPED_CAS_INT_TPG) >= 2)
        threads_per_group = GROUPED_CAS_INT_TPG;
    else
        threads_per_group = (nthreads + 1) / 2;
    ngroups = (nthreads + threads_per_group - 1) / threads_per_group;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].groupno = i / threads_per_group;
        thread_data[i].ngroups = ngroups;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, grouped_cas_int_helper,
                &thread_data[i])) TEST_ERROR;
    (void)grouped_cas_int_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Calculate the total number of successes for each group */
    if(NULL == (group_success = (int *) calloc(ngroups, sizeof(int))))
        TEST_ERROR;
    for(i=0; i<nthreads; i++)
        group_success[thread_data[i].groupno] += thread_data[i].nsuccess;

    /* Verify that cas succeeded at least once */
    if(group_success[0] == 0)
        FAIL_OP_ERROR(printf("    Compare-and-swap never succeeded\n"));

    /* Verify that the number of successes does not increase with increasing
     * group number, and also that the shared value's final value is consistent
     * with the numbers of successes */
    if(nthreads > 1)
        for(i=1; i<ngroups; i++) {
            if(group_success[i] > group_success[i-1])
                FAIL_OP_ERROR(printf("    Group %d succeeded more times than group %d\n",
                        i, i-1));

            if((group_success[i] < group_success[i-1])
                    && (OPA_load_int(&shared_val) != i))
                FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));
        } /* end for */
    if((group_success[0] == group_success[ngroups-1])
            && (OPA_load_int(&shared_val) != 0))
        FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));

    /* Verify that the number of successes for the first threads is equal to or
     * one greater than the number of successes for the last thread.  Have
     * already determined that it is not less in the previous test. */
    if(group_success[0] > (group_success[ngroups-1] + 1))
        FAIL_OP_ERROR(printf("    Group 0 succeeded %d times more than group %d\n",
                thread_data[0].nsuccess - thread_data[nthreads-1].nsuccess,
                nthreads - 1));

    /* Free memory */
    free(threads);
    free(thread_data);
    free(group_success);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("grouped integer compare-and-swap", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    if(group_success) free(group_success);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_grouped_cas_int() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: grouped_cas_ptr_helper
 *
 * Purpose: Helper (thread) routine for test_grouped_cas_ptr
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Monday, August 10, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *grouped_cas_ptr_helper(void *_udata)
{
    grouped_cas_ptr_t   *udata = (grouped_cas_ptr_t *)_udata;
    int                 *group_id = udata->groupno;
    int                 *next_id = group_id + 1;
    unsigned            niter = GROUPED_CAS_PTR_NITER;
    unsigned            i;

    /* This is the equivalent of the "modulus" operation, but for pointers */
    if(next_id > udata->max_groupno)
        next_id = (int *) 0;

    /* Main loop */
    for(i=0; i<niter; i++)
        if(OPA_cas_ptr(udata->shared_val, (void *) group_id, (void *) next_id) == (void *) group_id)
            udata->nsuccess++;

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end grouped_cas_ptr_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_grouped_cas_ptr
 *
 * Purpose: Tests atomicity of OPA_cas_ptr.  Launches nthreads threads,
 *          subdivided into nthreads/4 or 2 groups, whichever is greater.
 *          Each thread continually tries to compare-and-swap a shared
 *          value with its group id (oldv) and group id + 1 % ngroups
 *          (newv).
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Monday, August 10, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_grouped_cas_ptr(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    grouped_cas_ptr_t   *thread_data = NULL; /* User data structs for threads */
    OPA_ptr_t           shared_val;     /* Integer shared between threads */
    int                 ngroups;        /* Number of groups of threads */
    int                 threads_per_group; /* Threads per group */
    int                 *group_success = NULL; /* Number of successes for each group */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("grouped pointer compare-and-swap", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data =
            (grouped_cas_ptr_t *) calloc(nthreads, sizeof(grouped_cas_ptr_t))))
        TEST_ERROR;

    /* Calculate number of groups and threads per group */
    if((nthreads / GROUPED_CAS_PTR_TPG) >= 2)
        threads_per_group = GROUPED_CAS_PTR_TPG;
    else
        threads_per_group = (nthreads + 1) / 2;
    ngroups = (nthreads + threads_per_group - 1) / threads_per_group;

    /* Initialize thread data structs */
    OPA_store_ptr(&shared_val, (void *) 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].groupno = (int *) 0 + (i / threads_per_group);
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;
    for(i=0; i<nthreads; i++)
        thread_data[i].max_groupno = thread_data[nthreads-1].groupno;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, grouped_cas_ptr_helper,
                &thread_data[i])) TEST_ERROR;
    (void)grouped_cas_ptr_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Calculate the total number of successes for each group */
    if(NULL == (group_success = (int *) calloc(ngroups, sizeof(int))))
        TEST_ERROR;
    for(i=0; i<nthreads; i++)
        group_success[(int) (thread_data[i].groupno - (int *) 0)] +=
                thread_data[i].nsuccess;

    /* Verify that cas succeeded at least once */
    if(group_success[0] == 0)
        FAIL_OP_ERROR(printf("    Compare-and-swap never succeeded\n"));

    /* Verify that the number of successes does not increase with increasing
     * group number, and also that the shared value's final value is consistent
     * with the numbers of successes */
    if(nthreads > 1)
        for(i=1; i<ngroups; i++) {
            if(group_success[i] > group_success[i-1])
                FAIL_OP_ERROR(printf("    Group %d succeeded more times than group %d\n",
                        i, i-1));

            if((group_success[i] < group_success[i-1])
                    && (OPA_load_ptr(&shared_val) != ((int *) 0) + i))
                FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));
        } /* end for */
    if((group_success[0] == group_success[ngroups-1])
            && (OPA_load_ptr(&shared_val) != (int *) 0))
        FAIL_OP_ERROR(printf("    Number of successes is inconsistent\n"));

    /* Verify that the number of successes for the first threads is equal to or
     * one greater than the number of successes for the last thread.  Have
     * already determined that it is not less in the previous test. */
    if(group_success[0] > (group_success[ngroups-1] + 1))
        FAIL_OP_ERROR(printf("    Group 0 succeeded %d times more than group %d\n",
                thread_data[0].nsuccess - thread_data[nthreads-1].nsuccess,
                nthreads - 1));

    /* Free memory */
    free(threads);
    free(thread_data);
    free(group_success);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("grouped pointer compare-and-swap", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    if(group_success) free(group_success);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_grouped_cas_ptr() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_cas_int_fairness_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_cas_int_fairness
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_cas_int_fairness_helper(void *_udata)
{
    cas_int_fairness_t  *udata = (cas_int_fairness_t *)_udata;
    int                 thread_id = udata->threadno;
    unsigned            nsuccess = 0;
    unsigned            min_success = CAS_INT_FAIRNESS_MIN_SUCCESS;
    unsigned            max_iter = CAS_INT_FAIRNESS_MAX_ITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<max_iter; i++) {
        /* Attempt to set the shared value to the thread id */
        if(OPA_cas_int(udata->shared_val, -1, thread_id) == -1) {

            /* Increment success counter */
            nsuccess++;

            /* Check for completion */
            if(nsuccess == min_success)
                OPA_incr_int(udata->successful_threads);

            /* Reset shared value */
            if(OPA_cas_int(udata->shared_val, thread_id, -1) != thread_id)
                udata->nerrors++;
        } /* end if */

        /* Check for global completion */
        if(udata->nthreads == OPA_load_int(udata->successful_threads))
            break;

        /* Yield if we are not performing strict fairness checks.  This gives
         * other threads a chance and reduces the probability of a thread being
         * stopped for a long period of time after succeeding but before
         * resetting the shared value, preventing forward progress. */
#ifndef OPA_HAVE_STRICT_FAIRNESS_CHECKS
        OPA_TEST_YIELD();
#endif /* OPA_HAVE_STRICT_FAIRNESS_CHECKS */
    } /* end for */

    /* Check if the loop was terminated due to hitting the MAX_ITER barrier,
     * throw a warning if it was */
    if(i == max_iter)
        udata->terminated = 1;

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_cas_int_fairness_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_cas_int_fairness
 *
 * Purpose: Tests fairness of OPA_cas_int.  Launches nthreads threads,
 *          each of which continually tries to compare-and-swap a shared
 *          value with -1 (oldv) and thread id(newv), then sets it back to
 *          -1.  Each thread continues until all threads have succeeded
 *          CAS_INT_FAIRNESS_MIN_SUCCESS times.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, August 11, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_cas_int_fairness(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    cas_int_fairness_t  *thread_data = NULL; /* User data structs for threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    OPA_int_t           successful_threads; /* Number of threads that have succeeded */
    int                 succeeded = 0;  /* Whether all threads succeeded */
    int                 terminated = 0; /* Number of threads that were terminated */
    int                 nerrors = 0;    /* Number of errors */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("integer compare-and-swap fairness", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (cas_int_fairness_t *) calloc(nthreads, sizeof(cas_int_fairness_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, -1);
    OPA_store_int(&successful_threads, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].threadno = i;
        thread_data[i].nthreads = (int) nthreads;
        thread_data[i].successful_threads = &successful_threads;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_cas_int_fairness_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_cas_int_fairness_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Check if any errors were reported by the threads */
    for(i=0; i<nthreads; i++)
        nerrors += thread_data[i].nerrors;
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d unexpected return%s from OPA_cas_int\n",
                nerrors, nerrors == 1 ? "" : "s"));

    /* Verify that all threads succeeded */
    if(OPA_load_int(&successful_threads) == (int) nthreads)
        succeeded = 1;

    /* Check if any threads were terminated before succeeding */
    for(i=0; i<nthreads; i++)
        terminated += thread_data[i].terminated;

    /* If no threads were terminated, but not all succeeded, something is very
     * wrong... */
    if(!succeeded && !terminated)
        FAIL_OP_ERROR(printf("    Not all threads succeeded, but none were terminated\n"));

    /* Otherwise, throw a warning if any threads were terminated */
    if(terminated)
    {
        WARNING();
        if(succeeded)
            printf("    %d thread%s were terminated before succeeding: there may be an issue with\n    fairness\n",
                    terminated, terminated == 1 ? "" : "s");
        else {
            i = nthreads - OPA_load_int(&successful_threads);
            printf("    %d thread%s did not succeed: there may be an issue with fairness\n",
                    i, i == 1 ? "" : "s");
        } /* end else */
    } /* end if */

    /* Free memory */
    free(threads);
    free(thread_data);

    if(!terminated)
        PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("integer compare-and-swap fairness", 0);
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
} /* end test_threaded_cas_int_fairness() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_cas_ptr_fairness_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_cas_ptr_fairness
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_cas_ptr_fairness_helper(void *_udata)
{
    cas_ptr_fairness_t  *udata = (cas_ptr_fairness_t *)_udata;
    int                 *thread_id = udata->threadno;
    unsigned            nsuccess = 0;
    unsigned            min_success = CAS_PTR_FAIRNESS_MIN_SUCCESS;
    unsigned            max_iter = CAS_PTR_FAIRNESS_MAX_ITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<max_iter; i++) {
        /* Attempt to set the shared value to the thread id */
        if(OPA_cas_ptr(udata->shared_val, (void *) 0, thread_id) == (void *) 0) {

            /* Increment success counter */
            nsuccess++;

            /* Check for completion */
            if(nsuccess == min_success)
                OPA_incr_int(udata->successful_threads);

            /* Reset shared value */
            if(OPA_cas_ptr(udata->shared_val, thread_id, (void *) 0) != (void *) thread_id)
                udata->nerrors++;
        } /* end if */

        /* Check for global completion */
        if(udata->nthreads == OPA_load_int(udata->successful_threads))
            break;

        /* Yield if we are not performing strict fairness checks.  This gives
         * other threads a chance and reduces the probability of a thread being
         * stopped for a long period of time after succeeding but before
         * resetting the shared value, preventing forward progress. */
#ifndef OPA_HAVE_STRICT_FAIRNESS_CHECKS
        OPA_TEST_YIELD();
#endif /* OPA_HAVE_STRICT_FAIRNESS_CHECKS */
    } /* end for */

    /* Check if the loop was terminated due to hitting the MAX_ITER barrier,
     * throw a warning if it was */
    if(i == max_iter)
        udata->terminated = 1;

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_cas_ptr_fairness_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_cas_ptr_fairness
 *
 * Purpose: Tests fairness of OPA_cas_int.  Launches nthreads threads,
 *          each of which continually tries to compare-and-swap a shared
 *          value with -1 (oldv) and thread id(newv), then sets it back to
 *          -1.  Each thread continues until all threads have succeeded
 *          CAS_INT_FAIRNESS_MIN_SUCCESS times.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, August 11, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_cas_ptr_fairness(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    cas_ptr_fairness_t  *thread_data = NULL; /* User data structs for threads */
    OPA_ptr_t           shared_val;     /* Integer shared between threads */
    OPA_int_t           successful_threads; /* Number of threads that have succeeded */
    int                 succeeded = 0;  /* Whether all threads succeeded */
    int                 terminated = 0; /* Number of threads that were terminated */
    int                 nerrors = 0;    /* Number of errors */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("pointer compare-and-swap fairness", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (cas_ptr_fairness_t *) calloc(nthreads, sizeof(cas_ptr_fairness_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_ptr(&shared_val, (void *) 0);
    OPA_store_int(&successful_threads, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].threadno = (void *) 0 + i + 1;
        thread_data[i].nthreads = (int) nthreads;
        thread_data[i].successful_threads = &successful_threads;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_cas_ptr_fairness_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_cas_ptr_fairness_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Check if any errors were reported by the threads */
    for(i=0; i<nthreads; i++)
        nerrors += thread_data[i].nerrors;
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d unexpected return%s from OPA_cas_ptr\n",
                nerrors, nerrors == 1 ? "" : "s"));

    /* Verify that all threads succeeded */
    if(OPA_load_int(&successful_threads) == (int) nthreads)
        succeeded = 1;

    /* Check if any threads were terminated before succeeding */
    for(i=0; i<nthreads; i++)
        terminated += thread_data[i].terminated;

    /* If no threads were terminated, but not all succeeded, something is very
     * wrong... */
    if(!succeeded && !terminated)
        FAIL_OP_ERROR(printf("    Not all threads succeeded, but none were terminated\n"));

    /* Otherwise, throw a warning if any threads were terminated */
    if(terminated)
    {
        WARNING();
        if(succeeded)
            printf("    %d thread%s were terminated before succeeding: there may be an issue with\n    fairness\n",
                    terminated, terminated == 1 ? "" : "s");
        else {
            i = nthreads - OPA_load_int(&successful_threads);
            printf("    %d thread%s did not succeed: there may be an issue with fairness\n",
                    i, i == 1 ? "" : "s");
        } /* end else */
    } /* end if */

    /* Free memory */
    free(threads);
    free(thread_data);

    if(!terminated)
        PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("pointer compare-and-swap fairness", 0);
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
} /* end test_threaded_cas_ptr_fairness() */


/*-------------------------------------------------------------------------
 * Function: test_simple_swap_int
 *
 * Purpose: Tests basic functionality of OPA_swap_int with a single
 *          thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, March 24, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_swap_int(void)
{
    OPA_int_t   a;

    TESTING("simple integer swap functionality", 0);

    /* Store 0 in a */
    OPA_store_int(&a, 0);

    /* Compare and swap multiple times, verify return value and final result */
    if(0 != OPA_swap_int(&a, INT_MAX)) TEST_ERROR;
    if(INT_MAX != OPA_swap_int(&a, INT_MIN)) TEST_ERROR;
    if(INT_MIN != OPA_swap_int(&a, 1)) TEST_ERROR;
    if(1 != OPA_load_int(&a)) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_simple_swap_int() */


/*-------------------------------------------------------------------------
 * Function: test_simple_swap_ptr
 *
 * Purpose: Tests basic functionality of OPA_swap_ptr with a single
 *          thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, March 24, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_swap_ptr(void)
{
    OPA_ptr_t   a;
    void        *ptr1 = malloc(1);      /* Pointers to assign to a */
    void        *ptr2 = malloc(1);
    void        *ptr3 = malloc(1);
    void        *ptr4 = malloc(1);

    TESTING("simple pointer swap functionality", 0);

    /* Store ptr1 in a */
    OPA_store_ptr(&a, ptr1);

    /* Compare and swap multiple times, verify return value and final result */
    if(ptr1 != OPA_swap_ptr(&a, ptr3)) TEST_ERROR;
    if(ptr3 != OPA_swap_ptr(&a, ptr4)) TEST_ERROR;
    if(ptr4 != OPA_swap_ptr(&a, ptr2)) TEST_ERROR;
    if(ptr2 != OPA_load_ptr(&a)) TEST_ERROR;

    if(ptr1) free(ptr1);
    if(ptr2) free(ptr2);
    if(ptr3) free(ptr3);
    if(ptr4) free(ptr4);

    PASSED();
    return 0;

error:
    if(ptr1) free(ptr1);
    if(ptr2) free(ptr2);
    if(ptr3) free(ptr3);
    if(ptr4) free(ptr4);

    return 1;
} /* end test_simple_swap_ptr() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_swap_int_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_swap_int
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, March 24, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_swap_int_helper(void *_udata)
{
    swap_int_t          *udata = (swap_int_t *)_udata;
    unsigned            niter = SWAP_INT_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        udata->local_val = OPA_swap_int(udata->shared_val, udata->local_val);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_swap_int_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_swap_int
 *
 * Purpose: Tests atomicity of OPA_swap_int.  Launches nthreads threads,
 *          each of which continually swaps a shared value with a local
 *          variable whose initial value is the thread id.  Afterwards,
 *          Verifies that each thread id is present exactly once in all of
 *          the thread local values, and the shared value.  The initial
 *          state of the shared value must also be present exactly once in
 *          these places.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_swap_int(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    swap_int_t          *thread_data = NULL; /* User data structs for threads */
    OPA_int_t           shared_val;     /* Integer shared between threads */
    unsigned            *vals = NULL;   /* Local values found */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("integer swap", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (swap_int_t *) calloc(nthreads, sizeof(swap_int_t))))
        TEST_ERROR;

    /* Initialize thread data structs */
    OPA_store_int(&shared_val, nthreads);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].local_val = i;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_swap_int_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_swap_int_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Allocate the array of local values found.  These number 0 to nthreads
     * (1 for each thread id, and nthreads was the original value) */
    if(NULL == (vals = (unsigned *) calloc(nthreads + 1, sizeof(unsigned))))
        TEST_ERROR;

    /* Loop over all local values and the shared value, and total up the number
     * of times each value was found */
    for(i=0; i<nthreads; i++) {
        /* Verify that the value is in range */
        if((thread_data[i].local_val < 0)
                || (thread_data[i].local_val > nthreads))
            FAIL_OP_ERROR(printf("    Local value for thread %u is out of range: %d\n",
                    i, thread_data[i].local_val));

        /* Increment the number of times local_val was encountered */
        vals[thread_data[i].local_val]++;
    } /* end for */
    if((OPA_load_int(&shared_val) < 0)
            || (OPA_load_int(&shared_val) > nthreads))
        FAIL_OP_ERROR(printf("    Shared value is out of range: %d\n",
                OPA_load_int(&shared_val)));
    vals[OPA_load_int(&shared_val)]++;

    /* Verify that each possible value was encountered exactly once */
    for(i=0; i<nthreads; i++)
        if(vals[i] != 1)
            FAIL_OP_ERROR(printf("    Value %u was encountered %u times.  Expected: 1\n",
                    i, vals[i]));

    /* Free memory */
    free(threads);
    free(thread_data);
    free(vals);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("integer swap", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    if(vals) free(vals);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_swap_int() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: threaded_swap_ptr_helper
 *
 * Purpose: Helper (thread) routine for test_threaded_swap_ptr
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, March 24, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_swap_ptr_helper(void *_udata)
{
    swap_ptr_t          *udata = (swap_ptr_t *)_udata;
    unsigned            niter = SWAP_PTR_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++)
        udata->local_val = OPA_swap_ptr(udata->shared_val, udata->local_val);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_swap_ptr_helper() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_threaded_swap_ptr
 *
 * Purpose: Tests atomicity of OPA_swap_ptr.  Launches nthreads threads,
 *          each of which continually swaps a shared value with a local
 *          variable whose initial value is the thread id.  Afterwards,
 *          Verifies that each thread id is present exactly once in all of
 *          the thread local values, and the shared value.  The initial
 *          state of the shared value must also be present exactly once in
 *          these places.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 9, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_swap_ptr(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    swap_ptr_t          *thread_data = NULL; /* User data structs for threads */
    OPA_ptr_t           shared_val;     /* Pointer shared between threads */
    unsigned            *vals = NULL;   /* Local values found */
    unsigned            nthreads = num_threads[curr_test];
    unsigned            i;

    TESTING("pointer swap", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (swap_ptr_t *) calloc(nthreads, sizeof(swap_ptr_t))))
        TEST_ERROR;

    /* Initialize thread data structs.  Each local_val points to an element in
     * thread_data, or NULL (original value) */
    OPA_store_ptr(&shared_val, NULL);
    for(i=0; i<nthreads; i++) {
        thread_data[i].shared_val = &shared_val;
        thread_data[i].local_val = &(thread_data[i]);
        thread_data[i].threadno = i;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, threaded_swap_ptr_helper,
                &thread_data[i])) TEST_ERROR;
    (void)threaded_swap_ptr_helper(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Allocate the array of local values found.  These number 0 to nthreads
     * (1 for each thread id, and nthreads was the original value) */
    if(NULL == (vals = (unsigned *) calloc(nthreads + 1, sizeof(unsigned))))
        TEST_ERROR;

    /* Loop over all local values and the shared value, and total up the number
     * of times each value was found */
    for(i=0; i<nthreads; i++) {
        /* Increment the number of times local_val was encountered.  Use the
         * threadno of the target thread_data element as the index into vals.
         * If local_val is NULL, use nthreads as the index. */
        if(thread_data[i].local_val)
            vals[((swap_ptr_t *)thread_data[i].local_val)->threadno]++;
        else
            vals[nthreads]++;
    } /* end for */
    if(OPA_load_ptr(&shared_val))
        vals[((swap_ptr_t *)OPA_load_ptr(&shared_val))->threadno]++;
    else
        vals[nthreads]++;

    /* Verify that each possible value was encountered exactly once */
    for(i=0; i<nthreads; i++)
        if(vals[i] != 1)
            FAIL_OP_ERROR(printf("    Value %d was encountered %d times.  Expected: 1\n",
                    i, vals[i]));

    /* Free memory */
    free(threads);
    free(thread_data);
    free(vals);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("pointer swap", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    if(vals) free(vals);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_threaded_swap_ptr() */


/*-------------------------------------------------------------------------
 * Function: test_simple_llsc_int
 *
 * Purpose: Tests basic functionality of OPA_LL_int and OPA_SC_int with a
 *          single thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 15, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_llsc_int(void)
{
#if defined(OPA_LL_SC_SUPPORTED)
    OPA_int_t   a;
    int         warned = 0;     /* Whether a warning message has been issued */

    TESTING("simple integer load-linked/store-conditional functionality", 0);

    /* Store 0 in a */
    OPA_store_int(&a, 0);

    /* Load linked a */
    if(0 != OPA_LL_int(&a)) TEST_ERROR;

    /* Store conditional INT_MAX in a - should succeed */
    if(!OPA_SC_int(&a, INT_MAX) && !warned) {
        WARNING();
        printf("    Unexpected failure of OPA_SC_int.  LL/SC may be weak\n");
        warned = 1;
        OPA_store_int(&a, INT_MAX);
    } /* end if */

    /* Load linked a */
    if(INT_MAX != OPA_LL_int(&a)) TEST_ERROR;

    /* Store conditional INT_MIN in a - should succeed */
    if(!OPA_SC_int(&a, INT_MIN) && !warned) {
        WARNING();
        printf("    Unexpected failure of OPA_SC_int.  LL/SC may be weak\n");
        warned = 1;
        OPA_store_int(&a, INT_MIN);
    } /* end if */

    /* Load linked a */
    if(INT_MIN != OPA_LL_int(&a)) TEST_ERROR;

    /* Store conditional 0 in a - should succeed */
    if(!OPA_SC_int(&a, 0) && !warned) {
        WARNING();
        printf("    Unexpected failure of OPA_SC_int.  LL/SC may be weak\n");
        warned = 1;
        OPA_store_int(&a, 0);
    } /* end if */

    /* Make sure a contains 0 */
    if(0 != OPA_load_int(&a)) TEST_ERROR;

    if(!warned)
        PASSED();

#else /* OPA_LL_SC_SUPPORTED */
    TESTING("simple integer load-linked/store-conditional functionality", 0);
    SKIPPED();
    puts("    LL/SC not available");
#endif /* OPA_LL_SC_SUPPORTED */

    return 0;

#if defined(OPA_LL_SC_SUPPORTED)
error:
    return 1;
#endif /* OPA_LL_SC_SUPPORTED */
} /* end test_simple_llsc_int() */


/*-------------------------------------------------------------------------
 * Function: test_simple_llsc_ptr
 *
 * Purpose: Tests basic functionality of OPA_LL_ptr and OPA_SC_ptr with a
 *          single thread.  Does not test atomicity of operations.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 15, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_simple_llsc_ptr(void)
{
#if defined(OPA_LL_SC_SUPPORTED)
    OPA_ptr_t   a;
    void        *ptr1 = malloc(1);      /* Pointers to assign to a */
    void        *ptr2 = malloc(1);
    void        *ptr3 = malloc(1);
    int         warned = 0;             /* Whether a warning message has been issued */

    TESTING("simple pointer load-linked/store-conditional functionality", 0);

    /* Store ptr1 in a */
    OPA_store_ptr(&a, ptr1);

    /* Load linked a */
    if(ptr1 != OPA_LL_ptr(&a)) TEST_ERROR;

    /* Store conditional ptr2 in a - should succeed */
    if(!OPA_SC_ptr(&a, ptr2) && !warned) {
        WARNING();
        printf("    Unexpected failure of OPA_SC_ptr.  LL/SC may be weak\n");
        warned = 1;
        OPA_store_ptr(&a, ptr2);
    } /* end if */

    /* Load linked a */
    if(ptr2 != OPA_LL_ptr(&a)) TEST_ERROR;

    /* Store conditional ptr3 in a - should succeed */
    if(!OPA_SC_ptr(&a, ptr3) && !warned) {
        WARNING();
        printf("    Unexpected failure of OPA_SC_ptr.  LL/SC may be weak\n");
        warned = 1;
        OPA_store_ptr(&a, ptr3);
    } /* end if */

    /* Load linked a */
    if(ptr3 != OPA_LL_ptr(&a)) TEST_ERROR;

    /* Store conditional ptr1 in a - should succeed */
    if(!OPA_SC_ptr(&a, ptr1) && !warned) {
        WARNING();
        printf("    Unexpected failure of OPA_SC_ptr.  LL/SC may be weak\n");
        warned = 1;
        OPA_store_ptr(&a, ptr1);
    } /* end if */

    /* Make sure a contains ptr1 */
    if(ptr1 != OPA_load_ptr(&a)) TEST_ERROR;

    if(ptr1) free(ptr1);
    if(ptr2) free(ptr2);
    if(ptr3) free(ptr3);

    if(!warned)
        PASSED();

#else /* OPA_LL_SC_SUPPORTED */
    TESTING("simple pointer load-linked/store-conditional functionality", 0);
    SKIPPED();
    puts("    LL/SC not available");
#endif /* OPA_LL_SC_SUPPORTED */

    return 0;

#if defined(OPA_LL_SC_SUPPORTED)
error:
    if(ptr1) free(ptr1);
    if(ptr2) free(ptr2);
    if(ptr3) free(ptr3);

    return 1;
#endif /* OPA_LL_SC_SUPPORTED */
} /* end test_simple_llsc_ptr() */


#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
/*-------------------------------------------------------------------------
 * Function: threaded_llsc_int_aba_helper_0
 *
 * Purpose: Helper (thread) routine 0 for test_threaded_llsc_int_aba
 *
 * Return: Number of errors
 *
 * Programmer: Neil Fortner
 *             Monday, June 21, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int threaded_llsc_int_aba_helper_0(llsc_int_aba_t *udata)
{
    unsigned            niter = LLSC_PTR_ABA_NITER;
    int                 nerrors = 0;    /* Number of errors */
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Initialize the shared value to 0 */
        OPA_store_int(&udata->shared_val, 0);

        /* Load linked the shared value, and verify that it returns 0 */
        if(0 != OPA_LL_int(&udata->shared_val)) {
            OP_SUPPRESS(printf("    Unexpected return from OPA_LL_int\n"), nerrors, 20);
            nerrors++;
        } /* end if */

        /* Point 0 */
        OPA_store_int(&udata->pass_point_0, 1);

        /* Wait until thread 1 passes point 1 */
        while(!OPA_load_int(&udata->pass_point_1));

        /* Store conditional 1 to the shared value */
        if(OPA_SC_int(&udata->shared_val, 1)) {
            /* SC succeeded, make sure that the shared value was not changed by
             * thread 1 */
            if(OPA_load_int(&udata->change_val)) {
                OP_SUPPRESS(printf("    Unexpected success of OPA_SC_int\n"), nerrors, 20);
                nerrors++;
            } else
                udata->nunchanged_val++;

            /* Verify that the shared value contains 1 */
            if(1 != OPA_load_int(&udata->shared_val)) {
                OP_SUPPRESS(printf("    Unexpected return from OPA_load_int after OPA_SC_int success\n"), nerrors, 20);
                nerrors++;
            } /* end if */
        } else {
            /* SC failed, check if it was a false positive */
            if(!OPA_load_int(&udata->change_val)) {
                udata->false_positives++;
                udata->nunchanged_val++;
            } /* end if */

            /* Verify that the shared value contains 0 */
            if(0 != OPA_load_int(&udata->shared_val)) {
                OP_SUPPRESS(printf("    Unexpected return from OPA_load_int after OPA_SC_int failure\n"), nerrors, 20);
                nerrors++;
            } /* end if */
        } /* end else */

        /* Reset pass_point_1 */
        OPA_store_int(&udata->pass_point_1, 0);

        /* Point 2 */
        OPA_store_int(&udata->pass_point_2, 1);
    } /* end for */

    /* Exit */
    return(nerrors);
} /* end threaded_llsc_int_aba_helper_0() */


/*-------------------------------------------------------------------------
 * Function: threaded_llsc_int_aba_helper_1
 *
 * Purpose: Helper (thread) routine 1 for test_threaded_llsc_int_aba
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 22, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_llsc_int_aba_helper_1(void *_udata)
{
    llsc_int_aba_t      *udata = (llsc_int_aba_t *)_udata;
    unsigned            niter = LLSC_PTR_ABA_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Decide if we are changing the shared value in this iteration */
        OPA_store_int(&udata->change_val, rand() % 2);

        /* Wait until thread 0 passes point 0 */
        while(!OPA_load_int(&udata->pass_point_0));

        /* Reset pass_point_0 */
        OPA_store_int(&udata->pass_point_0, 0);

        if(OPA_load_int(&udata->change_val)) {
            /* Set the shared value to 1 then back to 0.  This is the "ABA" part
             * of this test */
            OPA_store_int(&udata->shared_val, 1);
            OPA_store_int(&udata->shared_val, 0);
        } /* end if */

        /* Point 1 */
        OPA_store_int(&udata->pass_point_1, 1);

        /* Wait until thread 0 passes point 2 */
        while(!OPA_load_int(&udata->pass_point_2));

        /* Reset pass_point_2 */
        OPA_store_int(&udata->pass_point_2, 0);
    } /* end for */

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_llsc_int_aba_helper_1() */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */


/*-------------------------------------------------------------------------
 * Function: test_threaded_llsc_int_aba
 *
 * Purpose: Tests functionality of OPA_LL_int and OPA_SC_int.  Launches 2
 *          threads, which manipulate a shared value in such a way that it
 *          is changed to a new value then back to the original value
 *          before the other thread can call OPA_SC_int.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 22, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_llsc_int_aba(void)
{
#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
    pthread_t           thread;         /* Thread */
    pthread_attr_t      ptattr;         /* Thread attributes */
    llsc_int_aba_t      thread_data;    /* User data struct for threads */
    int                 nerrors = 0;    /* Number of errors */
    unsigned            i;

    TESTING("integer LL/SC ABA", 2);

    /* Initialize thread data struct */
    OPA_store_int(&thread_data.shared_val, 0);
    OPA_store_int(&thread_data.pass_point_0, 0);
    OPA_store_int(&thread_data.pass_point_1, 0);
    OPA_store_int(&thread_data.pass_point_2, 0);
    OPA_store_int(&thread_data.change_val, 0);
    thread_data.false_positives = 0;
    thread_data.nunchanged_val = 0;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    if(pthread_create(&thread, &ptattr, threaded_llsc_int_aba_helper_1,
            &thread_data)) TEST_ERROR;
    nerrors = threaded_llsc_int_aba_helper_0(&thread_data);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join thread 1 */
    if(pthread_join(thread, NULL)) TEST_ERROR;

    /* Check if thread 0 returned any errors */
    if(nerrors) TEST_ERROR;

    /* Print a warning if OPA_SC_int never succeeded, otherwise pass */
    if(thread_data.false_positives == thread_data.nunchanged_val) {
        WARNING();
        printf("    OPA_SC_int never succeeded.  LL/SC appears to be weak.\n");
    } else
        PASSED();

    /* Report the number of false positives */
    printf("    False positives: %d / %d\n", thread_data.false_positives,
            thread_data.nunchanged_val);

#else /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
    TESTING("integer LL/SC ABA", 0);
    SKIPPED();
#  if !defined(OPA_HAVE_PTHREAD_H)
    puts("    pthread.h not available");
#  else /* OPA_HAVE_PTHREAD_H */
    puts("    LL/SC not available");
#  endif /* OPA_HAVE_PTHREAD_H */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
error:
    printf("    False positives: %d / %d\n", thread_data.false_positives,
            thread_data.nunchanged_val);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
} /* end test_threaded_llsc_int_aba() */


#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
/*-------------------------------------------------------------------------
 * Function: threaded_llsc_ptr_aba_helper_0
 *
 * Purpose: Helper (thread) routine 0 for test_threaded_llsc_ptr_aba
 *
 * Return: Number of errors
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 22, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int threaded_llsc_ptr_aba_helper_0(llsc_ptr_aba_t *udata)
{
    unsigned            niter = LLSC_INT_ABA_NITER;
    int                 nerrors = 0;    /* Number of errors */
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Initialize the shared value to 0 */
        OPA_store_ptr(&udata->shared_val, (void *) 0);

        /* Load linked the shared value, and verify that it returns 0 */
        if((void *) 0 != OPA_LL_ptr(&udata->shared_val)) {
            OP_SUPPRESS(printf("    Unexpected return from OPA_LL_ptr\n"), nerrors, 20);
            nerrors++;
        } /* end if */

        /* Point 0 */
        OPA_store_int(&udata->pass_point_0, 1);

        /* Wait until thread 1 passes point 1 */
        while(!OPA_load_int(&udata->pass_point_1));

        /* Store conditional 1 to the shared value */
        if(OPA_SC_ptr(&udata->shared_val, (void *) ((int *) 0 + 1))) {
            /* SC succeeded, make sure that the shared value was not changed by
             * thread 1 */
            if(OPA_load_int(&udata->change_val)) {
                OP_SUPPRESS(printf("    Unexpected success of OPA_SC_ptr\n"), nerrors, 20);
                nerrors++;
            } else
                udata->nunchanged_val++;

            /* Verify that the shared value contains 1 */
            if((void *) ((int *) 0 + 1) != OPA_load_ptr(&udata->shared_val)) {
                OP_SUPPRESS(printf("    Unexpected return from OPA_load_ptr after OPA_SC_int success\n"), nerrors, 20);
                nerrors++;
            } /* end if */
        } else {
            /* SC failed, check if it was a false positive */
            if(!OPA_load_int(&udata->change_val)) {
                udata->false_positives++;
                udata->nunchanged_val++;
            } /* end if */

            /* Verify that the shared value contains 0 */
            if((void *) 0 != OPA_load_ptr(&udata->shared_val)) {
                OP_SUPPRESS(printf("    Unexpected return from OPA_load_ptr after OPA_SC_int failure\n"), nerrors, 20);
                nerrors++;
            } /* end if */
        } /* end else */

        /* Reset pass_point_1 */
        OPA_store_int(&udata->pass_point_1, 0);

        /* Point 2 */
        OPA_store_int(&udata->pass_point_2, 1);
    } /* end for */

    /* Exit */
    return(nerrors);
} /* end threaded_llsc_ptr_aba_helper_0() */


/*-------------------------------------------------------------------------
 * Function: threaded_llsc_ptr_aba_helper_1
 *
 * Purpose: Helper (thread) routine 1 for test_threaded_llsc_ptr_aba
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 22, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_llsc_ptr_aba_helper_1(void *_udata)
{
    llsc_ptr_aba_t      *udata = (llsc_ptr_aba_t *)_udata;
    unsigned            niter = LLSC_INT_ABA_NITER;
    unsigned            i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Decide if we are changing the shared value in this iteration */
        OPA_store_int(&udata->change_val, rand() % 2);

        /* Wait until thread 0 passes point 0 */
        while(!OPA_load_int(&udata->pass_point_0));

        /* Reset pass_point_0 */
        OPA_store_int(&udata->pass_point_0, 0);

        if(OPA_load_int(&udata->change_val)) {
            /* Set the shared value to 1 then back to 0.  This is the "ABA" part
             * of this test */
            OPA_store_ptr(&udata->shared_val, (void *) ((int *) 0 + 1));
            OPA_store_ptr(&udata->shared_val, (void *) 0);
        } /* end if */

        /* Point 1 */
        OPA_store_int(&udata->pass_point_1, 1);

        /* Wait until thread 0 passes point 2 */
        while(!OPA_load_int(&udata->pass_point_2));

        /* Reset pass_point_2 */
        OPA_store_int(&udata->pass_point_2, 0);
    } /* end for */

    /* Exit */
    pthread_exit(NULL);
} /* end threaded_llsc_ptr_aba_helper_1() */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */


/*-------------------------------------------------------------------------
 * Function: test_threaded_llsc_ptr_aba
 *
 * Purpose: Tests functionality of OPA_LL_ptr and OPA_SC_ptr.  Launches 2
 *          threads, which manipulate a shared value in such a way that it
 *          is changed to a new value then back to the original value
 *          before the other thread can call OPA_SC_ptr.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 22, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_llsc_ptr_aba(void)
{
#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
    pthread_t           thread;         /* Thread */
    pthread_attr_t      ptattr;         /* Thread attributes */
    llsc_ptr_aba_t      thread_data;    /* User data struct for threads */
    int                 nerrors = 0;    /* Number of errors */
    unsigned            i;

    TESTING("pointer LL/SC ABA", 2);

    /* Initialize thread data struct */
    OPA_store_ptr(&thread_data.shared_val, (void *) 0);
    OPA_store_int(&thread_data.pass_point_0, 0);
    OPA_store_int(&thread_data.pass_point_1, 0);
    OPA_store_int(&thread_data.pass_point_2, 0);
    OPA_store_int(&thread_data.change_val, 0);
    thread_data.false_positives = 0;
    thread_data.nunchanged_val = 0;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    if(pthread_create(&thread, &ptattr, threaded_llsc_ptr_aba_helper_1,
            &thread_data)) TEST_ERROR;
    nerrors = threaded_llsc_ptr_aba_helper_0(&thread_data);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join thread 1 */
    if(pthread_join(thread, NULL)) TEST_ERROR;

    /* Check if thread 0 returned any errors */
    if(nerrors) TEST_ERROR;

    /* Print a warning if OPA_SC_int never succeeded, otherwise pass */
    if(thread_data.false_positives == thread_data.nunchanged_val) {
        WARNING();
        printf("    OPA_SC_ptr never succeeded.  LL/SC appears to be weak.\n");
    } else
        PASSED();

    /* Report the number of false positives */
    printf("    False positives: %d / %d\n", thread_data.false_positives,
            thread_data.nunchanged_val);

#else /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
    TESTING("pointer LL/SC ABA", 0);
    SKIPPED();
#  if !defined(OPA_HAVE_PTHREAD_H)
    puts("    pthread.h not available");
#  else /* OPA_HAVE_PTHREAD_H */
    puts("    LL/SC not available");
#  endif /* OPA_HAVE_PTHREAD_H */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
error:
    printf("    False positives: %d / %d\n", thread_data.false_positives,
            thread_data.nunchanged_val);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
} /* end test_threaded_llsc_ptr_aba() */


#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
/*-------------------------------------------------------------------------
 * Function: threaded_llsc_int_stack_push
 *
 * Purpose: "Push" routine for test_threaded_llsc_int_stack
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, June 23, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_llsc_int_stack_push(void *_udata)
{
    llsc_int_stack_t    *udata = (llsc_int_stack_t *)_udata;
    llsc_int_stack_obj_t *obj = udata->objs[udata->obj];
    int                 tmp_head;
    unsigned            niter = LLSC_INT_STACK_NITER;
    unsigned            i, j;

    /* Main loop */
    for(i=0; i<niter && !OPA_load_int(udata->failed); i++) {
        /* Acquire object */
        j = 0;
        while(OPA_cas_int(&obj->on_stack, 0, 1)) {
            OPA_TEST_YIELD();
            if(++j == LLSC_INT_STACK_NLOOP)
                break;
        } /* end while */

        /* Check if we failed to acquire the object */
        if(j == LLSC_INT_STACK_NLOOP)
            continue;

        /* Push object onto stack */
        do {
            tmp_head = OPA_load_int(udata->head);
            OPA_store_int(&obj->next, tmp_head);
        } while(tmp_head != OPA_cas_int(udata->head, tmp_head, udata->obj));

        /* Incremen the number of successful pushes */
        udata->nsuccess++;
    } /* end for */

    /* Mark this thread as having completed */
    OPA_decr_int(udata->npushers);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_llsc_int_stack_push() */

/*-------------------------------------------------------------------------
 * Function: threaded_llsc_int_stack_pop
 *
 * Purpose: "Pop" routine for test_threaded_llsc_int_stack
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, June 23, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_llsc_int_stack_pop(void *_udata)
{
    llsc_int_stack_t    *udata = (llsc_int_stack_t *)_udata;
    llsc_int_stack_obj_t *obj = NULL;
    int                 tmp_obj;
    unsigned            i;

    /* Main loop */
    do {
        /* Load-linked the object */
        if(-1 == (tmp_obj = OPA_LL_int(udata->head))) {
            OPA_TEST_YIELD();
            continue;
        } /* end if */
        obj = udata->objs[tmp_obj];

        /* Get the next object */
        tmp_obj = OPA_load_int(&obj->next);

        /* Store conditional the "next" object as the new head */
        if(OPA_SC_int(udata->head, tmp_obj)) {
            /* Make sure this object was marked as on the stack */
            if(!OPA_load_int(&obj->on_stack)) {
                OP_SUPPRESS(printf("    Popped object was not on the stack\n"),
                        udata->nerrors, 20);
                udata->nerrors++;

                if(udata->nerrors >= 1000000) {
                    /* Cause all threads to break out so the test fails in a
                     * reasonable amount of time */
                    OPA_store_int(udata->failed, 1);
                    break;
                } /* end if */
            } /* end if */

            /* Mark the object as off the stack, release for pushing */
            OPA_store_int(&obj->on_stack, 0);

            /* Increment the number of successful pops */
            udata->nsuccess++;
        } /* end if */
    } while(OPA_load_int(udata->npushers) && !OPA_load_int(udata->failed));

    pthread_exit(NULL);
} /* end threaded_llsc_int_stack_pop() */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */


/*-------------------------------------------------------------------------
 * Function: test_threaded_llsc_int_stack
 *
 * Purpose: Tests atomicity of OPA_LL_int and OPA_SC_int.  Launches
 *          nthreads threads, subdivided into nthreads/3 or 2 groups,
 *          whichever is greater.  One group of threads continually tries
 *          to pop objects off a shared stack using LL/SC, while the
 *          other groups all try to push objects onto the stack.  The
 *          algorithm of the pop routine is such that LL/SC must be able
 *          to detect ABA conditions, or the wrong object might be marked
 *          as the head of the stack.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, June 23, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_llsc_int_stack(void)
{
#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    llsc_int_stack_t    *thread_data = NULL; /* User data structs for threads */
    llsc_int_stack_obj_t **objs = NULL; /* Array of pointers to objects to put on stack */
    int                 nobjs;          /* Number of objects */
    OPA_int_t           head;           /* The head of the stack */
    OPA_int_t           npushers;       /* Number of "push" threads running */
    OPA_int_t           failed;         /* Whether the test has failed and must be terminated */
    int                 threads_per_group; /* Threads per group */
    int                 npops = 0;      /* Total number of times an object was popped */
    int                 npushes = 0;    /* Total number of times an object was pushed */
    int                 non_stack = 0;  /* Number of objects on the stack */
    int                 nerrors = 0;    /* Number of errors returned from threads */
    int                 warned = 0;     /* Whether a warning message has been issued */
    unsigned            nthreads = num_threads[curr_test];
    int                 i;

    /* Don't test with only 1 thread */
    if(nthreads < 2)
        return 0;

    TESTING("integer LL/SC stack", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data =
            (llsc_int_stack_t *) calloc(nthreads, sizeof(llsc_int_stack_t))))
        TEST_ERROR;

    /* Calculate number of groups and threads per group */
    if((nthreads / LLSC_INT_STACK_TPG) >= 2)
        threads_per_group = LLSC_INT_STACK_TPG;
    else
        threads_per_group = (nthreads + 1) / 2;
    nobjs = (nthreads - 1) / threads_per_group;
    OPA_store_int(&npushers, nthreads - threads_per_group);

    /* Allocate objects.  Allocate each object individually instead of one large
     * array to add a random component to which objects share a cache line.  The
     * "objs" array is also used to allow integers to function as "pointers" to
     * objects, by being an index into this array. */
    if(NULL == (objs = (llsc_int_stack_obj_t **) malloc(nobjs * sizeof(llsc_int_stack_obj_t *))))
        TEST_ERROR;
    for(i=0; i<nobjs; i++) {
        if(NULL == (objs[i] = (llsc_int_stack_obj_t *) malloc(sizeof(llsc_int_stack_obj_t))))
            TEST_ERROR;
        OPA_store_int(&objs[i]->on_stack, 0);
        OPA_store_int(&objs[i]->next, -1);
    } /* end for */

    /* Initialize thread data structs */
    OPA_store_int(&head, -1);
    OPA_store_int(&failed, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].head = &head;
        thread_data[i].objs = objs;
        thread_data[i].obj = (i / threads_per_group) - 1;
        thread_data[i].npushers = &npushers;
        thread_data[i].failed = &failed;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++) {
        if(i < threads_per_group) {
            if(pthread_create(&threads[i], &ptattr, threaded_llsc_int_stack_pop,
                    &thread_data[i])) TEST_ERROR;
        } else
            if(pthread_create(&threads[i], &ptattr, threaded_llsc_int_stack_push,
                    &thread_data[i])) TEST_ERROR;
    } /* end for */
    (void)threaded_llsc_int_stack_push(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Check for any errors returned from any threads */
    for(i=0; i<nthreads; i++)
        nerrors += thread_data[i].nerrors;
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d error%s encountered in thread routines\n", nerrors,
                nerrors == 1 ? "" : "s"));

    /* Walk the stack, and make sure it is consistent */
    i = OPA_load_int(&head);
    while(i >= 0 && i < nobjs) {
        /* Make sure it is marked as "on_stack" */
        if(1 != OPA_load_int(&objs[i]->on_stack))
            FAIL_OP_ERROR(printf("    Unexpected value of \"on stack\" for object %d: %d Expected: 1\n",
                    i, OPA_load_int(&objs[i]->on_stack)));

        /* Change "on_stack" to 2, so if it appears again it will fail */
        OPA_store_int(&objs[i]->on_stack, 2);

        /* Advance to next object in stack */
        non_stack++;
        i = OPA_load_int(&objs[i]->next);
    } /* end while */

    /* Make sure the last "next" pointer was -1 */
    if(i != -1)
        FAIL_OP_ERROR(printf("    Last \"next\" pointer was %d Expected: -1\n", i));

    /* Walk through all of the objects and make sure that they are all either
     * off the stack or have been encountered by walking the stack */
    for(i=0; i<nobjs; i++)
        if(OPA_load_int(&objs[i]->on_stack) != 0
                && OPA_load_int(&objs[i]->on_stack) != 2)
            FAIL_OP_ERROR(printf("    After walking the stack, object %d's \"on_stack\" set to %d Expected: 0 or 2\n",
                    i, OPA_load_int(&objs[i]->on_stack)));

    /* Make sure the number of successessful pops is consistent with the number
     * of pushes */
    for(i=0; i<threads_per_group; i++)
        npops += thread_data[i].nsuccess;
    for(i=threads_per_group; i<nthreads; i++)
        npushes += thread_data[i].nsuccess;
    if(npops != npushes - non_stack)
        FAIL_OP_ERROR(printf("    Unexpected number of pops: %d Expected: %d\n",
                npops, npushes - non_stack));

    /* Check if there were any failed pushes (this is a warning, not an error)
     */
    if(npushes != (nthreads - threads_per_group) * LLSC_INT_STACK_NITER) {
        WARNING();
        printf("    Only %d/%d pushes succeeded.  There may be an issue with fairness.\n",
                npushes, (nthreads - threads_per_group) * LLSC_INT_STACK_NITER);
        warned = 1;
    } /* end if */

    /* Free memory */
    for(i=0; i<nobjs; i++)
        free(objs[i]);
    free(objs);
    free(threads);
    free(thread_data);

    if(!warned)
        PASSED();

#else /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
    TESTING("integer LL/SC stack", 0);
    SKIPPED();
#  if !defined(OPA_HAVE_PTHREAD_H)
    puts("    pthread.h not available");
#  else /* OPA_HAVE_PTHREAD_H */
    puts("    LL/SC not available");
#  endif /* OPA_HAVE_PTHREAD_H */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
error:
    if(objs) {
        for(i=0; i<nobjs; i++)
            if(objs[i])
                free(objs[i]);
        free(objs);
    } /* end if */
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
} /* end test_threaded_llsc_int_stack() */


#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
/*-------------------------------------------------------------------------
 * Function: threaded_llsc_ptr_stack_push
 *
 * Purpose: "Push" routine for test_threaded_llsc_ptr_stack
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Friday, June 25, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_llsc_ptr_stack_push(void *_udata)
{
    llsc_ptr_stack_t    *udata = (llsc_ptr_stack_t *)_udata;
    llsc_ptr_stack_obj_t *obj = udata->obj;
    llsc_ptr_stack_obj_t *tmp_head = NULL;
    unsigned            niter = LLSC_PTR_STACK_NITER;
    unsigned            i, j;

    /* Main loop */
    for(i=0; i<niter && !OPA_load_int(udata->failed); i++) {
        /* Acquire object */
        j = 0;
        while(OPA_cas_int(&obj->on_stack, 0, 1)) {
            OPA_TEST_YIELD();
            if(++j == LLSC_PTR_STACK_NLOOP)
                break;
        } /* end while */

        /* Check if we failed to acquire the object */
        if(j == LLSC_PTR_STACK_NLOOP)
            continue;

        /* Push object onto stack */
        do {
            tmp_head = OPA_load_ptr(udata->head);
            OPA_store_ptr(&obj->next, tmp_head);
        } while(tmp_head != OPA_cas_ptr(udata->head, tmp_head, obj));

        /* Incremen the number of successful pushes */
        udata->nsuccess++;
    } /* end for */

    /* Mark this thread as having completed */
    OPA_decr_int(udata->npushers);

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end threaded_llsc_ptr_stack_push() */

/*-------------------------------------------------------------------------
 * Function: threaded_llsc_ptr_stack_pop
 *
 * Purpose: "Pop" routine for test_threaded_llsc_ptr_stack
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Friday, June 25, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *threaded_llsc_ptr_stack_pop(void *_udata)
{
    llsc_ptr_stack_t    *udata = (llsc_ptr_stack_t *)_udata;
    llsc_ptr_stack_obj_t *obj = NULL;
    llsc_ptr_stack_obj_t *next = NULL;
    unsigned            i;

    /* Main loop */
    do {
        /* Load-linked the object */
        if(NULL == (obj = OPA_LL_ptr(udata->head))) {
            OPA_TEST_YIELD();
            continue;
        } /* end if */

        /* Get the next object */
        next = OPA_load_ptr(&obj->next);

        /* Store conditional the "next" object as the new head */
        if(OPA_SC_ptr(udata->head, next)) {
            /* Make sure this object was marked as on the stack */
            if(!OPA_load_int(&obj->on_stack)) {
                OP_SUPPRESS(printf("    Popped object was not on the stack\n"),
                        udata->nerrors, 20);
                udata->nerrors++;

                if(udata->nerrors >= 1000000) {
                    /* Cause all threads to break out so the test fails in a
                     * reasonable amount of time */
                    OPA_store_int(udata->failed, 1);
                    break;
                } /* end if */
            } /* end if */

            /* Mark the object as off the stack, release for pushing */
            OPA_store_int(&obj->on_stack, 0);

            /* Increment the number of successful pops */
            udata->nsuccess++;
        } /* end if */
    } while(OPA_load_int(udata->npushers) && !OPA_load_int(udata->failed));

    pthread_exit(NULL);
} /* end threaded_llsc_ptr_stack_pop() */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */


/*-------------------------------------------------------------------------
 * Function: test_threaded_llsc_ptr_stack
 *
 * Purpose: Tests atomicity of OPA_LL_ptr and OPA_SC_ptr.  Launches
 *          nthreads threads, subdivided into nthreads/3 or 2 groups,
 *          whichever is greater.  One group of threads continually tries
 *          to pop objects off a shared stack using LL/SC, while the
 *          other groups all try to push objects onto the stack.  The
 *          algorithm of the pop routine is such that LL/SC must be able
 *          to detect ABA conditions, or the wrong object might be marked
 *          as the head of the stack.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Friday, June 25, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_threaded_llsc_ptr_stack(void)
{
#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    llsc_ptr_stack_t    *thread_data = NULL; /* User data structs for threads */
    llsc_ptr_stack_obj_t **objs = NULL; /* Array of pointers to objects to put on stack */
    llsc_ptr_stack_obj_t *obj = NULL;   /* Object on stack (used while walking the stack) */
    int                 nobjs;          /* Number of objects */
    OPA_ptr_t           head;           /* The head of the stack */
    OPA_int_t           npushers;       /* Number of "push" threads running */
    OPA_int_t           failed;         /* Whether the test has failed and must be terminated */
    int                 threads_per_group; /* Threads per group */
    int                 npops = 0;      /* Total number of times an object was popped */
    int                 npushes = 0;    /* Total number of times an object was pushed */
    int                 non_stack = 0;  /* Number of objects on the stack */
    int                 nerrors = 0;    /* Number of errors returned from threads */
    int                 warned = 0;     /* Whether a warning message has been issued */
    unsigned            nthreads = num_threads[curr_test];
    int                 i;

    /* Don't test with only 1 thread */
    if(nthreads < 2)
        return 0;

    TESTING("pointer LL/SC stack", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc((nthreads - 1) * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data =
            (llsc_ptr_stack_t *) calloc(nthreads, sizeof(llsc_ptr_stack_t))))
        TEST_ERROR;

    /* Calculate number of groups and threads per group */
    if((nthreads / LLSC_PTR_STACK_TPG) >= 2)
        threads_per_group = LLSC_PTR_STACK_TPG;
    else
        threads_per_group = (nthreads + 1) / 2;
    nobjs = (nthreads - 1) / threads_per_group;
    OPA_store_int(&npushers, nthreads - threads_per_group);

    /* Allocate objects.  Allocate each object individually instead of one large
     * array to add a random component to which objects share a cache line.  The
     * "objs" array is also used to allow integers to function as "pointers" to
     * objects, by being an index into this array. */
    if(NULL == (objs = (llsc_ptr_stack_obj_t **) malloc(nobjs * sizeof(llsc_ptr_stack_obj_t *))))
        TEST_ERROR;
    for(i=0; i<nobjs; i++) {
        if(NULL == (objs[i] = (llsc_ptr_stack_obj_t *) malloc(sizeof(llsc_ptr_stack_obj_t))))
            TEST_ERROR;
        OPA_store_int(&objs[i]->on_stack, 0);
        OPA_store_ptr(&objs[i]->next, NULL);
    } /* end for */

    /* Initialize thread data structs */
    OPA_store_ptr(&head, NULL);
    OPA_store_int(&failed, 0);
    for(i=0; i<nthreads; i++) {
        thread_data[i].head = &head;
        if(i >= threads_per_group)
            thread_data[i].obj = objs[(i / threads_per_group) - 1];
        thread_data[i].npushers = &npushers;
        thread_data[i].failed = &failed;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Create the threads */
    for(i=0; i<(nthreads - 1); i++) {
        if(i < threads_per_group) {
            if(pthread_create(&threads[i], &ptattr, threaded_llsc_ptr_stack_pop,
                    &thread_data[i])) TEST_ERROR;
        } else
            if(pthread_create(&threads[i], &ptattr, threaded_llsc_ptr_stack_push,
                    &thread_data[i])) TEST_ERROR;
    } /* end for */
    (void)threaded_llsc_ptr_stack_push(&thread_data[i]);

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++)
        if(pthread_join(threads[i], NULL)) TEST_ERROR;

    /* Check for any errors returned from any threads */
    for(i=0; i<nthreads; i++)
        nerrors += thread_data[i].nerrors;
    if(nerrors)
        FAIL_OP_ERROR(printf("    %d error%s encountered in thread routines\n", nerrors,
                nerrors == 1 ? "" : "s"));

    /* Walk the stack, and make sure it is consistent */
    obj = OPA_load_ptr(&head);
    while(obj) {
        /* Make sure it is marked as "on_stack" */
        if(1 != OPA_load_int(&obj->on_stack))
            FAIL_OP_ERROR(printf("    Unexpected value of \"on stack\" for object %d: %d Expected: 1\n",
                    i, OPA_load_int(&obj->on_stack)));

        /* Change "on_stack" to 2, so if it appears again it will fail */
        OPA_store_int(&obj->on_stack, 2);

        /* Advance to next object in stack */
        non_stack++;
        obj = OPA_load_ptr(&obj->next);
    } /* end while */

    /* Walk through all of the objects and make sure that they are all either
     * off the stack or have been encountered by walking the stack */
    for(i=0; i<nobjs; i++)
        if(OPA_load_int(&objs[i]->on_stack) != 0
                && OPA_load_int(&objs[i]->on_stack) != 2)
            FAIL_OP_ERROR(printf("    After walking the stack, object %d's \"on_stack\" set to %d Expected: 0 or 2\n",
                    i, OPA_load_int(&objs[i]->on_stack)));

    /* Make sure the number of successessful pops is consistent with the number
     * of pushes */
    for(i=0; i<threads_per_group; i++)
        npops += thread_data[i].nsuccess;
    for(i=threads_per_group; i<nthreads; i++)
        npushes += thread_data[i].nsuccess;
    if(npops != npushes - non_stack)
        FAIL_OP_ERROR(printf("    Unexpected number of pops: %d Expected: %d\n",
                npops, npushes - non_stack));

    /* Check if there were any failed pushes (this is a warning, not an error)
     */
    if(npushes != (nthreads - threads_per_group) * LLSC_PTR_STACK_NITER) {
        WARNING();
        printf("    Only %d/%d pushes succeeded.  There may be an issue with fairness.\n",
                npushes, (nthreads - threads_per_group) * LLSC_PTR_STACK_NITER);
        warned = 1;
    } /* end if */

    /* Free memory */
    for(i=0; i<nobjs; i++)
        free(objs[i]);
    free(objs);
    free(threads);
    free(thread_data);

    if(!warned)
        PASSED();

#else /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
    TESTING("pointer LL/SC stack", 0);
    SKIPPED();
#  if !defined(OPA_HAVE_PTHREAD_H)
    puts("    pthread.h not available");
#  else /* OPA_HAVE_PTHREAD_H */
    puts("    LL/SC not available");
#  endif /* OPA_HAVE_PTHREAD_H */
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H) && defined(OPA_LL_SC_SUPPORTED)
error:
    if(objs) {
        for(i=0; i<nobjs; i++)
            if(objs[i])
                free(objs[i]);
        free(objs);
    } /* end if */
    if(threads) free(threads);
    if(thread_data) free(thread_data);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H && OPA_LL_SC_SUPPORTED */
} /* end test_threaded_llsc_ptr_stack() */


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
    OPA_emulation_ipl_t shm_lock;
    OPA_Interprocess_lock_init(&shm_lock, 1/*isLeader*/);
#endif

    srand((unsigned)time(NULL));

    /* Simple tests */
    nerrors += test_simple_loadstore_int();
    nerrors += test_simple_loadstore_ptr();
    nerrors += test_simple_add_incr_decr();
    nerrors += test_simple_decr_and_test();
    nerrors += test_simple_faa_fai_fad();
    nerrors += test_simple_cas_int();
    nerrors += test_simple_cas_ptr();
    nerrors += test_simple_swap_int();
    nerrors += test_simple_swap_ptr();
    nerrors += test_simple_llsc_int();
    nerrors += test_simple_llsc_ptr();

    /* Threaded tests with a fixed number of threads */
    nerrors += test_threaded_llsc_int_aba();
    nerrors += test_threaded_llsc_ptr_aba();

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
        nerrors += test_threaded_fai_fad();
        nerrors += test_threaded_fai_ret();
        nerrors += test_threaded_fad_ret();
        nerrors += test_threaded_cas_int();
        nerrors += test_threaded_cas_ptr();
        nerrors += test_grouped_cas_int();
        nerrors += test_grouped_cas_ptr();
        nerrors += test_threaded_cas_int_fairness();
        nerrors += test_threaded_cas_ptr_fairness();
        nerrors += test_threaded_swap_int();
        nerrors += test_threaded_swap_ptr();
        nerrors += test_threaded_llsc_int_stack();
        nerrors += test_threaded_llsc_ptr_stack();
    } /* end for */

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

