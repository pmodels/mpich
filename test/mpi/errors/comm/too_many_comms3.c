/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test attempts to create a large number of communicators, in an effort
 * to exceed the number of communicators that the MPI implementation can
 * provide.  It checks that the implementation detects this error correctly
 * handles it.
 */

/* In this version, we create communicators that contain ranks {0}, {0}, ...,
 * {P} from MPI_COMM_WORLD until we run out of context IDs.  This test
 * fragments the context ID space, resulting in a context ID allocation that
 * fails because there is no common free ID, even though all processes have
 * unused context IDs. */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

#define MAX_NCOMM 100000

static const int verbose = 0;

int main(int argc, char **argv)
{
    int rank, nproc, mpi_errno;
    int i, ncomm, *ranks;
    int errors = 1;
    MPI_Comm *comm_hdls;
    MPI_Group world_group;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    comm_hdls = malloc(sizeof(MPI_Comm) * MAX_NCOMM);
    ranks = malloc(sizeof(int) * nproc);

    ncomm = 0;
    for (i = 0; i < MAX_NCOMM; i++) {
        int incl = i % nproc;
        MPI_Group comm_group;

        /* Comms include ranks: 0; 1; 2; ...; 0; 1; ... */
        MPI_Group_incl(world_group, 1, &incl, &comm_group);

        /* Note: the comms we create all contain one rank from MPI_COMM_WORLD */
        mpi_errno = MPI_Comm_create(MPI_COMM_WORLD, comm_group, &comm_hdls[i]);

        if (mpi_errno == MPI_SUCCESS) {
            if (verbose)
                printf("%d: Created comm %d\n", rank, i);
            ncomm++;
        }
        else {
            if (verbose)
                printf("%d: Error creating comm %d\n", rank, i);
            MPI_Group_free(&comm_group);
            errors = 0;
            break;
        }

        MPI_Group_free(&comm_group);
    }

    for (i = 0; i < ncomm; i++)
        MPI_Comm_free(&comm_hdls[i]);

    free(comm_hdls);
    MPI_Group_free(&world_group);

    MTest_Finalize(errors);
    MPI_Finalize();

    return 0;
}
