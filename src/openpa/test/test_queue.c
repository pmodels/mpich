/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Uncomment this definition to disable the OPA library and instead use naive
 * (non-atomic) operations.  This should cause failures.
 */
/*
 * Note: The NAIVE implementation is likely to cause an infinite loop because of
 * the busy wait in OPA_Queue_dequeue that might never exit if an element was
 * enqueued improperly.
 */
/*
#define OPA_TEST_NAIVE
*/

#include "opa_test.h"
#include "opa_queue.h"

/* Definitions for test_queue_sanity */
typedef struct {
    OPA_Queue_element_hdr_t hdr;        /* Queue element header */
    int         val;                    /* Value of element */
} sanity_t;

/* Definitions for test_queue_threaded */
#define THREADED_NITER 500000
typedef struct {
    OPA_Queue_element_hdr_t hdr;        /* Queue element header */
    int         threadno;               /* Unique thread number */
    int         sequenceno;             /* Unique sequence number within thread */
} threaded_element_t;
typedef enum {
    THREADED_MODE_DEFAULT,              /* Test does not try to influence scheduling */
    THREADED_MODE_EMPTY,                /* Test tries to keep queue empty */
    THREADED_MODE_FULL,                 /* Test tries to keep queue full */
    THREADED_MODE_NMODES                /* Number of modes (must be last) */
} threaded_mode_t;
typedef struct {
    OPA_Queue_info_t *qhead;            /* Head for shared queue */
    int         threadno;               /* Unique thread number */
    threaded_mode_t mode;               /* Test mode (empty or full) */
} threaded_t;


