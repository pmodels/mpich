/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>
#include "mpitest.h"
#include "mcs-mutex.h"

#define NUM_ITER    1000
#define NUM_MUTEXES 1

const int verbose = 0;
double delay_ctr = 0.0;

int main(int argc, char **argv)
{
    int rank, nproc, i;
    double t_mcs_mtx;
    MPI_Comm mtx_comm;
    MCS_Mutex mcs_mtx;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

#ifdef USE_WIN_SHARED
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &mtx_comm);
#else
    mtx_comm = MPI_COMM_WORLD;
#endif

    MCS_Mutex_create(0, mtx_comm, &mcs_mtx);

    MPI_Barrier(MPI_COMM_WORLD);
    t_mcs_mtx = MPI_Wtime();

    for (i = 0; i < NUM_ITER; i++) {
        /* Combining trylock and lock here is helpful for testing because it makes
         * CAS and Fetch-and-op contend for the tail pointer. */
#ifdef USE_CONTIGUOUS_RANK
        if (rank < nproc / 2) {
#else
        if (rank % 2) {
#endif
            int success = 0;
            while (!success) {
                MCS_Mutex_trylock(mcs_mtx, &success);
            }
        }
        else {
            MCS_Mutex_lock(mcs_mtx);
        }
        MCS_Mutex_unlock(mcs_mtx);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    t_mcs_mtx = MPI_Wtime() - t_mcs_mtx;

    MCS_Mutex_free(&mcs_mtx);

    if (rank == 0) {
        if (verbose) {
            printf("Nproc %d, MCS Mtx = %f us\n", nproc, t_mcs_mtx / NUM_ITER * 1.0e6);
        }
    }

    if (mtx_comm != MPI_COMM_WORLD)
        MPI_Comm_free(&mtx_comm);

    MTest_Finalize(0);
    MPI_Finalize();

    return 0;
}
