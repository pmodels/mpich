/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test err in status return, using truncated \
messages for MPI_Testsome";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Comm comm;
    MPI_Request r[2];
    MPI_Status s[2];
    int indices[2], outcount;
    int errval, errclass;
    int b1[20], b2[20], rank, size, src, dest, i, j;

    MTEST_VG_MEM_INIT(b1, 20 * sizeof(int));
    MTEST_VG_MEM_INIT(b2, 20 * sizeof(int));

    MTest_Init(&argc, &argv);

    /* Create some receive requests.  tags 0-9 will succeed, tags 10-19
     * will be used for ERR_TRUNCATE (fewer than 20 messages will be used) */
    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    src = 1;
    dest = 0;
    if (rank == dest) {
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);
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

        /* Wait for sends to complete at the sender before proceeding */
        /* WARNING: This does not guarantee that the sends are ready to
         * complete at the receiver. */
        errval = MPI_Recv(NULL, 0, MPI_INT, src, 10, comm, MPI_STATUS_IGNORE);
        if (errval) {
            errs++;
            MTestPrintError(errval);
            printf("Error returned from Recv\n");
        }
        for (i = 0; i < 2; i++) {
            s[i].MPI_ERROR = -1;
        }

        /* Spin MPI_Testsome until both Irecvs completes. */
        /* NOTE: MPI does not guarantee a sinngle MPI_Testsome complete both requests */
        int got_err_in_status = 0;
        int complete_count = 0;
        while (complete_count < 2) {
            errval = MPI_Testsome(2, r, &outcount, indices, s);
            complete_count += outcount;
            if (errval != MPI_SUCCESS) {
                MPI_Error_class(errval, &errclass);
                if (errclass == MPI_ERR_IN_STATUS) {
                    got_err_in_status++;
                }
            }
        }

        if (got_err_in_status != 1) {
            errs++;
            printf("Did not get ERR_IN_STATUS in Testsome; class returned was %d\n", errclass);
        }
        if (complete_count != 2) {
            printf("complete_count = %d\n", complete_count);
        } else {
            /* Check for success */
            for (i = 0; i < outcount; i++) {
                j = i;
                /* Indices is the request index */
                if (s[j].MPI_TAG < 10 && s[j].MPI_ERROR != MPI_SUCCESS) {
                    errs++;
                    printf("correct msg had error class %d\n", s[j].MPI_ERROR);
                } else if (s[j].MPI_TAG >= 10 && s[j].MPI_ERROR == MPI_SUCCESS) {
                    errs++;
                    printf("truncated msg had MPI_SUCCESS\n");
                }
            }
        }

    } else if (rank == src) {
        /* Wait for Irecvs to be posted before the sender calls send */
        MPI_Ssend(NULL, 0, MPI_INT, dest, 100, comm);

        /* Send test messages, then send another message so that the test does
         * not start until we are sure that the sends have begun */
        MPI_Send(b1, 10, MPI_INT, dest, 0, comm);
        MPI_Send(b2, 11, MPI_INT, dest, 10, comm);

        /* Wait for sends to complete before proceeding to the testsome. */
        MPI_Ssend(NULL, 0, MPI_INT, dest, 10, comm);
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
