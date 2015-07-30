/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Simple test of intercommunicator send and receive";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int leftGroup, buf, rank, remote_size, i;
    MPI_Comm comm;
    MPI_Status status;

    MTest_Init(&argc, &argv);

    while (MTestGetIntercomm(&comm, &leftGroup, 4)) {
        if (comm == MPI_COMM_NULL)
            continue;

        if (leftGroup) {
            MPI_Comm_rank(comm, &rank);
            buf = rank;
            MPI_Send(&buf, 1, MPI_INT, 0, 0, comm);
        }
        else {
            MPI_Comm_remote_size(comm, &remote_size);
            MPI_Comm_rank(comm, &rank);
            if (rank == 0) {
                for (i = 0; i < remote_size; i++) {
                    buf = -1;
                    MPI_Recv(&buf, 1, MPI_INT, i, 0, comm, &status);
                    if (buf != i) {
                        errs++;
                        fprintf(stderr, "buf = %d, should be %d\n", buf, i);
                    }
                }
            }
        }
        /* Now, reverse it and send back */
        if (!leftGroup) {
            MPI_Comm_rank(comm, &rank);
            buf = rank;
            MPI_Send(&buf, 1, MPI_INT, 0, 0, comm);
        }
        else {
            MPI_Comm_remote_size(comm, &remote_size);
            MPI_Comm_rank(comm, &rank);
            if (rank == 0) {
                for (i = 0; i < remote_size; i++) {
                    buf = -1;
                    MPI_Recv(&buf, 1, MPI_INT, i, 0, comm, &status);
                    if (buf != i) {
                        errs++;
                        fprintf(stderr, "buf = %d, should be %d\n", buf, i);
                    }
                }
            }
        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
