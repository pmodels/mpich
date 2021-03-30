/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Test for streaming ACC-like operations with lock";
*/

enum acc_type {
    ACC_TYPE__ACC,
    ACC_TYPE__GACC,
};

MTEST_DTP_DECLARE(orig);
MTEST_DTP_DECLARE(target);
MTEST_DTP_DECLARE(result);
static MPI_Aint base_type_size;
static MPI_Aint maxbufsize;

int world_rank, world_size;

static int run_test(MPI_Comm comm, MPI_Win win, int count, enum acc_type acc)
{
    int rank, size, orig, target;
    int target_start_idx, target_end_idx;
    MPI_Datatype origtype, targettype, resulttype;
    MPI_Aint origcount, targetcount, resultcount;
    enum {
        LOCK_TYPE__LOCK,
        LOCK_TYPE__LOCKALL,
    } lock_type;
    enum {
        FLUSH_TYPE__NONE,
        FLUSH_TYPE__FLUSH,
        FLUSH_TYPE__FLUSH_ALL,
    } flush_type;
    enum {
        FLUSH_LOCAL_TYPE__NONE,
        FLUSH_LOCAL_TYPE__FLUSH_LOCAL,
        FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL,
    } flush_local_type;
    int err, errs = 0;


    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

#if defined(SINGLE_ORIGIN) && defined(SINGLE_TARGET)
    orig = 0;
    target = size - 1;
    target_start_idx = target;
    target_end_idx = target;
    lock_type = LOCK_TYPE__LOCK;
#elif defined(SINGLE_ORIGIN) && defined(MULTI_TARGET)
    orig = 0;
    if (rank == orig)
        target = size - 1;
    else
        target = rank;
    target_start_idx = orig + 1;
    target_end_idx = target;
    lock_type = LOCK_TYPE__LOCKALL;
#elif defined(MULTI_ORIGIN) && defined(SINGLE_TARGET)
    target = size - 1;
    if (rank == target)
        orig = 0;
    else
        orig = rank;
    target_start_idx = target;
    target_end_idx = target;
    lock_type = LOCK_TYPE__LOCK;
#else
#error "SINGLE|MULTI_ORIGIN and SINGLE|MULTI_TARGET not defined"
#endif

#if defined(USE_FLUSH)
    flush_type = FLUSH_TYPE__FLUSH;
#elif defined(USE_FLUSH_ALL)
    flush_type = FLUSH_TYPE__FLUSH_ALL;
#elif defined(USE_FLUSH_NONE)
    flush_type = FLUSH_TYPE__NONE;
#else
#error "FLUSH_TYPE not defined"
#endif

#if defined(USE_FLUSH_LOCAL)
    flush_local_type = FLUSH_LOCAL_TYPE__FLUSH_LOCAL;
#elif defined(USE_FLUSH_LOCAL_ALL)
    flush_local_type = FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL;
#elif defined(USE_FLUSH_LOCAL_NONE)
    flush_local_type = FLUSH_LOCAL_TYPE__NONE;
#else
#error "FLUSH_LOCAL_TYPE not defined"
#endif

    /* we do two barriers: the first one informs the target(s) that
     * the RMA operation should be visible at the target, and the
     * second one informs the origin that the target is done
     * checking. */
    if (rank == orig) {
        MTest_dtp_malloc_obj(orig, 0);
        MTest_dtp_init(orig, 0, 1, count);

        if (lock_type == LOCK_TYPE__LOCK) {
            MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
        } else {
            MPI_Win_lock_all(0, win);
        }

        origcount = orig_obj.DTP_type_count;
        origtype = orig_obj.DTP_datatype;
        targetcount = target_obj.DTP_type_count;
        targettype = target_obj.DTP_datatype;
        resultcount = result_obj.DTP_type_count;
        resulttype = result_obj.DTP_datatype;

        int t;
        if (acc == ACC_TYPE__ACC) {
            for (t = target_start_idx; t <= target_end_idx; t++) {
                MPI_Accumulate(origbuf + orig_obj.DTP_buf_offset, origcount,
                               origtype, t, target_obj.DTP_buf_offset / base_type_size, targetcount,
                               targettype, MPI_REPLACE, win);
            }
        } else {
            MTest_dtp_malloc_obj(result, 1);

#if !defined(MULTI_ORIGIN) && !defined(MULTI_TARGET)
            MTest_dtp_init(result, -1, -1, count);
#endif

            for (t = target_start_idx; t <= target_end_idx; t++) {
                MPI_Get_accumulate(origbuf + orig_obj.DTP_buf_offset, origcount,
                                   origtype, resultbuf + result_obj.DTP_buf_offset, resultcount,
                                   resulttype, t, target_obj.DTP_buf_offset / base_type_size,
                                   targetcount, targettype, MPI_REPLACE, win);
            }
        }

        if (flush_local_type == FLUSH_LOCAL_TYPE__FLUSH_LOCAL) {
            for (t = target_start_idx; t <= target_end_idx; t++)
                MPI_Win_flush_local(t, win);
            /* reset the send buffer to test local completion */
            MTest_dtp_init(orig, 0, 0, count);
        } else if (flush_local_type == FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL) {
            MPI_Win_flush_local_all(win);
            /* reset the send buffer to test local completion */
            MTest_dtp_init(orig, 0, 0, count);
        }

        if (flush_type == FLUSH_TYPE__FLUSH_ALL) {
            MPI_Win_flush_all(win);
            MPI_Barrier(comm);
            MPI_Barrier(comm);
        } else if (flush_type == FLUSH_TYPE__FLUSH) {
            for (t = target_start_idx; t <= target_end_idx; t++)
                MPI_Win_flush(t, win);
            MPI_Barrier(comm);
            MPI_Barrier(comm);
        }

        if (lock_type == LOCK_TYPE__LOCK) {
            MPI_Win_unlock(target, win);
        } else {
            MPI_Win_unlock_all(win);
        }

        if (flush_type == FLUSH_TYPE__NONE) {
            MPI_Barrier(comm);
            MPI_Barrier(comm);
        }

        if (acc != ACC_TYPE__ACC) {
#if !defined(MULTI_ORIGIN) && !defined(MULTI_TARGET)
            /* check get results */
            /* this check is not valid for multi-origin tests, as some
             * origins might receive the value that has already been
             * overwritten by other origins */
            MTest_dtp_check(result, 1, 2, count);
#endif

            MTest_dtp_free(result);
        }

        MTest_dtp_free(orig);
    } else if (rank == target) {
        MPI_Barrier(comm);
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);

        MTest_dtp_check(target, 0, 1, count);
        MTest_dtp_init(target, 1, 2, count);

        MPI_Win_unlock(rank, win);
        MPI_Barrier(comm);
    } else {
        MPI_Barrier(comm);
        MPI_Barrier(comm);
    }

    return errs;
}

