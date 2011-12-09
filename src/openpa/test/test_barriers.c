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

/*
 * Uncomment these lines to disable only memory barriers, while leaving the rest
 * of the OPA functions intact.
 */
/*
#ifdef OPA_write_barrier
#undef OPA_write_barrier
#endif
#define OPA_write_barrier() ((void) 0)
#ifdef OPA_read_barrier
#undef OPA_read_barrier
#endif
#define OPA_read_barrier() ((void) 0)
#ifdef OPA_read_write_barrier
#undef OPA_read_write_barrier
#endif
#define OPA_read_write_barrier() ((void) 0)
*/

/* Definitions for test_barriers_linear_array */
#define LINEAR_ARRAY_NITER 4000000
#define LINEAR_ARRAY_LEN 100
typedef struct {
    OPA_int_t *shared_array;
    int       master_thread;  /* Whether this is the master thread */
} linear_array_t;

/* Definitions for test_barriers_variables */
#define VARIABLES_NITER 4000000
#define VARIABLES_NVAR 10
typedef struct {
    OPA_int_t *v_0;
    OPA_int_t *v_1;
    OPA_int_t *v_2;
    OPA_int_t *v_3;
    OPA_int_t *v_4;
    OPA_int_t *v_5;
    OPA_int_t *v_6;
    OPA_int_t *v_7;
    OPA_int_t *v_8;
    OPA_int_t *v_9;
    int       master_thread;  /* Whether this is the master thread */
} variables_t;

/* Definitions for test_barriers_scattered_array */
#define SCATTERED_ARRAY_SIZE 100000
#define SCATTERED_ARRAY_LOCS {254, 85920, 255, 35529, 75948, 75947, 253, 99999, 11111, 11112}


/*-------------------------------------------------------------------------
 * Function: test_barriers_sanity
 *
 * Purpose: Essentially tests that memory barriers don't interfere with
 *          normal single threaded operations.  If this fails then
 *          something is *very* wrong.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *     goodell@, December 1, 2011: load-acquire/store-release sanity test
 *
 *-------------------------------------------------------------------------
 */
