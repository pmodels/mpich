/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "zmtest_absqueue.h"
#define TEST_NELEMTS  1000

/*-------------------------------------------------------------------------
 * Function: run
 *
 * Purpose: Test the correctness of queue operations by counting the number
 *  of dequeued elements to the expected number
 *
 * Return: Success: 0
 *         Failure: 1
 *-------------------------------------------------------------------------
 */
static inline void run() {
    unsigned test_counter = 0;
    zm_absqueue_t queue;
    double t1, t2;

    printf("#threads \t throughput ops/s\n");

    for ( int nthreads = 2; nthreads <= omp_get_max_threads(); nthreads ++) {
        zm_absqueue_init(&queue);
        int nelem_enq, nelem_deq;

        #if   defined(ZMTEST_MPMC)
            nelem_enq = TEST_NELEMTS/(nthreads/2);
            nelem_deq = (nthreads/2)*nelem_enq;
        #elif defined(ZMTEST_MPSC)
            nelem_enq = TEST_NELEMTS/(nthreads-1);
            nelem_deq = (nthreads-1)*nelem_enq;
        #elif defined(ZMTEST_SPMC)
            nelem_enq = TEST_NELEMTS;
            nelem_deq = nelem_enq;
        #endif

        t1 = omp_get_wtime();

        #pragma omp parallel num_threads(nthreads)
        {

            int tid, producer_b;
        #if defined(ZMTEST_ALLOC_QELEM)
            int *input;
        #else
            int input = 1;
        #endif
        tid = omp_get_thread_num();
        #if   defined(ZMTEST_MPMC)
            producer_b = (tid % 2 == 0);
        #elif defined(ZMTEST_MPSC)
            producer_b = (tid != 0);
        #elif defined(ZMTEST_SPMC)
            producer_b = (tid == 0);
        #endif

            if(producer_b) { /* producer */
                for(int elem=0; elem < nelem_enq; elem++) {
        #if defined(ZMTEST_ALLOC_QELEM)
                    input = malloc(sizeof *input);
                    *input = 1;
                    zm_absqueue_enqueue(&queue, (void*) input);
        #else
                    zm_absqueue_enqueue(&queue, (void*) &input);
        #endif
                }
            } else {           /* consumer */
                while(test_counter < nelem_deq) {
                    int* elem = NULL;
                    zm_absqueue_dequeue(&queue, (void**)&elem);
                    if ((elem != NULL) && (*elem == 1)) {
                        #pragma omp atomic
                            test_counter++;
        #if defined(ZMTEST_ALLOC_QELEM)
                        free(elem);
        #endif
                    }
                }
            }
        }

        t2 = omp_get_wtime();
        printf("%d \t %lf\n", nthreads, (double)nelem_deq/(t2-t1));
    }

} /* end run() */

int main(int argc, char **argv) {
  run();
} /* end main() */

