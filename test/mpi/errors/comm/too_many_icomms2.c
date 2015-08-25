/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test attempts to create a large number of communicators, in an effort
 * to exceed the number of communicators that the MPI implementation can
 * provide.  It checks that the implementation detects this error correctly
 * handles it.
 */

/* In this version, we duplicate MPI_COMM_WORLD in a non-blocking
 * fashion until we run out of context IDs. */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

#define MAX_NCOMM 100000
#define WAIT_COMM 7
static const int verbose = 0;

int main(int argc, char **argv)
{
    int rank, nproc, mpi_errno;
    int i, j, ncomm, block;
    int errors = 1;
    MPI_Comm *comm_hdls;
    MPI_Request req[WAIT_COMM];

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Status error_status[WAIT_COMM];
    comm_hdls = malloc(sizeof(MPI_Comm) * MAX_NCOMM);


    ncomm = 0;
    block = 0;
    for (i = 0; i < MAX_NCOMM; i++) {
        /* Note: the comms we create are all dups of MPI_COMM_WORLD */
        MPI_Comm_idup(MPI_COMM_WORLD, &comm_hdls[i], &req[block++]);
        if(block == WAIT_COMM ){
            mpi_errno = MPI_Waitall(block, req, error_status);
            if (mpi_errno == MPI_SUCCESS) {
                ncomm+=block;
            }
            else {
                if (verbose)
                    printf("%d: Error creating comm %d\n", rank, i);
                for(j = 0; j <  block; j++) {
                    if(error_status[j].MPI_ERROR == MPI_SUCCESS){
                        ncomm+=1;
                    }
                    else if(error_status[j].MPI_ERROR == MPI_ERR_PENDING) {
                        mpi_errno = MPI_Wait(&req[j], MPI_STATUSES_IGNORE);
                        if(mpi_errno == MPI_SUCCESS) {
                            ncomm+=1;
                        }
                        else {
                            if (verbose)
                                printf("%d: Error creating comm %d\n", rank, i);
                        }
                    }
                }
                errors = 0;
                block = 0;
                break;
            }
            block = 0;
        }
    }
    if(block != 0) {
        mpi_errno = MPI_Waitall(block, req, MPI_STATUSES_IGNORE);
        if (mpi_errno == MPI_SUCCESS) {
            ncomm+=block;
        }
        else {
           if (verbose)
                printf("%d: Error creating comm %d\n", rank, i);
            errors = 0;
        }
    }
    for (i = 0; i < ncomm; i++)
        MPI_Comm_free(&comm_hdls[i]);

    free(comm_hdls);
    MTest_Finalize(errors);
    MPI_Finalize();

    return 0;
}
