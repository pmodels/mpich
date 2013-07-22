/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

#define VERBOSE 0

int main(int argc, char **argv) {
    int      i, j, rank, nproc;
    MPI_Info info_in, info_out;
    int      errors = 0, all_errors = 0;
    MPI_Win  win;
    void    *base;
    char     invalid_key[] = "invalid_test_key";
    char     buf[MPI_MAX_INFO_VAL];
    int      flag;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, invalid_key, "true");

    MPI_Win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &base, &win);

    MPI_Win_set_info(win, info_in);
    MPI_Win_get_info(win, &info_out);

    MPI_Info_get(info_out, invalid_key, MPI_MAX_INFO_VAL, buf, &flag);
#ifndef USE_STRICT_MPI
    /* Check if our invalid key was ignored.  Note, this check's MPICH's
     * behavior, but this behavior may not be required for a standard
     * conforming MPI implementation. */
    if (flag) {
        printf("%d: %s was not ignored\n", rank, invalid_key);
        errors++;
    }
#endif

    MPI_Info_get(info_out, "no_locks", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE) printf("%d: no_locks = %s\n", rank, buf);

    MPI_Info_get(info_out, "accumulate_ordering", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE) printf("%d: accumulate_ordering = %s\n", rank, buf);

    MPI_Info_get(info_out, "accumulate_ops", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE) printf("%d: accumulate_ops = %s\n", rank, buf);

    MPI_Info_get(info_out, "same_size", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE) printf("%d: same_size = %s\n", rank, buf);

    MPI_Info_get(info_out, "alloc_shm", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE) printf("%d: alloc_shm = %s\n", rank, buf);

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);
    MPI_Win_free(&win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
