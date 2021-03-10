/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "mpitest.h"
#include "dtpools.h"
#include "mtest_dtp.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Get with Fence";
*/

int world_rank, world_size;
MTEST_DTP_DECLARE(orig);
MTEST_DTP_DECLARE(target);

static inline int test(MPI_Comm comm, int rank, int orig, int target,
                       MPI_Aint count, DTP_pool_s dtp, MPI_Win win)
{
    int errs = 0, err;
    MPI_Aint origcount, targetcount;
    MPI_Aint extent, lb;
    MPI_Datatype origtype, targettype;

    origtype = orig_obj.DTP_datatype;
    origcount = orig_obj.DTP_type_count;
    targettype = target_obj.DTP_datatype;
    targetcount = target_obj.DTP_type_count;

    char *orig_desc, *target_desc;
    DTP_obj_get_description(orig_obj, &orig_desc);
    DTP_obj_get_description(target_obj, &target_desc);

    if (rank == target) {
#if defined(USE_GET)
        MTest_dtp_init(target, 0, 1, count);
#elif defined(USE_PUT)
        MTest_dtp_init(target, -1, -1, count);
#endif

        /* The target does not need to do anything besides the
         * fence */
        MPI_Win_fence(0, win);
        MPI_Win_fence(0, win);

#if defined(USE_PUT)
        MTest_dtp_check(target, 0, 1, count);
        if (err != DTP_SUCCESS && errs < 10) {
            printf
                ("Data in target buffer did not match for target datatype %s (get with orig datatype %s)\n",
                 target_desc, orig_desc);
        }
#endif
    } else if (rank == orig) {
        MTest_dtp_malloc_obj(orig, 0);

#if defined(USE_GET)
        MTest_dtp_init(orig, -1, -1, count);
#elif defined(USE_PUT)
        MTest_dtp_init(orig, 0, 1, count);
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
        err =
            MPI_Get(origbuf + orig_obj.DTP_buf_offset, origcount, origtype, target,
                    target_obj.DTP_buf_offset / extent, targetcount, targettype, win);
#elif defined(USE_PUT)
        err =
            MPI_Put(origbuf + orig_obj.DTP_buf_offset, origcount, origtype, target,
                    target_obj.DTP_buf_offset / extent, targetcount, targettype, win);
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
        MTest_dtp_check(orig, 0, 1, count);
        if (err != DTP_SUCCESS && errs < 10) {
            printf
                ("Data in origin buffer did not match for origin datatype %s (get with target datatype %s)\n",
                 orig_desc, target_desc);
        }
#endif

        MTest_dtp_free(orig);
    } else {
        MPI_Win_fence(0, win);
        MPI_Win_fence(0, win);
    }

    free(orig_desc);
    free(target_desc);

    return errs;
}

static int fence_test(int seed, int testsize, int count, const char *basic_type)
{
    int err, errs = 0;
    int rank, size, orig, target;
    int minsize = 2;
    int i;
    MPI_Aint maxbufsize;
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

    maxbufsize = MTestDefaultMaxBufferSize();

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
    if (type_size * count > maxbufsize) {
        /* if the type size or count are too large, we do not have too
         * many objects in the pool that we can search for.  in such
         * cases, simply return. */
        goto fn_exit;
    }

    MTest_dtp_malloc_max(target, 1);

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL) {
            /* for NULL comms, make sure these processes create the
             * same number of objects, so the target knows what
             * datatype layout to check for */
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, maxbufsize, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, maxbufsize, testsize);
            continue;
        }

        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        orig = 0;
        target = size - 1;

        MPI_Win_create(targetbuf, maxbufsize, extent, MPI_INFO_NULL, comm, &win);

        for (i = 0; i < testsize; i++) {
            err = DTP_obj_create(dtp, &orig_obj, maxbufsize);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            err = DTP_obj_create(dtp, &target_obj, maxbufsize);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            errs += test(comm, rank, orig, target, count, dtp, win);

            DTP_obj_free(target_obj);
            DTP_obj_free(orig_obj);
        }
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    MTest_dtp_free(target);

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
        origmem = dtp_args.u.rma.origmem;
        targetmem = dtp_args.u.rma.targetmem;
        errs += fence_test(dtp_args.seed, dtp_args.testsize, dtp_args.count, dtp_args.basic_type);

    }
    dtp_args_finalize(&dtp_args);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
