/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpicolltest.h"

/*
static char MTEST_Descrip[] = "Simple intercomm gatherv test";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int *buf = 0;
    int *recvcounts;
    int *recvdispls;
    int leftGroup, i, count, rank, rsize, size;
    MPI_Comm comm;
    MPI_Datatype datatype;

    MTest_Init(&argc, &argv);

    datatype = MPI_INT;
    while (MTestGetIntercomm(&comm, &leftGroup, 4)) {
        if (comm == MPI_COMM_NULL)
            continue;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_remote_size(comm, &rsize);
        MPI_Comm_size(comm, &size);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        for (count = 1; count < 65000; count = 2 * count) {
            /* Get an intercommunicator */
            recvcounts = (int *) malloc(rsize * sizeof(int));
            recvdispls = (int *) malloc(rsize * sizeof(int));
            /* This simple test duplicates the Gather test,
             * using the same lengths for all messages */
            for (i = 0; i < rsize; i++) {
                recvcounts[i] = count;
                recvdispls[i] = count * i;
            }
            if (leftGroup) {
                buf = (int *) malloc(count * rsize * sizeof(int));
                for (i = 0; i < count * rsize; i++)
                    buf[i] = -1;

                err = MTest_Gatherv(NULL, 0, datatype,
                                    buf, recvcounts, recvdispls, datatype,
                                    (rank == 0) ? MPI_ROOT : MPI_PROC_NULL, comm);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }
                /* Test that no other process in this group received the
                 * broadcast */
                if (rank != 0) {
                    for (i = 0; i < count; i++) {
                        if (buf[i] != -1) {
                            errs++;
                        }
                    }
                }
                else {
                    /* Check for the correct data */
                    for (i = 0; i < count * rsize; i++) {
                        if (buf[i] != i) {
                            errs++;
                        }
                    }
                }
            }
            else {
                /* In the right group */
                buf = (int *) malloc(count * sizeof(int));
                for (i = 0; i < count; i++)
                    buf[i] = rank * count + i;
                err = MTest_Gatherv(buf, count, datatype, NULL, 0, 0, datatype, 0, comm);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }
            }
            free(buf);
            free(recvcounts);
            free(recvdispls);
        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
