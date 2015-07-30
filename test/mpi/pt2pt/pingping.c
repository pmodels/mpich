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
static char MTEST_Descrip[] = "Send flood test";
*/

#define MAX_MSG_SIZE 40000000
#define MAX_COUNT    4000
int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, source, dest;
    int minsize = 2, count, nmsg, maxmsg;
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

        for (count = 1; count < MAX_COUNT; count = count * 2) {

            /* To shorten test time, only run the default version of datatype tests
             * for comm world and run the minimum version for other communicators. */
            if (comm != MPI_COMM_WORLD) {
                MTestInitMinDatatypes();
            }

            while (MTestGetDatatypes(&sendtype, &recvtype, count)) {
                int nbytes;
                MPI_Type_size(sendtype.datatype, &nbytes);

                /* We may want to limit the total message size sent */
                if (nbytes > MAX_MSG_SIZE) {
                    /* We do not need to free, as we haven't
                     * initialized any of the buffers (?) */
                    continue;
                }
                maxmsg = MAX_COUNT - count;
                MTestPrintfMsg(1, "Sending count = %d of sendtype %s of total size %d bytes\n",
                               count, MTestGetDatatypeName(&sendtype), nbytes * count);
                /* Make sure that everyone has a recv buffer */
                recvtype.InitBuf(&recvtype);

                if (rank == source) {
                    sendtype.InitBuf(&sendtype);

                    for (nmsg = 1; nmsg < maxmsg; nmsg++) {
                        err = MPI_Send(sendtype.buf, sendtype.count,
                                       sendtype.datatype, dest, 0, comm);
                        if (err) {
                            errs++;
                            if (errs < 10) {
                                MTestPrintError(err);
                            }
                        }
                    }
                }
                else if (rank == dest) {
                    for (nmsg = 1; nmsg < maxmsg; nmsg++) {
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
                                    ("Data in target buffer did not match for destination datatype %s and source datatype %s, count = %d, message iteration %d of %d\n",
                                     MTestGetDatatypeName(&recvtype),
                                     MTestGetDatatypeName(&sendtype), count, nmsg, maxmsg);
                                recvtype.printErrors = 1;
                                (void) MTestCheckRecv(0, &recvtype);
                            }
                            errs += err;
                        }
                    }
                }
                MTestFreeDatatype(&recvtype);
                MTestFreeDatatype(&sendtype);
            }
        }
        MTestFreeComm(&comm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
