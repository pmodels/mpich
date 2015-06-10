/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test message reception ordering issues
after cancelling a send";
*/

int main(int argc, char *argv[])
{

    int a, b, flag = 0, errs = 0;
    MPI_Request requests[2];
    MPI_Status statuses[2];

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int source = 0;
    int dest = size - 1;

    if (rank == 0) {
        a = 10;
        b = 20;
        MPI_Isend(&a, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &requests[0]);
        MPI_Isend(&b, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &requests[1]);
        MPI_Cancel(&requests[1]);
        MPI_Wait(&requests[1], &statuses[1]);
        MPI_Test_cancelled(&statuses[1], &flag);

        if (!flag) {
            printf("Failed to cancel send");
            errs++;
        }
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Wait(&requests[0], MPI_STATUS_IGNORE);
    }
    else if (rank == 1) {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Recv(&a, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (a == 20) {
            printf("Failed! got the data from the wrong send!\n");
            errs++;
        }
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
