/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Get with Fence";
*/

int world_rank, world_size;
struct mtest_obj orig, target;

static inline int test(MPI_Comm comm, int rank, int orig_rank, int target_rank,
                       MPI_Aint count, DTP_pool_s dtp, MPI_Win win)
{
    int errs = 0, err;
    MPI_Aint origcount, targetcount;
    MPI_Aint extent, lb;
    MPI_Datatype origtype, targettype;

    origtype = orig.dtp_obj.DTP_datatype;
    origcount = orig.dtp_obj.DTP_type_count;
    targettype = target.dtp_obj.DTP_datatype;
    targetcount = target.dtp_obj.DTP_type_count;

    if (rank == target_rank) {
#if defined(USE_GET)
        MTest_dtp_init(&target, 0, 1, count);
#elif defined(USE_PUT)
        MTest_dtp_init(&target, -1, -1, count);
#endif

        /* The target does not need to do anything besides the
         * fence */
        MPI_Win_fence(0, win);
        MPI_Win_fence(0, win);

#if defined(USE_PUT)
        errs += MTest_dtp_check(&target, 0, 1, count, &orig, errs < 10);
#endif
    } else if (rank == orig_rank) {
#if defined(USE_GET)
        MTest_dtp_init(&orig, -1, -1, count);
#elif defined(USE_PUT)
        MTest_dtp_init(&orig, 0, 1, count);
#endif

        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        MPI_Win_fence(0, win);

        if (MTestIsBasicDtype(dtp.DTP_base_type)) {
            MPI_Type_get_extent(dtp.DTP_base_type, &lb, &extent);
        } else {
            /* if the base datatype is not a basic datatype, use an
             * extent of 1 */
            extent = 1;
        }

        /* This should have the same effect, in terms of
         * transferring data, as a send/recv pair */
#if defined(USE_GET)
        err = MPI_Get((char *) orig.buf + orig.dtp_obj.DTP_buf_offset,
                      origcount, origtype, target_rank,
                      target.dtp_obj.DTP_buf_offset / extent, targetcount, targettype, win);
#elif defined(USE_PUT)
        err = MPI_Put((char *) orig.buf + orig.dtp_obj.DTP_buf_offset,
                      origcount, origtype, target_rank,
                      target.dtp_obj.DTP_buf_offset / extent, targetcount, targettype, win);
#endif
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
#if defined(USE_GET)
        errs += MTest_dtp_check(&orig, 0, 1, count, &target, errs < 10);
#endif
    } else {
        MPI_Win_fence(0, win);
        MPI_Win_fence(0, win);
    }

    return errs;
}

static int fence_test(int seed, int testsize, int count, const char *basic_type,
                      mtest_mem_type_e origmem, mtest_mem_type_e targetmem)
{
    int err, errs = 0;
    int rank, size, orig_rank, target_rank;
    int minsize = 2;
    int i;
    MPI_Comm comm;
    DTP_pool_s dtp;
    MPI_Aint extent, lb;
    MPI_Win win;

    static char test_desc[200];
    snprintf(test_desc, 200,
#if defined(USE_GET)
             "./getfence -seed=%d -testsize=%d -type=%s -count=%d -origmem=%s -targetmem=%s",
#elif defined(USE_PUT)
             "./putfence -seed=%d -testsize=%d -type=%s -count=%d -origmem=%s -targetmem=%s",
#endif
             seed, testsize, basic_type, count, MTest_memtype_name(origmem),
             MTest_memtype_name(targetmem));
    if (world_rank == 0) {
        MTestPrintfMsg(1, " %s\n", test_desc);
    }

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating orig pool (%s,%d)\n", basic_type, count);
        fflush(stderr);
    }

    if (MTestIsBasicDtype(dtp.DTP_base_type)) {
        MPI_Type_get_extent(dtp.DTP_base_type, &lb, &extent);
    } else {
        /* if the base datatype is not a basic datatype, use an extent
         * of 1 */
        extent = 1;
    }

    int type_size;
    MPI_Type_size(dtp.DTP_base_type, &type_size);
    if (type_size * count > MTestDefaultMaxBufferSize()) {
        /* if the type size or count are too large, we do not have too
         * many objects in the pool that we can search for.  in such
         * cases, simply return. */
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

        for (i = 0; i < testsize; i++) {
            errs += MTest_dtp_create(&orig, true);
            errs += MTest_dtp_create(&target, false);

            errs += test(comm, rank, orig_rank, target_rank, count, dtp, win);

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
        errs +=
            fence_test(dtp_args.seed, dtp_args.testsize, dtp_args.count, dtp_args.basic_type,
                       dtp_args.u.rma.origmem, dtp_args.u.rma.targetmem);

    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
