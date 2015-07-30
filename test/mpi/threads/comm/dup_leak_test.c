/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* This test repeatedly dups and frees communicators with multiple threads
 * concurrently to stress the multithreaded aspects of the context ID allocation
 * code.
 *
 * Thanks to IBM for providing the original version of this test */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#ifndef NITER
#define NITER 12345
#endif /* ! NITER */

#ifndef NTHREADS
#define NTHREADS 4
#endif /* ! NTHREADS */

MTEST_THREAD_RETURN_TYPE do_thread(void *v);
MTEST_THREAD_RETURN_TYPE do_thread(void *v)
{
    int x;
    MPI_Comm comm = *(MPI_Comm *) v;
    MPI_Comm newcomm;
    for (x = 0; x < NITER; ++x) {
        MPI_Comm_dup(comm, &newcomm);
        MPI_Comm_free(&newcomm);
    }
    return (MTEST_THREAD_RETURN_TYPE) 0;
}

int main(int argc, char **argv)
{
    int x;
    int threaded;
    int err;
    MPI_Comm comms[NTHREADS];
    int num_threads_obtained = 1;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &threaded);
    if (threaded != MPI_THREAD_MULTIPLE) {
        printf("unable to initialize with MPI_THREAD_MULTIPLE\n");
        goto fn_fail;
    }

    for (x = 0; x < NTHREADS; ++x) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comms[x]);
        if (x != 0) {
            err = MTest_Start_thread(do_thread, (void *) &comms[x]);
            if (err) {
                /* attempt to continue with fewer threads, we may be on a
                 * thread-constrained platform like BG/P in DUAL mode */
                MPI_Comm_free(&comms[x]);
                break;
            }
            ++num_threads_obtained;
        }
    }

    if (num_threads_obtained <= 1) {
        printf("unable to create any additional threads, exiting\n");
        goto fn_fail;
    }

    do_thread((void *) &comms[0]);      /* we are thread 0 */

    err = MTest_Join_threads();
    if (err) {
        printf("error joining threads, err=%d", err);
        goto fn_fail;
    }

    for (x = 0; x < num_threads_obtained; ++x) {
        MPI_Comm_free(&comms[x]);
    }

  fn_exit:
    MTest_Finalize(err);
    MPI_Finalize();
    return 0;
  fn_fail:
    err = 1;
    goto fn_exit;
}
