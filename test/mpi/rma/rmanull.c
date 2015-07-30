/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* Test the given operation within a Fence epoch */
#define TEST_FENCE_OP(op_name_, fcn_call_)                              \
    do {                                                                \
        err = fcn_call_                                                 \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("PROC_NULL to " op_name_, err);    \
            }                                                           \
        }                                                               \
        err = MPI_Win_fence(0, win);                                  \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("Fence after " op_name_, err);     \
            }                                                           \
        }                                                               \
    } while (0)


/* Test the given operation within a passive target epoch */
#define TEST_PT_OP(op_name_, fcn_call_)                                 \
    do {                                                                \
        err = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, MPI_PROC_NULL, 0, win);  \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("Lock before" op_name_, err);      \
            }                                                           \
        }                                                               \
        err = fcn_call_                                                 \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("PROC_NULL to " op_name_, err);    \
            }                                                           \
        }                                                               \
        err = MPI_Win_unlock(MPI_PROC_NULL, win);                     \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("Unlock after " op_name_, err);    \
            }                                                           \
        }                                                               \
    } while (0)


/* Test the given request-based operation within a passive target epoch */
#define TEST_REQ_OP(op_name_, req_, fcn_call_)                          \
    do {                                                                \
        err = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, MPI_PROC_NULL, 0, win);  \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("Lock before" op_name_, err);      \
            }                                                           \
        }                                                               \
        err = fcn_call_                                                 \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("PROC_NULL to " op_name_, err);    \
            }                                                           \
        }                                                               \
        err = MPI_Win_unlock(MPI_PROC_NULL, win);                     \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("Unlock after " op_name_, err);    \
            }                                                           \
        }                                                               \
        err = MPI_Wait(&req_, MPI_STATUS_IGNORE);                     \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("Wait after " op_name_, err);      \
            }                                                           \
        }                                                               \
    } while (0)

/*
static char MTEST_Descrip[] = "Test the MPI_PROC_NULL is a valid target";
*/

int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size;
    int *buf, bufsize;
    int *result;
    int *rmabuf, rsize, rcount;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Request req;

    MTest_Init(&argc, &argv);

    bufsize = 256 * sizeof(int);
    buf = (int *) malloc(bufsize);
    if (!buf) {
        fprintf(stderr, "Unable to allocated %d bytes\n", bufsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    result = (int *) malloc(bufsize);
    if (!result) {
        fprintf(stderr, "Unable to allocated %d bytes\n", bufsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    rcount = 16;
    rsize = rcount * sizeof(int);
    rmabuf = (int *) malloc(rsize);
    if (!rmabuf) {
        fprintf(stderr, "Unable to allocated %d bytes\n", rsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, 1, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        MPI_Win_create(buf, bufsize, sizeof(int), MPI_INFO_NULL, comm, &win);
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        /** TEST OPERATIONS USING ACTIVE TARGET (FENCE) SYNCHRONIZATION **/
        MPI_Win_fence(0, win);

        TEST_FENCE_OP("Put",
                      MPI_Put(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0, rcount, MPI_INT, win);
);

        TEST_FENCE_OP("Get",
                      MPI_Get(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0, rcount, MPI_INT, win);
);
        TEST_FENCE_OP("Accumulate",
                      MPI_Accumulate(rmabuf, rcount, MPI_INT, MPI_PROC_NULL,
                                     0, rcount, MPI_INT, MPI_SUM, win);
);
        TEST_FENCE_OP("Get accumulate",
                      MPI_Get_accumulate(rmabuf, rcount, MPI_INT, result,
                                         rcount, MPI_INT, MPI_PROC_NULL, 0,
                                         rcount, MPI_INT, MPI_SUM, win);
);
        TEST_FENCE_OP("Fetch and op",
                      MPI_Fetch_and_op(rmabuf, result, MPI_INT, MPI_PROC_NULL, 0, MPI_SUM, win);
);
        TEST_FENCE_OP("Compare and swap",
                      MPI_Compare_and_swap(rmabuf, &rank, result, MPI_INT, MPI_PROC_NULL, 0, win);
);

        /** TEST OPERATIONS USING PASSIVE TARGET SYNCHRONIZATION **/

        TEST_PT_OP("Put", MPI_Put(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0, rcount, MPI_INT, win);
);
        TEST_PT_OP("Get", MPI_Get(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0, rcount, MPI_INT, win);
);
        TEST_PT_OP("Accumulate",
                   MPI_Accumulate(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0,
                                  rcount, MPI_INT, MPI_SUM, win);
);
        TEST_PT_OP("Get accumulate",
                   MPI_Get_accumulate(rmabuf, rcount, MPI_INT, result, rcount,
                                      MPI_INT, MPI_PROC_NULL, 0, rcount, MPI_INT, MPI_SUM, win);
);
        TEST_PT_OP("Fetch and op",
                   MPI_Fetch_and_op(rmabuf, result, MPI_INT, MPI_PROC_NULL, 0, MPI_SUM, win);
);
        TEST_PT_OP("Compare and swap",
                   MPI_Compare_and_swap(rmabuf, &rank, result, MPI_INT, MPI_PROC_NULL, 0, win);
);

        /** TEST REQUEST-BASED OPERATIONS (PASSIVE TARGET ONLY) **/

        TEST_REQ_OP("Rput", req,
                    MPI_Rput(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0, rcount, MPI_INT, win, &req);
);
        TEST_REQ_OP("Rget", req,
                    MPI_Rget(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0, rcount, MPI_INT, win, &req);
);
        TEST_REQ_OP("Raccumulate", req,
                    MPI_Raccumulate(rmabuf, rcount, MPI_INT, MPI_PROC_NULL, 0,
                                    rcount, MPI_INT, MPI_SUM, win, &req);
);
        TEST_REQ_OP("Rget_accumulate", req,
                    MPI_Rget_accumulate(rmabuf, rcount, MPI_INT, result,
                                        rcount, MPI_INT, MPI_PROC_NULL, 0,
                                        rcount, MPI_INT, MPI_SUM, win, &req);
);

        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    free(result);
    free(buf);
    free(rmabuf);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
