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
static char MTEST_Descrip[] = "Test err in status return, using truncated \
messages for MPI_Waitall";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Comm comm;
    MPI_Request r[2];
    MPI_Status s[2];
    int errval, errclass;
    int b1[20], b2[20], rank, size, src, dest, i;

    MTest_Init(&argc, &argv);

    /* Create some receive requests.  tags 0-9 will succeed, tags 10-19
     * will be used for ERR_TRUNCATE (fewer than 20 messages will be used) */
    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    src = 1;
    dest = 0;
    if (rank == dest) {
        MPI_Errhandler_set(comm, MPI_ERRORS_RETURN);
        errval = MPI_Irecv(b1, 10, MPI_INT, src, 0, comm, &r[0]);
        if (errval) {
            errs++;
            MTestPrintError(errval);
            printf("Error returned from Irecv\n");
        }
        errval = MPI_Irecv(b2, 10, MPI_INT, src, 10, comm, &r[1]);
        if (errval) {
            errs++;
            MTestPrintError(errval);
            printf("Error returned from Irecv\n");
        }

        /* Wait for Irecvs to be posted before the sender calls send.  This
         * prevents the operation from completing and returning an error in the
         * Irecv. */
        errval = MPI_Recv(NULL, 0, MPI_INT, src, 100, comm, MPI_STATUS_IGNORE);
        if (errval) {
            errs++;
            MTestPrintError(errval);
            printf("Error returned from Recv\n");
        }

        if (errval) {
            errs++;
            MTestPrintError(errval);
            printf("Error returned from Barrier\n");
        }
        for (i = 0; i < 2; i++) {
            s[i].MPI_ERROR = -1;
        }
        errval = MPI_Waitall(2, r, s);
        MPI_Error_class(errval, &errclass);
        if (errclass != MPI_ERR_IN_STATUS) {
            errs++;
            printf("Did not get ERR_IN_STATUS in Waitall\n");
        }
        else {
            /* Check for success */
            /* We allow ERR_PENDING (neither completed nor in error) in case
             * the MPI implementation exits the Waitall when an error
             * is detected. Thanks to Jim Hoekstra of Iowa State University
             * and Kim McMahon for finding this bug in the test. */
            for (i = 0; i < 2; i++) {
                if (s[i].MPI_TAG < 10 && (s[i].MPI_ERROR != MPI_SUCCESS &&
                                          s[i].MPI_ERROR != MPI_ERR_PENDING)) {
                    char msg[MPI_MAX_ERROR_STRING];
                    int msglen = MPI_MAX_ERROR_STRING;
                    errs++;
                    printf("correct msg had error code %d\n", s[i].MPI_ERROR);
                    MPI_Error_string(s[i].MPI_ERROR, msg, &msglen);
                    printf("Error message was %s\n", msg);
                }
                else if (s[i].MPI_TAG >= 10 && s[i].MPI_ERROR == MPI_SUCCESS) {
                    errs++;
                    printf("truncated msg had MPI_SUCCESS\n");
                }
            }
        }

    }
    else if (rank == src) {
        /* Wait for Irecvs to be posted before the sender calls send */
        MPI_Ssend(NULL, 0, MPI_INT, dest, 100, comm);

        MPI_Send(b1, 10, MPI_INT, dest, 0, comm);
        MPI_Send(b2, 11, MPI_INT, dest, 10, comm);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
