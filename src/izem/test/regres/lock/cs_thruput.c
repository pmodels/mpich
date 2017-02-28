/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <zmtest_abslock.h>

#define TEST_NTHREADS 4
#define TEST_NITER 1000

static void* run(void *arg) {
     zm_abslock_t *lock = (zm_abslock_t*) arg;
     zm_abslock_localctx_t my_ctx __attribute__ ((unused));
     for(int iter=0; iter<TEST_NITER; iter++) {
         int err =  zm_abslock_acquire(lock, &my_ctx);
         if(err==0)  /* Lock successfully acquired */
            zm_abslock_release(lock, &my_ctx);   /* Release the lock */
         else {
            fprintf(stderr, "Error: couldn't acquire the lock\n");
            exit(1);
         }
     }
     return 0;
}

/*-------------------------------------------------------------------------
 * Function: test_lock_throughput
 *
 * Purpose: Test the lock thruput for an empty critical section
 *
 * Return: Success: 0
 *         Failure: 1
 *-------------------------------------------------------------------------
 */
static void test_lock_thruput() {
    void *res;
    pthread_t threads[TEST_NTHREADS];

    zm_abslock_t lock;
    zm_abslock_init(&lock);

    for (int th=0; th<TEST_NTHREADS; th++) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(th, &cpuset);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
        pthread_create(&threads[th], &attr, run, (void*) &lock);
    }
    for (int th=0; th<TEST_NTHREADS; th++)
        pthread_join(threads[th], &res);

    printf("Pass\n");

} /* end test_lock_thruput() */

int main(int argc, char **argv)
{
  test_lock_thruput();
} /* end main() */

