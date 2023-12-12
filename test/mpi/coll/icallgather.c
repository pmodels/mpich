/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpicolltest.h"

#ifdef MULTI_TESTS
#define run coll_icallgather
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Simple intercomm allgather test";
*/

int run(const char *arg)
{
    int errs = 0, err;
    int *rbuf = 0, *sbuf = 0;
    int leftGroup, i, count, rank, rsize;
    MPI_Comm comm;
    MPI_Datatype datatype;

    int is_blocking = 1;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    if (MTestArgListGetInt_with_default(head, "nonblocking", 0)) {
        is_blocking = 0;
    }
    MTestArgListDestroy(head);

    datatype = MPI_INT;
    /* Get an intercommunicator */
    while (MTestGetIntercomm(&comm, &leftGroup, 4)) {
        if (comm == MPI_COMM_NULL)
            continue;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_remote_size(comm, &rsize);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

        for (count = 1; count < 65000; count = 2 * count) {
            /* The left group will send rank to the right group;
             * The right group will send -rank to the left group */
            rbuf = (int *) malloc(count * rsize * sizeof(int));
            sbuf = (int *) malloc(count * sizeof(int));
            for (i = 0; i < count * rsize; i++)
                rbuf[i] = -1;
            if (leftGroup) {
                for (i = 0; i < count; i++)
                    sbuf[i] = i + rank * count;
            } else {
                for (i = 0; i < count; i++)
                    sbuf[i] = -(i + rank * count);
            }
            err = MTest_Allgather(is_blocking, sbuf, count, datatype, rbuf, count, datatype, comm);
            if (err) {
                errs++;
                MTestPrintError(err);
            }
            if (leftGroup) {
                for (i = 0; i < count * rsize; i++) {
                    if (rbuf[i] != -i) {
                        errs++;
                    }
                }
            } else {
                for (i = 0; i < count * rsize; i++) {
                    if (rbuf[i] != i) {
                        errs++;
                    }
                }
            }

            /* Use Allgather in a unidirectional way */
            for (i = 0; i < count * rsize; i++)
                rbuf[i] = -1;
            if (leftGroup) {
                err = MTest_Allgather(is_blocking, sbuf, 0, datatype, rbuf, count, datatype, comm);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }
                for (i = 0; i < count * rsize; i++) {
                    if (rbuf[i] != -i) {
                        errs++;
                    }
                }
            } else {
                err = MTest_Allgather(is_blocking, sbuf, count, datatype, rbuf, 0, datatype, comm);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }
                for (i = 0; i < count * rsize; i++) {
                    if (rbuf[i] != -1) {
                        errs++;
                    }
                }
            }
            free(rbuf);
            free(sbuf);
        }
        MTestFreeComm(&comm);
    }

    return errs;
}