int main(int argc, char *argv[])
{
    int err, errs = 0;
    int rank, size, orig, target;
    int minsize = 2;
    int i, seed, testsize;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint lb;
    MPI_Aint count;
    DTP_pool_s dtp;
    char *basic_type;

    MTest_Init(&argc, &argv);

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");
    origmem = MTestArgListGetMemType(head, "origmem");
    targetmem = MTestArgListGetMemType(head, "targetmem");
    resultmem = MTestArgListGetMemType(head, "resultmem");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stderr, "Error while creating orig pool (%s,%ld)\n", basic_type, count);
        fflush(stderr);
    }

    MTestArgListDestroy(head);

    if (MTestIsBasicDtype(dtp.DTP_base_type)) {
        MPI_Type_get_extent(dtp.DTP_base_type, &lb, &base_type_size);
    } else {
        /* accumulate tests cannot use struct types */
        goto fn_exit;
    }

    MTest_dtp_malloc_max(target, 2);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL) {
            /* for NULL comms, make sure these processes create the
             * same number of objects, so the target knows what
             * datatype layout to check for */
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, maxbufsize, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, maxbufsize, testsize);
            errs += MTEST_CREATE_AND_FREE_DTP_OBJS(dtp, maxbufsize, testsize);
            continue;
        }

        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        MPI_Win_create(targetbuf, maxbufsize, base_type_size, MPI_INFO_NULL, comm, &win);

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

            err = DTP_obj_create(dtp, &result_obj, maxbufsize);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            MTest_dtp_init(target, 1, 2, count);

            /* do a barrier to ensure that the target buffer is
             * initialized before we start writing data to it */
            MPI_Barrier(comm);

            errs += run_test(comm, win, count, ACC_TYPE__ACC);
            errs += run_test(comm, win, count, ACC_TYPE__GACC);

            DTP_obj_free(orig_obj);
            DTP_obj_free(target_obj);
            DTP_obj_free(result_obj);
        }
        MPI_Win_free(&win);
        MTestFreeComm(&comm);
    }

    MTest_dtp_free(target);

  fn_exit:
    DTP_pool_free(dtp);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
