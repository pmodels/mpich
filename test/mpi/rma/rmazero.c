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

#define TARGET 0

/* Test the given operation within a Fence epoch */
#define TEST_FENCE_OP(op_name_, fcn_call_)                              \
    do {                                                                \
        err = fcn_call_                                                 \
        if (err) {                                                      \
            errs++;                                                     \
            if (errs < 10) {                                            \
                MTestPrintErrorMsg("Zero-byte op " op_name_, err);    \
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
        err = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, TARGET, 0, win);         \
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
                MTestPrintErrorMsg("Zero-byte op " op_name_, err);    \
            }                                                           \
        }                                                               \
        err = MPI_Win_unlock(TARGET, win);                            \
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
        err = MPI_Win_lock(MPI_LOCK_EXCLUSIVE, TARGET, 0, win);         \
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
                MTestPrintErrorMsg("Zero-byte op " op_name_, err);    \
            }                                                           \
        }                                                               \
        err = MPI_Win_unlock(TARGET, win);                            \
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
static char MTEST_Descrip[] = "Test handling of zero-byte transfers";
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
    MPI_Datatype derived_dtp;

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

    MPI_Type_contiguous(2, MPI_INT, &derived_dtp);
    MPI_Type_commit(&derived_dtp);

    /* The following loop is used to run through a series of communicators
     * that are subsets of MPI_COMM_WORLD, of size 1 or greater. */
    while (MTestGetIntracommGeneral(&comm, 1, 1)) {
        int count = 0;

        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        MPI_Win_create(buf, bufsize, 2 * sizeof(int), MPI_INFO_NULL, comm, &win);
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        /** TEST OPERATIONS USING ACTIVE TARGET (FENCE) SYNCHRONIZATION **/
        MPI_Win_fence(0, win);

        TEST_FENCE_OP("Put", MPI_Put(rmabuf, count, MPI_INT, TARGET, 0, count, MPI_INT, win);
);

        TEST_FENCE_OP("Get", MPI_Get(rmabuf, count, MPI_INT, TARGET, 0, count, MPI_INT, win);
);
        TEST_FENCE_OP("Accumulate",
                      MPI_Accumulate(rmabuf, count, MPI_INT, TARGET,
                                     0, count, MPI_INT, MPI_SUM, win);
);
        TEST_FENCE_OP("Accumulate_derived",
                      MPI_Accumulate(rmabuf, count, derived_dtp, TARGET,
                                     0, count, derived_dtp, MPI_SUM, win);
);
        TEST_FENCE_OP("Get accumulate",
                      MPI_Get_accumulate(rmabuf, count, MPI_INT, result,
                                         count, MPI_INT, TARGET, 0, count, MPI_INT, MPI_SUM, win);
);
        /* Note: It's not possible to generate a zero-byte FOP or CAS */

        /** TEST OPERATIONS USING PASSIVE TARGET SYNCHRONIZATION **/

        TEST_PT_OP("Put", MPI_Put(rmabuf, count, MPI_INT, TARGET, 0, count, MPI_INT, win);
);
        TEST_PT_OP("Get", MPI_Get(rmabuf, count, MPI_INT, TARGET, 0, count, MPI_INT, win);
);
        TEST_PT_OP("Accumulate",
                   MPI_Accumulate(rmabuf, count, MPI_INT, TARGET, 0, count, MPI_INT, MPI_SUM, win);
);
        TEST_PT_OP("Accumulate_derived",
                   MPI_Accumulate(rmabuf, count, derived_dtp, TARGET, 0,
                                  count, derived_dtp, MPI_SUM, win);
);
        TEST_PT_OP("Get accumulate",
                   MPI_Get_accumulate(rmabuf, count, MPI_INT, result, count,
                                      MPI_INT, TARGET, 0, count, MPI_INT, MPI_SUM, win);
);

        /* Note: It's not possible to generate a zero-byte FOP or CAS */

        /** TEST REQUEST-BASED OPERATIONS (PASSIVE TARGET ONLY) **/

        TEST_REQ_OP("Rput", req,
                    MPI_Rput(rmabuf, count, MPI_INT, TARGET, 0, count, MPI_INT, win, &req);
);
        TEST_REQ_OP("Rget", req,
                    MPI_Rget(rmabuf, count, MPI_INT, TARGET, 0, count, MPI_INT, win, &req);
);
        TEST_REQ_OP("Raccumulate", req,
                    MPI_Raccumulate(rmabuf, count, MPI_INT, TARGET, 0,
                                    count, MPI_INT, MPI_SUM, win, &req);
);
        TEST_REQ_OP("Raccumulate_derived", req,
                    MPI_Raccumulate(rmabuf, count, derived_dtp, TARGET, 0,
                                    count, derived_dtp, MPI_SUM, win, &req);
);
        TEST_REQ_OP("Rget_accumulate", req,
                    MPI_Rget_accumulate(rmabuf, count, MPI_INT, result,
                                        count, MPI_INT, TARGET, 0,
                                        count, MPI_INT, MPI_SUM, win, &req);
);

        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    MPI_Type_free(&derived_dtp);

    free(result);
    free(buf);
    free(rmabuf);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
