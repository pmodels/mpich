/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of send cancel (failure) calls";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, source, dest;
    MPI_Comm comm;
    MPI_Status status;
    MPI_Request req;
    static int bufsizes[4] = { 1, 100, 10000, 1000000 };
    char *buf;
    int cs, flag, n;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    source = 0;
    dest = size - 1;

    MTestPrintfMsg(1, "Starting scancel test\n");

    for (cs = 0; cs < 4; cs++) {
        n = bufsizes[cs];
        buf = (char *) malloc(n);
        if (!buf) {
            fprintf(stderr, "Unable to allocate %d bytes\n", n);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (rank == source) {
            MTestPrintfMsg(1, "(%d) About to create isend and cancel\n", cs);
            MPI_Isend(buf, n, MPI_CHAR, dest, cs + n + 1, comm, &req);
            MPI_Barrier(comm);
            MPI_Cancel(&req);
            MPI_Wait(&req, &status);
            MTestPrintfMsg(1, "Completed wait on isend\n");
            MPI_Test_cancelled(&status, &flag);
            if (flag) {
                errs++;
                printf("Cancelled a matched Isend request (msg size = %d)!\n", n);
                fflush(stdout);
            }
            else {
                n = 0;
            }
            /* Send the size, zero for not cancelled (success) */
            MPI_Send(&n, 1, MPI_INT, dest, 123, comm);
        }
        else if (rank == dest) {
            MPI_Recv(buf, n, MPI_CHAR, source, cs + n + 1, comm, &status);
            MPI_Barrier(comm);
            MPI_Recv(&n, 1, MPI_INT, source, 123, comm, &status);
        }
        else {
            MPI_Barrier(comm);
        }

        MPI_Barrier(comm);
        free(buf);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
