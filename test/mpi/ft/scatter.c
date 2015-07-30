/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * This test attempts collective communication after a process in
 * the communicator has failed.
 */
int main(int argc, char **argv)
{
    int rank, size, i, rc, errclass, toterrs, errs = 0;
    char rbuf[100000];
    char *sendbuf;
    int deadprocs[1] = { 1 };
    MPI_Group world, newgroup;
    MPI_Comm newcomm;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (size < 3) {
        fprintf(stderr, "Must run with at least 3 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_group(MPI_COMM_WORLD, &world);
    MPI_Group_excl(world, 1, deadprocs, &newgroup);
    MPI_Comm_create_group(MPI_COMM_WORLD, newgroup, 0, &newcomm);

    if (rank == 1) {
        exit(EXIT_FAILURE);
    }

    /* try a small send first */
    sendbuf = (char *) malloc(10 * size * sizeof(char));

    if (rank == 0) {
        for (i = 0; i < size; i++) {
            strcpy(sendbuf + i * 10, "No Errors");
        }
    }

    rc = MPI_Scatter(sendbuf, 10, MPI_CHAR, rbuf, 10, MPI_CHAR, 0, MPI_COMM_WORLD);

#if defined (MPICH) && (MPICH_NUMVERSION >= 30100102)
    MPI_Error_class(rc, &errclass);
    if ((rc) && (errclass != MPIX_ERR_PROC_FAILED)) {
        fprintf(stderr, "Wrong error code (%d) returned. Expected MPIX_ERR_PROC_FAILED\n",
                errclass);
        errs++;
    }
#endif

    /* reset the buffers and try a larger scatter */
    free(sendbuf);
    memset(rbuf, 0, sizeof(rbuf));
    sendbuf = (char *) malloc(100000 * size * sizeof(char));

    if (rank == 0) {
        for (i = 0; i < size; i++) {
            strcpy(sendbuf + i * 100000, "No Errors");
        }
    }

    rc = MPI_Scatter(sendbuf, 100000, MPI_CHAR, rbuf, 100000, MPI_CHAR, 0, MPI_COMM_WORLD);

#if defined (MPICH) && (MPICH_NUMVERSION >= 30100102)
    MPI_Error_class(rc, &errclass);
    if ((rc) && (errclass != MPIX_ERR_PROC_FAILED)) {
        fprintf(stderr, "Wrong error code (%d) returned. Expected MPIX_ERR_PROC_FAILED\n",
                errclass);
        errs++;
    }
#endif

    rc = MPI_Reduce(&errs, &toterrs, 1, MPI_INT, MPI_SUM, 0, newcomm);
    if (rc)
        fprintf(stderr, "Failed to get errors from other processes\n");

    if (rank == 0) {
        if (toterrs) {
            printf(" Found %d errors\n", toterrs);
        }
        else {
            printf(" No Errors\n");
        }
        fflush(stdout);
    }

    free(sendbuf);

    MPI_Comm_free(&newcomm);
    MPI_Group_free(&newgroup);
    MPI_Group_free(&world);

    MPI_Finalize();

    return 0;
}
