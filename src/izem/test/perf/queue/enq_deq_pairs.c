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
    int max_threads = omp_get_max_threads();
    zm_absqueue_t* queues = malloc (max_threads/2 * sizeof(zm_absqueue_t));
    double t1, t2;

    printf("#threads \t throughput ops/s\n");

    for ( int nthreads = 2; nthreads <= max_threads; nthreads += 2) {
        for(int i=0; i< nthreads/2; i++)
            zm_absqueue_init(&queues[i]);
        int nelem_enq, nelem_deq;

        nelem_enq = TEST_NELEMTS/(nthreads/2);
        nelem_deq = nelem_enq;

        t1 = omp_get_wtime();

        #pragma omp parallel num_threads(nthreads)
        {

            int tid, producer_b, qidx;
        #if defined(ZMTEST_ALLOC_QELEM)
            int *input;
        #else
            int input = 1;
        #endif
            tid = omp_get_thread_num();
            producer_b = (tid % 2 == 0);
            qidx = tid/2;
    
            unsigned deq_count = 0;
            if(producer_b) { /* producer */
                for(int elem=0; elem < nelem_enq; elem++) {
        #if defined(ZMTEST_ALLOC_QELEM)
                    input = malloc(sizeof *input);
                    *input = 1;
                    zm_absqueue_enqueue(&queues[qidx], (void*) input);
        #else
                    zm_absqueue_enqueue(&queues[qidx], (void*) &input);
        #endif
                }
            } else {           /* consumer */
                while(deq_count < nelem_deq) {
                    int* elem = NULL;
                    zm_absqueue_dequeue(&queues[qidx], (void**)&elem);
                    if ((elem != NULL) && (*elem == 1)) {
                        deq_count++;
        #if defined(ZMTEST_ALLOC_QELEM)
                        free(elem);
        #endif
                    }
                }
            }
        }

        t2 = omp_get_wtime();
        printf("%d \t %lf\n", nthreads, (double)nelem_deq*nthreads/(t2-t1));
    }

} /* end run() */

int main(int argc, char **argv) {
  run();
} /* end main() */

