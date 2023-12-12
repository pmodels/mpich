/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpicolltest.h"

#ifdef MULTI_TESTS
#define run coll_icgather
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Simple intercomm gather test";
*/

int run(const char *arg)
{
    int errs = 0, err;
    int *buf = 0;
    int leftGroup, i, count, rank, rsize, size;
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
        MPI_Comm_size(comm, &size);
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);
        for (count = 1; count < 65000; count = 2 * count) {
            if (leftGroup) {
                buf = (int *) malloc(count * rsize * sizeof(int));
                for (i = 0; i < count * rsize; i++)
                    buf[i] = -1;

                err = MTest_Gather(is_blocking, NULL, 0, datatype,
                                   buf, count, datatype,
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
                } else {
                    /* Check for the correct data */
                    for (i = 0; i < count * rsize; i++) {
                        if (buf[i] != i) {
                            errs++;
                        }
                    }
                }
            } else {
                /* In the right group */
                buf = (int *) malloc(count * sizeof(int));
                for (i = 0; i < count; i++)
                    buf[i] = rank * count + i;
                err = MTest_Gather(is_blocking, buf, count, datatype, NULL, 0, datatype, 0, comm);
                if (err) {
                    errs++;
                    MTestPrintError(err);
                }
            }
            free(buf);
        }
        MTestFreeComm(&comm);
    }

    return errs;
}
