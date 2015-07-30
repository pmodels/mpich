/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of sending to self (with a preposted receive)";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size;
    int count;
    MPI_Comm comm;
    MPI_Request req;
    MTestDatatype sendtype, recvtype;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

    MTEST_DATATYPE_FOR_EACH_COUNT(count) {
        while (MTestGetDatatypes(&sendtype, &recvtype, count)) {

            sendtype.InitBuf(&sendtype);
            recvtype.InitBuf(&recvtype);

            err = MPI_Irecv(recvtype.buf, recvtype.count, recvtype.datatype, rank, 0, comm, &req);
            if (err) {
                errs++;
                if (errs < 10) {
                    MTestPrintError(err);
                }
            }

            err = MPI_Send(sendtype.buf, sendtype.count, sendtype.datatype, rank, 0, comm);
            if (err) {
                errs++;
                if (errs < 10) {
                    MTestPrintError(err);
                }
            }
            err = MPI_Wait(&req, MPI_STATUS_IGNORE);
            err = MTestCheckRecv(0, &recvtype);
            if (err) {
                if (errs < 10) {
                    printf
                        ("Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d\n",
                         MTestGetDatatypeName(&recvtype), MTestGetDatatypeName(&sendtype), count);
                    recvtype.printErrors = 1;
                    (void) MTestCheckRecv(0, &recvtype);
                }
                errs += err;
            }

            err = MPI_Irecv(recvtype.buf, recvtype.count, recvtype.datatype, rank, 0, comm, &req);
            if (err) {
                errs++;
                if (errs < 10) {
                    MTestPrintError(err);
                }
            }

            err = MPI_Ssend(sendtype.buf, sendtype.count, sendtype.datatype, rank, 0, comm);
            if (err) {
                errs++;
                if (errs < 10) {
                    MTestPrintError(err);
                }
            }
            err = MPI_Wait(&req, MPI_STATUS_IGNORE);
            err = MTestCheckRecv(0, &recvtype);
            if (err) {
                if (errs < 10) {
                    printf
                        ("Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d\n",
                         MTestGetDatatypeName(&recvtype), MTestGetDatatypeName(&sendtype), count);
                    recvtype.printErrors = 1;
                    (void) MTestCheckRecv(0, &recvtype);
                }
                errs += err;
            }

            err = MPI_Irecv(recvtype.buf, recvtype.count, recvtype.datatype, rank, 0, comm, &req);
            if (err) {
                errs++;
                if (errs < 10) {
                    MTestPrintError(err);
                }
            }

            err = MPI_Rsend(sendtype.buf, sendtype.count, sendtype.datatype, rank, 0, comm);
            if (err) {
                errs++;
                if (errs < 10) {
                    MTestPrintError(err);
                }
            }
            err = MPI_Wait(&req, MPI_STATUS_IGNORE);
            err = MTestCheckRecv(0, &recvtype);
            if (err) {
                if (errs < 10) {
                    printf
                        ("Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d\n",
                         MTestGetDatatypeName(&recvtype), MTestGetDatatypeName(&sendtype), count);
                    recvtype.printErrors = 1;
                    (void) MTestCheckRecv(0, &recvtype);
                }
                errs += err;
            }

            MTestFreeDatatype(&sendtype);
            MTestFreeDatatype(&recvtype);
        }
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