/*-------------------------------------------------------------------------
 * Function: test_queue_sanity
 *
 * Purpose: Tests basic functionality of the OPA Queue in a single
 *          threaded environment.
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
static int test_queue_sanity(void)
{
    OPA_Queue_info_t    qhead;
    sanity_t            elements[4];
    sanity_t            *element;
    int                 i;

    TESTING("queue sanity", 0);

    /* Initialize queue */
    OPA_Queue_init(&qhead);

    /* Make sure the queue is empty */
    if(!OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    if(NULL != OPA_Queue_peek_head(&qhead)) TEST_ERROR;

    /* Initialize elements */
    for(i=0; i<4; i++) {
        OPA_Queue_header_init(&elements[i].hdr);
        elements[i].val = i;
    } /* end for */

    /* Enqueue element 0 */
    OPA_Queue_enqueue(&qhead, &elements[0], sanity_t, hdr);

    /* Peek and make sure we get element 0 */
    if(OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    if(NULL == (element = OPA_Queue_peek_head(&qhead))) TEST_ERROR;
    if(element->val != 0) TEST_ERROR;

    /* Dequeue and make sure we get element 0 */
    if(OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    OPA_Queue_dequeue(&qhead, element, sanity_t, hdr);
    if(element->val != 0) TEST_ERROR;

    /* Make sure the queue is now empty */
    if(!OPA_Queue_is_empty(&qhead)) TEST_ERROR;

    /* Enqueue elements 1 and 2 */
    OPA_Queue_enqueue(&qhead, &elements[1], sanity_t, hdr);
    OPA_Queue_enqueue(&qhead, &elements[2], sanity_t, hdr);

    /* Peek and make sure we get element 1 */
    if(OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    if(NULL == (element = OPA_Queue_peek_head(&qhead))) TEST_ERROR;
    if(element->val != 1) TEST_ERROR;

    /* Dequeue and make sure we get element 1 */
    if(OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    OPA_Queue_dequeue(&qhead, element, sanity_t, hdr);
    if(element->val != 1) TEST_ERROR;

    /* Enqueue element 3 */
    OPA_Queue_enqueue(&qhead, &elements[3], sanity_t, hdr);

    /* Dequeue and make sure we get element 2 */
    if(OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    OPA_Queue_dequeue(&qhead, element, sanity_t, hdr);
    if(element->val != 2) TEST_ERROR;

    /* Peek and make sure we get element 3 */
    if(OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    if(NULL == (element = OPA_Queue_peek_head(&qhead))) TEST_ERROR;
    if(element->val != 3) TEST_ERROR;

    /* Dequeue and make sure we get element 3 */
    if(OPA_Queue_is_empty(&qhead)) TEST_ERROR;
    OPA_Queue_dequeue(&qhead, element, sanity_t, hdr);
    if(element->val != 3) TEST_ERROR;

    /* Make sure the queue is now empty */
    if(!OPA_Queue_is_empty(&qhead)) TEST_ERROR;

    PASSED();
    return 0;

error:
    return 1;
} /* end test_queue_sanity() */


#if defined(OPA_HAVE_PTHREAD_H)
/*-------------------------------------------------------------------------
 * Function: test_queue_threaded_enqueue
 *
 * Purpose: Helper (enqueue) routine for test_queue_threaded.  Enqueues
 *          elements to the shared queue with increasing sequence number
 *          and constant thread number.
 *
 * Return: Success: NULL
 *         Failure: non-NULL
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 8, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *test_queue_threaded_enqueue(void *_udata)
{
    threaded_t  *udata = (threaded_t *)_udata;
    threaded_element_t *element = NULL;         /* Element to enqueue */
    int         niter = THREADED_NITER / iter_reduction[curr_test];
    int         i;

    /* Main loop */
    for(i=0; i<niter; i++) {
        /* If we're trying to keep the queue empty, yield here so the dequeuer
         * has a chance to empty the queue */
        if(udata->mode == THREADED_MODE_EMPTY)
            OPA_TEST_YIELD();

        /* Create an element to enqueue */
        if(NULL == (element = (threaded_element_t *) malloc(sizeof(threaded_element_t))))
            pthread_exit((void *) 1);
        OPA_Queue_header_init(&element->hdr);
        element->threadno = udata->threadno;
        element->sequenceno = i;

        /* Enqueue the element */
        OPA_Queue_enqueue(udata->qhead, element, threaded_element_t, hdr);
    } /* end for */

    /* Exit */
    pthread_exit(NULL);
} /* end test_queue_threaded_enqueue() */


/*-------------------------------------------------------------------------
 * Function: test_queue_threaded_dequeue
 *
 * Purpose: Helper (dequeue) routine for test_queue_threaded.  Dequeues
 *          elements from the shared queue and verifies that each thread
 *          added each sequence number exactly once and in the right
 *          order, and that order was preserved by the queue.
 *
 * Return: Success: 0
 *         Failure: 1
 *
 * Programmer: Neil Fortner
 *             Tuesday, June 8, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int test_queue_threaded_dequeue(void *_udata)
{
    threaded_t  *udata = (threaded_t *)_udata;
    threaded_element_t *element = NULL;         /* Dequeued element */
    threaded_element_t *peek_element = NULL;    /* Peeked element */
    int         *expect_sn = NULL;              /* Expected sequence numbers */
    const int   nenqueuers = num_threads[curr_test] - 1;
    int         ndequeues = 0;  /* Number of times we have dequeued */
    int         peek_counter = 0; /* Iterations since last peek */
    const int   niter = (THREADED_NITER / iter_reduction[curr_test])
                        * nenqueuers;
    int         nerrors = 0;    /* Number of errors */
    int         i;

    /* Allocate expect_sn array */
    if(NULL == (expect_sn = (int *) calloc(nenqueuers, sizeof(int))))
        return(1);

    /* Main loop.  Terminates when either the expected number of elements have
     * been dequeued or the loop has continued too many times. */
    for(i=0; ndequeues < niter && i < (1000 * niter); i++) {
        /* If we're trying to keep the queue full, yield here so the enqueuers
         * have a chance to fill the queue */
        if(udata->mode == THREADED_MODE_FULL)
            OPA_TEST_YIELD();

        /* Check if the queue is nonempty */
        if(!OPA_Queue_is_empty(udata->qhead)) {
            /* Increment peek counter, and peek if the counter is 100 */
            peek_counter++;
            if(peek_counter == 100) {
                if(NULL == (peek_element = OPA_Queue_peek_head(udata->qhead)))
                    nerrors++;
                peek_counter = 0;
                if(OPA_Queue_is_empty(udata->qhead))
                    return(1);
            } /* end if */

            /* Dequeue the head element */
            OPA_Queue_dequeue(udata->qhead, element, threaded_element_t, hdr);

            /* Increment the counter */
            ndequeues++;

            /* Make sure the dequeued element is the same as the peeked element,
             * if applicable */
            if(peek_element) {
                if(peek_element->threadno != element->threadno
                        || peek_element->sequenceno != element->sequenceno) {
                    printf("    Dequeued element does not match peeked element\n");
                    nerrors++;
                } /* end if */
                peek_element = NULL;
            } /* end if */

            /* Check if the thread number is valid */
            if(element->threadno < 0 || element->threadno >= nenqueuers) {
                printf("    Unexpected thread number: %d\n", element->threadno);
                nerrors++;
            } else {
                /* Check if the sequence number is equal to that expected, and
                 * update the expected sequence number */
                if(element->sequenceno != expect_sn[element->threadno]) {
                    printf("    Unexpected sequence number: %d Expected: %d\n",
                            element->sequenceno, expect_sn[element->threadno]);
                    nerrors++;
                } /* end if */
                expect_sn[element->threadno] = element->sequenceno + 1;
            } /* end else */

            /* Free the element */
            free(element);
        } else
            OPA_TEST_YIELD();
    } /* end for */

    /* Check if the expected number of elements were dequeued */
    if(ndequeues != niter) {
        printf("    Incorrect number of elements dequeued: %d Expected: %d\n",
                ndequeues, niter);
        nerrors++;
    } /* end if */

    /* Verify that the queue is now empty */
    if(!OPA_Queue_is_empty(udata->qhead)) {
        printf("    Queue is not empty at end of test!\n");
        nerrors++;
    } /* end if */

    /* Sanity checking that expect_sn is consistent if no errors */
    if(!nerrors)
        for(i=0; i<nenqueuers; i++)
            assert(expect_sn[i] == THREADED_NITER / iter_reduction[curr_test]);

    /* Free expect_sn */
    free(expect_sn);

    /* Any non-NULL exit value indicates an error, we use (void *) 1 here */
    return(nerrors ? 1 : 0);
} /* end test_queue_threaded_dequeue() */
#endif /* OPA_HAVE_PTHREAD_H */


/*-------------------------------------------------------------------------
 * Function: test_queue_threaded
 *
 * Purpose: Tests that the queue behaves correctly in a multithreaded
 *          environment.  Creates many enqueuers and one dequeuer, and
 *          makes sure that elements are dequeued in the expected order.
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
static int test_queue_threaded(threaded_mode_t mode)
{
#if defined(OPA_HAVE_PTHREAD_H)
    pthread_t           *threads = NULL; /* Threads */
    pthread_attr_t      ptattr;         /* Thread attributes */
    static threaded_t   *thread_data = NULL; /* User data structs for threads */
    OPA_Queue_info_t    qhead;          /* The queue */
    void                *ret;           /* Thread return value */
    unsigned            nthreads = num_threads[curr_test];
    int                 nerrors = 0;    /* number of errors */
    int                 i;

    switch(mode) {
        case THREADED_MODE_DEFAULT:
            TESTING("multithreaded queue", nthreads);
            break;
        case THREADED_MODE_EMPTY:
            TESTING("multithreaded queue (empty queue)", nthreads);
            break;
        case THREADED_MODE_FULL:
            TESTING("multithreaded queue (full queue)", nthreads);
            break;
        default:
            FAIL_OP_ERROR(printf("    Unknown mode\n"));
    } /* end switch */

    /* Allocate array of threads */
    if(NULL == (threads = (pthread_t *) malloc(nthreads * sizeof(pthread_t))))
        TEST_ERROR;

    /* Set threads to be joinable */
    pthread_attr_init(&ptattr);
    pthread_attr_setdetachstate(&ptattr, PTHREAD_CREATE_JOINABLE);

    /* Allocate array of thread data */
    if(NULL == (thread_data = (threaded_t *) calloc(nthreads, sizeof(threaded_t))))
        TEST_ERROR;

    /* Initialize queue */
    OPA_Queue_init(&qhead);

    /* Initialize thread data structs */
    for(i=0; i<nthreads; i++) {
        thread_data[i].qhead = &qhead;
        thread_data[i].threadno = i;
        thread_data[i].mode = mode;
    } /* end for */

    /* Create the threads. */
    for(i=0; i<(nthreads - 1); i++)
        if(pthread_create(&threads[i], &ptattr, test_queue_threaded_enqueue,
                &thread_data[i])) TEST_ERROR;
    nerrors = test_queue_threaded_dequeue(&thread_data[i]);

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
    free(thread_data);

    PASSED();

#else /* OPA_HAVE_PTHREAD_H */
    TESTING("multithreaded queue", 0);
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
} /* end test_barriers_linear_array() */


/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     Tests the opa queue
 *
 * Return:      Success:        exit(0)
 *
 *              Failure:        exit(1)
 *
 * Programmer:  Neil Fortner
 *              Tuesday, June 8, 2010
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int main(int argc, char **argv)
{
    threaded_mode_t mode;       /* Mode for threaded test */
    unsigned nerrors = 0;
#if defined(OPA_USE_LOCK_BASED_PRIMITIVES)
    OPA_emulation_ipl_t shm_lock;
    OPA_Interprocess_lock_init(&shm_lock, 1/*isLeader*/);
#endif

    /* Initialize shared memory.  Because we are just running threads in one
     * process, the memory will always be symmetric.  Just set the base address
     * to 0. */
    if(OPA_Shm_asymm_init((char *)0) != 0) {
        printf("Unable to initialize shared memory\n");
        goto error;
    } /* end if */

    /* Simple tests */
    nerrors += test_queue_sanity();

    /* Loop over test configurations */
    for(curr_test=0; curr_test<num_thread_tests; curr_test++) {
        /* Don't test with only 1 thread */
        if(num_threads[curr_test] == 1)
            continue;

        /* Threaded tests */
        for(mode = THREADED_MODE_DEFAULT; mode < THREADED_MODE_NMODES; mode++)
            nerrors += test_queue_threaded(mode);
    } /* end for */

    if(nerrors)
        goto error;
    printf("All queue tests passed.\n");

    return 0;

error:
    if(!nerrors)
        nerrors = 1;
    printf("***** %d QUEUE TEST%s FAILED! *****\n",
            nerrors, 1 == nerrors ? "" : "S");
    return 1;
} /* end main() */

