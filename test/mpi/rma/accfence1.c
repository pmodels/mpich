/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Accumulate/Replace with Fence";
*/

#define error(...)                              \
    do {                                        \
        fprintf(stderr, __VA_ARGS__);           \
        fflush(stderr);                         \
    } while (0)

int world_rank, world_size;

static int accfence_test(int seed, int testsize, int count, const char *basic_type,
                         mtest_mem_type_e origmem, mtest_mem_type_e targetmem)
{
    int errs = 0, err;
    int rank, size, orig_rank, target_rank;
    int minsize = 2;
    int i;
    MPI_Aint origcount, targetcount;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint extent, lb;
    MPI_Datatype origtype, targettype;
    DTP_pool_s dtp;
    struct mtest_obj orig, target;

    static char test_desc[200];
    snprintf(test_desc, 200,
             "./accfence1 -seed=%d -testsize=%d -type=%s -count=%d -origmem=%s -targetmem=%s",
             seed, testsize, basic_type, count, MTest_memtype_name(origmem),
             MTest_memtype_name(targetmem));
    if (world_rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        error("Error while creating orig pool (%s,%d)\n", basic_type, count);
    }

    if (MTestIsBasicDtype(dtp.DTP_base_type)) {
        MPI_Type_get_extent(dtp.DTP_base_type, &lb, &extent);
    } else {
        /* accumulate tests cannot use struct types */
        goto fn_exit;
    }

    MTest_dtp_obj_start(&orig, "origin", dtp, origmem, 0, false);
    MTest_dtp_obj_start(&target, "target", dtp, targetmem, 1, true);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL) {
            /* for NULL comms, make sure these processes create the
             * same number of objects, so the target knows what
             * datatype layout to check for */
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, testsize);
            continue;
        }

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        orig_rank = 0;
        target_rank = size - 1;

        MPI_Win_create(target.buf, target.maxbufsize, extent, MPI_INFO_NULL, comm, &win);

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        for (i = 0; i < testsize; i++) {
            errs += MTest_dtp_create(&orig, rank == orig_rank);
            errs += MTest_dtp_create(&target, false);

            MTest_dtp_init(&target, -1, -1, count);

            targetcount = target.dtp_obj.DTP_type_count;
            targettype = target.dtp_obj.DTP_datatype;

            MPI_Win_fence(0, win);

            if (rank == orig_rank) {
                MTest_dtp_init(&orig, 0, 1, count);

                origcount = orig.dtp_obj.DTP_type_count;
                origtype = orig.dtp_obj.DTP_datatype;

                /* MPI_REPLACE on accumulate is almost the same
                 * as MPI_Put; the only difference is in the
                 * handling of overlapping accumulate operations,
                 * which are not tested here */
                err = MPI_Accumulate((char *) orig.buf + orig.dtp_obj.DTP_buf_offset, origcount,
                                     origtype, target_rank, target.dtp_obj.DTP_buf_offset / extent,
                                     targetcount, targettype, MPI_REPLACE, win);
                if (err) {
                    errs++;
                    if (errs < 10) {
                        MTestPrintError(err);
                    }
                }
                err = MPI_Win_fence(0, win);
                if (err) {
                    errs++;
                    if (errs < 10) {
                        MTestPrintError(err);
                    }
                }
            } else if (rank == target_rank) {
                MPI_Win_fence(0, win);
                /* This should have the same effect, in terms of
                 * transferring data, as a send/recv pair */
                errs += MTest_dtp_check(&target, 0, 1, count, &orig, errs < 10);
            } else {
                MPI_Win_fence(0, win);
            }
            MTest_dtp_destroy(&orig);
            MTest_dtp_destroy(&target);
        }
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    MTest_dtp_obj_finish(&orig);
    MTest_dtp_obj_finish(&target);

  fn_exit:
    DTP_pool_free(dtp);
    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    struct dtp_args dtp_args;
    dtp_args_init(&dtp_args, MTEST_DTP_RMA, argc, argv);
    while (dtp_args_get_next(&dtp_args)) {
        errs += accfence_test(dtp_args.seed, dtp_args.testsize,
                              dtp_args.count, dtp_args.basic_type,
                              dtp_args.u.rma.origmem, dtp_args.u.rma.targetmem);

    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
