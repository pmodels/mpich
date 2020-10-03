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
#include <assert.h>

/*
static char MTEST_Descrip[] = "Get with Fence";
*/

static mtest_mem_type_e origmem;
static mtest_mem_type_e targetmem;
static void *targetbuf_h;

static inline int test(MPI_Comm comm, int rank, int orig, int target,
                       MPI_Aint count, MPI_Aint maxbufsize,
                       DTP_obj_s orig_obj, DTP_pool_s dtp, DTP_obj_s target_obj, void *targetbuf,
                       MPI_Win win)
{
    int errs = 0, err;
    MPI_Aint origcount, targetcount;
    MPI_Aint extent, lb;
    MPI_Datatype origtype, targettype;
    void *origbuf;
    void *origbuf_h;

    origtype = orig_obj.DTP_datatype;
    origcount = orig_obj.DTP_type_count;
    targettype = target_obj.DTP_datatype;
    targetcount = target_obj.DTP_type_count;

    char *orig_desc, *target_desc;
    DTP_obj_get_description(orig_obj, &orig_desc);
    DTP_obj_get_description(target_obj, &target_desc);
    MTestPrintfMsg(1,
                   "Getting count = %ld of origtype %s - count = %ld target type %s\n",
                   origcount, orig_desc, targetcount, target_desc);

    if (rank == target) {
#if defined(USE_GET)
        err = DTP_obj_buf_init(target_obj, targetbuf_h, 0, 1, count);
        if (err != DTP_SUCCESS) {
            return ++errs;
        }
        MTestCopyContent(targetbuf_h, targetbuf, maxbufsize, targetmem);
#elif defined(USE_PUT)
        err = DTP_obj_buf_init(target_obj, targetbuf_h, -1, -1, count);
        if (err != DTP_SUCCESS) {
            return ++errs;
        }
        MTestCopyContent(targetbuf_h, targetbuf, maxbufsize, targetmem);
#endif

        /* The target does not need to do anything besides the
         * fence */
        MPI_Win_fence(0, win);
        MPI_Win_fence(0, win);

#if defined(USE_PUT)
        MTestCopyContent(targetbuf, targetbuf_h, maxbufsize, targetmem);
        err = DTP_obj_buf_check(target_obj, targetbuf_h, 0, 1, count);
        if (err != DTP_SUCCESS) {
            errs++;
            if (errs < 10) {
                printf
                    ("Data in target buffer did not match for target datatype %s (get with orig datatype %s)\n",
                     target_desc, orig_desc);
            }
        }
#endif
    } else if (rank == orig) {
        MTestAlloc(orig_obj.DTP_bufsize, origmem, &origbuf_h, &origbuf, 0);
        assert(origbuf && origbuf_h);

#if defined(USE_GET)
        err = DTP_obj_buf_init(orig_obj, origbuf_h, -1, -1, count);
        if (err != DTP_SUCCESS) {
            return ++errs;
        }
        MTestCopyContent(origbuf_h, origbuf, orig_obj.DTP_bufsize, origmem);
#elif defined(USE_PUT)
        err = DTP_obj_buf_init(orig_obj, origbuf_h, 0, 1, count);
        if (err != DTP_SUCCESS) {
            return ++errs;
        }
        MTestCopyContent(origbuf_h, origbuf, orig_obj.DTP_bufsize, origmem);
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
         * transfering data, as a send/recv pair */
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
        MTestCopyContent(origbuf, origbuf_h, orig_obj.DTP_bufsize, origmem);
        err = DTP_obj_buf_check(orig_obj, origbuf_h, 0, 1, count);
        if (err != DTP_SUCCESS) {
            errs++;
            if (errs < 10) {
                printf
                    ("Data in origin buffer did not match for origin datatype %s (get with target datatype %s)\n",
                     orig_desc, target_desc);
            }
        }
#endif

        MTestFree(origmem, origbuf_h, origbuf);
    } else {
        MPI_Win_fence(0, win);
        MPI_Win_fence(0, win);
    }

    free(orig_desc);
    free(target_desc);

    return errs;
}


int main(int argc, char *argv[])
{
    int err, errs = 0;
    int rank, size, orig, target;
    int minsize = 2;
    int i;
    int seed, testsize;
    MPI_Aint count, maxbufsize;
    MPI_Comm comm;
    DTP_pool_s dtp;
    DTP_obj_s orig_obj, target_obj;
    char *basic_type;
    void *targetbuf;
    MPI_Aint extent, lb;
    MPI_Win win;

    MTest_Init(&argc, &argv);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");
    origmem = MTestArgListGetMemType(head, "origmem");
    targetmem = MTestArgListGetMemType(head, "targetmem");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating orig pool (%s,%ld)\n", basic_type, count);
        fflush(stderr);
    }

    MTestArgListDestroy(head);

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

    MTestAlloc(maxbufsize, targetmem, &targetbuf_h, &targetbuf, 0);
    assert(targetbuf && targetbuf_h);

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

            errs +=
                test(comm, rank, orig, target, count, maxbufsize, orig_obj, dtp, target_obj,
                     targetbuf, win);

            DTP_obj_free(target_obj);
            DTP_obj_free(orig_obj);
        }
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    MTestFree(targetmem, targetbuf_h, targetbuf);

  fn_exit:
    DTP_pool_free(dtp);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
