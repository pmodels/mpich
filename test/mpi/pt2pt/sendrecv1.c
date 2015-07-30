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
static char MTEST_Descrip[] = "Send-Recv";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, count;
    MPI_Comm comm;
    MTestDatatype sendtype, recvtype;

    MTest_Init(&argc, &argv);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        source = 0;
        dest = size - 1;

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        MTEST_DATATYPE_FOR_EACH_COUNT(count) {
            while (MTestGetDatatypes(&sendtype, &recvtype, count)) {
                /* Make sure that everyone has a recv buffer */
                recvtype.InitBuf(&recvtype);

                if (rank == source) {
                    sendtype.InitBuf(&sendtype);

                    err = MPI_Send(sendtype.buf, sendtype.count, sendtype.datatype, dest, 0, comm);
                    if (err) {
                        errs++;
                        if (errs < 10) {
                            MTestPrintError(err);
                        }
                    }
                }
                else if (rank == dest) {
                    err = MPI_Recv(recvtype.buf, recvtype.count,
                                   recvtype.datatype, source, 0, comm, MPI_STATUS_IGNORE);
                    if (err) {
                        errs++;
                        if (errs < 10) {
                            MTestPrintError(err);
                        }
                    }

                    err = MTestCheckRecv(0, &recvtype);
                    if (err) {
                        if (errs < 10) {
                            printf
                                ("Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d\n",
                                 MTestGetDatatypeName(&recvtype), MTestGetDatatypeName(&sendtype),
                                 count);
                            recvtype.printErrors = 1;
                            (void) MTestCheckRecv(0, &recvtype);
                        }
                        errs += err;
                    }
                }
                MTestFreeDatatype(&sendtype);
                MTestFreeDatatype(&recvtype);
            }
        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