static int test_barriers_sanity(void)
{
    OPA_int_t   a;
    int         b;
    OPA_ptr_t   op;
    void       *p;
    struct {int i;} obj = {0xabcdef};

    TESTING("memory barrier sanity", 0);

    /* Store 0 in a and b */
    OPA_store_int(&a, 0);
    OPA_write_barrier();
    b = 0;

    OPA_read_write_barrier();

    /* Add INT_MIN */
    OPA_add_int(&a, INT_MIN);
    OPA_read_write_barrier();
    b += INT_MIN;

    OPA_read_write_barrier();

    /* Increment */
    OPA_incr_int(&a);
    OPA_read_write_barrier();
    b++;

    OPA_read_write_barrier();

    /* Add INT_MAX */
    OPA_add_int(&a, INT_MAX);
    OPA_read_write_barrier();
    b += INT_MAX;

    OPA_read_write_barrier();

    /* Decrement */
    OPA_decr_int(&a);
    OPA_read_write_barrier();
    b--;

    OPA_read_write_barrier();

    /* Load the result, verify it is correct */
    if(OPA_load_int(&a) != INT_MIN + 1 + INT_MAX - 1) TEST_ERROR;
    OPA_read_barrier();
    if(b != OPA_load_int(&a)) TEST_ERROR;

    OPA_read_write_barrier();

    /* Barriers are now the opposite of what they were before */

    /* Store 0 in a */
    OPA_store_int(&a, 0);
    OPA_read_barrier();
    b = 0;

    /* Add INT_MAX */
    OPA_add_int(&a, INT_MAX);
    b += INT_MAX;

    /* Decrement */
    OPA_decr_int(&a);
    b--;

    /* Add INT_MIN */
    OPA_add_int(&a, INT_MIN);
    b += INT_MIN;

    /* Increment */
    OPA_incr_int(&a);
    b++;

    /* Load the result, verify it is correct */
    if(OPA_load_int(&a) != INT_MAX - 1 + INT_MIN + 1) TEST_ERROR;
    OPA_write_barrier();
    if(b != OPA_load_int(&a)) TEST_ERROR;

    /* now provide a quick sanity check that the load-acquire/store-release code
     * works (as in, successfully compiles and runs single-threaded, no
     * multithreading is checked here) */

    OPA_store_int(&a, 5);
    b = OPA_load_acquire_int(&a);
    if (b != 5) TEST_ERROR;
    OPA_store_release_int(&a, 0);
    b = OPA_load_acquire_int(&a);
    if (b != 0) TEST_ERROR;

    OPA_store_ptr(&op, &obj);
    p = OPA_load_acquire_ptr(&op);
    if (p != &obj) TEST_ERROR;
    OPA_store_release_ptr(&op, NULL);
    p = OPA_load_acquire_ptr(&op);
    if (p != NULL) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_barriers_sanity() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: test_barriers_linear_array_write
 *
 * Purpose: Helper (write thread) routine for test_barriers_linear_array.
 *          Writes successive increments to the shared array with memory
 *          barriers between each increment.
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *test_barriers_linear_array_write(void *_udata)
{
    linear_array_t      *udata = (linear_array_t *)_udata;
    OPA_int_t           *shared_array = udata->shared_array;
    int                 niter = LINEAR_ARRAY_NITER / LINEAR_ARRAY_LEN
                                / iter_reduction[curr_test];
    int                 i, j;

    /* Main loop */
    for(i=0; i<niter; i++)
        for(j=0; j<LINEAR_ARRAY_LEN; j++) {
            /* Increment the value in the array */
            OPA_incr_int(&shared_array[j]);

            /* Write barrier */
            OPA_write_barrier();
        } /* end for */

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end test_barriers_linear_array_write() */


/*-------------------------------------------------------------------------
 * Function: test_barriers_linear_array_read
 *
 * Purpose: Helper (read thread) routine for test_barriers_linear_array.
 *          Reads successive increments from the shared array in reverse
 *          order with memory barriers between each read.
 *
 * Return: Success: NULL
 *         Failure: non-NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *test_barriers_linear_array_read(void *_udata)
{
    linear_array_t      *udata = (linear_array_t *)_udata;
    OPA_int_t           *shared_array = udata->shared_array;
    int                 read_buffer[LINEAR_ARRAY_LEN];
    int                 niter = LINEAR_ARRAY_NITER / LINEAR_ARRAY_LEN
                                / iter_reduction[curr_test];
    int                 nerrors = 0;    /* Number of errors */
    int                 i, j;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Load the values from the array into the read buffer in reverse
         * order */
        for(j = LINEAR_ARRAY_LEN - 1; j >= 0; j--) {
            read_buffer[j] = OPA_load_int(&shared_array[j]);

            /* Read barrier */
            OPA_read_barrier();
        } /* end for */

        /* Verify that the values never increase when read back in forward
        * order */
         for(j=1; j<LINEAR_ARRAY_LEN; j++)
            if(read_buffer[j-1] < read_buffer[j]) {
                printf("    Unexpected load: %d is less than %d\n",
                        read_buffer[j-1], read_buffer[j]);
                nerrors++;
            } /* end if */
    } /* end for */

    /* Any non-NULL exit value indicates an error, we use (void *) 1 here */
    if(udata->master_thread)
        return(nerrors ? (void *) 1 : NULL);
    else
        pthread_exit(nerrors ? (void *) 1 : NULL);
} /* end test_barriers_linear_array_read() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_barriers_linear_array
 *
 * Purpose: Tests memory barriers using simultaneous reads and writes to
 *          a linear array.  Launches nthreads threads split into read
 *          and write threads.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_barriers_linear_array(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    linear_array_t      *thread_data = NULL; /* User data structs for each thread */
    static OPA_int_t    shared_array[LINEAR_ARRAY_LEN]; /* Array to operate on */
    void                *ret;           /* Thread return value */
    unsigned            nthreads = num_threads[curr_test];
    int                 nerrors = 0;    /* number of errors */
    int                 i;

    TESTING("memory barriers with linear array", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (linear_array_t *) calloc(nthreads,
            sizeof(linear_array_t)))) TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Initialize shared array */
    for(i=0; i<LINEAR_ARRAY_LEN; i++)
        OPA_store_int(&shared_array[i], 0);

    /* Initialize thread data structs */
    for(i=0; i<nthreads; i++)
        thread_data[i].shared_array = shared_array;
    thread_data[nthreads-1].master_thread = 1;

    /* Create the threads. */
    for(i=0; i<(nthreads - 1); i++) {
        if(pthread_create(&threads[i], &ptattr, test_barriers_linear_array_write,
                &thread_data[i])) TEST_ERROR;
        if(++i < (nthreads - 1))
            if(pthread_create(&threads[i], &ptattr, test_barriers_linear_array_read,
                    &thread_data[i])) TEST_ERROR;
    } /* end for */
    if(nthreads % 2) {
        if(test_barriers_linear_array_write(&thread_data[(nthreads - 1)]))
            nerrors++;
    } else
        if(test_barriers_linear_array_read(&thread_data[(nthreads - 1)]))
            nerrors++;

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++) {
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

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("memory barriers with linear array", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_barriers_linear_array() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: test_barriers_variables_write
 *
 * Purpose: Helper (write thread) routine for test_barriers_variables.
 *          Writes successive increments to the shared variables with
 *          memory barriers between each increment.
 *
 * Return: NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *test_barriers_variables_write(void *_udata)
{
    variables_t         *udata = (variables_t *)_udata;
    OPA_int_t           *v_0, *v_1, *v_2, *v_3, *v_4, *v_5, *v_6, *v_7, *v_8, *v_9;
    int                 niter = VARIABLES_NITER / VARIABLES_NVAR
                                / iter_reduction[curr_test];
    int                 i;

    /* Make local copies of the pointers in udata, to maximize the chance of the
     * compiler reordering instructions (if the barriers don't work) */
    v_0 = udata->v_0;
    v_1 = udata->v_1;
    v_2 = udata->v_2;
    v_3 = udata->v_3;
    v_4 = udata->v_4;
    v_5 = udata->v_5;
    v_6 = udata->v_6;
    v_7 = udata->v_7;
    v_8 = udata->v_8;
    v_9 = udata->v_9;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Incrememnt the variables in forward order */
        OPA_incr_int(v_0);
        OPA_write_barrier();
        OPA_incr_int(v_1);
        OPA_write_barrier();
        OPA_incr_int(v_2);
        OPA_write_barrier();
        OPA_incr_int(v_3);
        OPA_write_barrier();
        OPA_incr_int(v_4);
        OPA_write_barrier();
        OPA_incr_int(v_5);
        OPA_write_barrier();
        OPA_incr_int(v_6);
        OPA_write_barrier();
        OPA_incr_int(v_7);
        OPA_write_barrier();
        OPA_incr_int(v_8);
        OPA_write_barrier();
        OPA_incr_int(v_9);
        OPA_write_barrier();
    } /* end for */

    /* Exit */
    if(udata->master_thread)
        return(NULL);
    else
        pthread_exit(NULL);
} /* end test_barriers_variables_write() */


/*-------------------------------------------------------------------------
 * Function: test_barriers_variables_read
 *
 * Purpose: Helper (read thread) routine for test_barriers_variables.
 *          Reads successive increments from the variables in reverse
 *          order with memory barriers between each read.
 *
 * Return: Success: NULL
 *         Failure: non-NULL
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *test_barriers_variables_read(void *_udata)
{
    variables_t         *udata = (variables_t *)_udata;
    OPA_int_t           *v_0, *v_1, *v_2, *v_3, *v_4, *v_5, *v_6, *v_7, *v_8, *v_9;
    int                 read_buffer[VARIABLES_NVAR];
    int                 niter = VARIABLES_NITER / VARIABLES_NVAR
                                / iter_reduction[curr_test];
    int                 nerrors = 0;    /* Number of errors */
    int                 i, j;

    /* Make local copies of the pointers in udata, to maximize the chance of the
     * compiler reordering instructions (if the barriers don't work) */
    v_0 = udata->v_0;
    v_1 = udata->v_1;
    v_2 = udata->v_2;
    v_3 = udata->v_3;
    v_4 = udata->v_4;
    v_5 = udata->v_5;
    v_6 = udata->v_6;
    v_7 = udata->v_7;
    v_8 = udata->v_8;
    v_9 = udata->v_9;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* Load the values from the array into the read buffer in reverse
         * order*/
        read_buffer[9] = OPA_load_int(v_9);
        OPA_read_barrier();
        read_buffer[8] = OPA_load_int(v_8);
        OPA_read_barrier();
        read_buffer[7] = OPA_load_int(v_7);
        OPA_read_barrier();
        read_buffer[6] = OPA_load_int(v_6);
        OPA_read_barrier();
        read_buffer[5] = OPA_load_int(v_5);
        OPA_read_barrier();
        read_buffer[4] = OPA_load_int(v_4);
        OPA_read_barrier();
        read_buffer[3] = OPA_load_int(v_3);
        OPA_read_barrier();
        read_buffer[2] = OPA_load_int(v_2);
        OPA_read_barrier();
        read_buffer[1] = OPA_load_int(v_1);
        OPA_read_barrier();
        read_buffer[0] = OPA_load_int(v_0);
        OPA_read_barrier();

        /* Verify that the values never increase when read back in forward
        * order */
         for(j=1; j<VARIABLES_NVAR; j++)
            if(read_buffer[j-1] < read_buffer[j]) {
                printf("    Unexpected load: %d is less than %d\n",
                        read_buffer[j-1], read_buffer[j]);
                nerrors++;
            } /* end if */
    } /* end for */

    /* Any non-NULL exit value indicates an error, we use (void *) 1 here */
    if(udata->master_thread)
        return(nerrors ? (void *) 1 : NULL);
    else
        pthread_exit(nerrors ? (void *) 1 : NULL);
} /* end test_barriers_variables_read() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_barriers_variables
 *
 * Purpose: Tests memory barriers using simultaneous reads and writes to
 *          a linear array.  Launches nthreads threads split into read
 *          and write threads.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_barriers_variables(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    variables_t         *thread_data = NULL; /* User data structs for each thread */
    OPA_int_t           v_0, v_1, v_2, v_3, v_4, v_5, v_6, v_7, v_8, v_9;
    void                *ret;           /* Thread return value */
    unsigned            nthreads = num_threads[curr_test];
    int                 nerrors = 0;    /* number of errors */
    int                 i;

    TESTING("memory barriers with local variables", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (variables_t *) calloc(nthreads,
            sizeof(variables_t)))) TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Initialize shared variables */
    OPA_store_int(&v_0, 0);
    OPA_store_int(&v_1, 0);
    OPA_store_int(&v_2, 0);
    OPA_store_int(&v_3, 0);
    OPA_store_int(&v_4, 0);
    OPA_store_int(&v_5, 0);
    OPA_store_int(&v_6, 0);
    OPA_store_int(&v_7, 0);
    OPA_store_int(&v_8, 0);
    OPA_store_int(&v_9, 0);

    /* Initialize thread data structs */
    for(i=0; i<nthreads; i++) {
        thread_data[i].v_0 = &v_0;
        thread_data[i].v_1 = &v_1;
        thread_data[i].v_2 = &v_2;
        thread_data[i].v_3 = &v_3;
        thread_data[i].v_4 = &v_4;
        thread_data[i].v_5 = &v_5;
        thread_data[i].v_6 = &v_6;
        thread_data[i].v_7 = &v_7;
        thread_data[i].v_8 = &v_8;
        thread_data[i].v_9 = &v_9;
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Create the threads. */
    for(i=0; i<(nthreads - 1); i++) {
        if(pthread_create(&threads[i], &ptattr, test_barriers_variables_write,
                &thread_data[i])) TEST_ERROR;
        if(++i < (nthreads - 1))
            if(pthread_create(&threads[i], &ptattr, test_barriers_variables_read,
                    &thread_data[i])) TEST_ERROR;
    } /* end for */
    if(nthreads % 2) {
        if(test_barriers_variables_write(&thread_data[(nthreads - 1)]))
            nerrors++;
    } else
        if(test_barriers_variables_read(&thread_data[(nthreads - 1)]))
            nerrors++;

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++) {
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

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("memory barriers with local variables", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_barriers_variables() */


/*-------------------------------------------------------------------------
 * Function: test_barriers_scattered_array
 *
 * Purpose: Tests memory barriers using simultaneous reads and writes to
 *          locations scattered in an array.  Launches nthreads threads
 *          split into read and write threads.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Wednesday, April 1, 2009
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_barriers_scattered_array(void)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    variables_t         *thread_data = NULL; /* User data structs for each thread */
    static OPA_int_t    shared_array[SCATTERED_ARRAY_SIZE];
    int                 shared_locs[VARIABLES_NVAR] = SCATTERED_ARRAY_LOCS;
    void                *ret;           /* Thread return value */
    unsigned            nthreads = num_threads[curr_test];
    int                 nerrors = 0;    /* number of errors */
    int                 i;

    TESTING("memory barriers with scattered array", nthreads);

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Allocate array of thread data */
    if(NULL == (thread_data = (variables_t *) calloc(nthreads,
            sizeof(variables_t)))) TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Initialize shared variables */
    for(i=0; i<VARIABLES_NVAR; i++)
        OPA_store_int(&shared_array[shared_locs[i]], 0);

    /* Initialize thread data structs */
    for(i=0; i<nthreads; i++) {
        thread_data[i].v_0 = &shared_array[shared_locs[0]];
        thread_data[i].v_1 = &shared_array[shared_locs[1]];
        thread_data[i].v_2 = &shared_array[shared_locs[2]];
        thread_data[i].v_3 = &shared_array[shared_locs[3]];
        thread_data[i].v_4 = &shared_array[shared_locs[4]];
        thread_data[i].v_5 = &shared_array[shared_locs[5]];
        thread_data[i].v_6 = &shared_array[shared_locs[6]];
        thread_data[i].v_7 = &shared_array[shared_locs[7]];
        thread_data[i].v_8 = &shared_array[shared_locs[8]];
        thread_data[i].v_9 = &shared_array[shared_locs[9]];
    } /* end for */
    thread_data[nthreads-1].master_thread = 1;

    /* Create the threads.  We will use the helper routines for
     * test_barriers_variables. */
    for(i=0; i<(nthreads - 1); i++) {
        if(pthread_create(&threads[i], &ptattr, test_barriers_variables_write,
                &thread_data[i])) TEST_ERROR;
        if(++i < (nthreads - 1))
            if(pthread_create(&threads[i], &ptattr, test_barriers_variables_read,
                    &thread_data[i])) TEST_ERROR;
    } /* end for */
    if(nthreads % 2) {
        if(test_barriers_variables_write(&thread_data[(nthreads - 1)]))
            nerrors++;
    } else
        if(test_barriers_variables_read(&thread_data[(nthreads - 1)]))
            nerrors++;

    /* Free the attribute */
    if(pthread_attr_destroy(&ptattr)) TEST_ERROR;

    /* Join the threads */
    for (i=0; i<(nthreads - 1); i++) {
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

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("memory barriers with scattered array", 0);
    SKIPPED();
    puts("    pthread.h not available");
#endif /* OPA_HAVE_PTHREAD_H */

    return 0;

#if defined(OPA_HAVE_PTHREAD_H)
error:
    if(threads) free(threads);
    return 1;
#endif /* OPA_HAVE_PTHREAD_H */
} /* end test_barriers_scattered_array() */


/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     Tests the opa memory barriers
 *
 * Return:      Success:        exit(0)
 *
 *              Failure:        exit(1)
 *
 * Programmer:  Neil Fortner
 *              Wednesday, April 1, 2009
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

    /* Simple tests */
    nerrors += test_barriers_sanity();

    /* Loop over test configurations */
    for(curr_test=0; curr_test<num_thread_tests; curr_test++) {
        /* Don't test with only 1 thread */
        if(num_threads[curr_test] == 1)
            continue;

        /* Threaded tests */
        nerrors += test_barriers_linear_array();
        nerrors += test_barriers_variables();
        nerrors += test_barriers_scattered_array();
    } /* end for */

    if(nerrors)
        goto error;
    printf("All barriers tests passed.\n");

    return 0;

error:
    if(!nerrors)
        nerrors = 1;
    printf("***** %d BARRIERS TEST%s FAILED! *****\n",
            nerrors, 1 == nerrors ? "" : "S");
    return 1;
} /* end main() */

