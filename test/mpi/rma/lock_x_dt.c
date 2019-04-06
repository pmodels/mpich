/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Test for streaming ACC-like operations with lock";
*/

enum acc_type {
    ACC_TYPE__ACC,
    ACC_TYPE__GACC,
};

static int run_test(MPI_Comm comm, MPI_Win win, DTP_t orig_dtp, int orig_idx,
                    DTP_t target_dtp, int target_idx, int count, enum acc_type acc)
{
    int rank, size, orig, target;
    int target_start_idx, target_end_idx;
    MPI_Aint origcount, targetcount;
    MPI_Datatype origtype, targettype;
    void *origbuf, *targetbuf;
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

    targetcount = target_dtp->DTP_obj_array[target_idx].DTP_obj_count;
    targettype = target_dtp->DTP_obj_array[target_idx].DTP_obj_type;
    targetbuf = target_dtp->DTP_obj_array[target_idx].DTP_obj_buf;

    origcount = orig_dtp->DTP_obj_array[orig_idx].DTP_obj_count;
    origtype = orig_dtp->DTP_obj_array[orig_idx].DTP_obj_type;
    origbuf = orig_dtp->DTP_obj_array[orig_idx].DTP_obj_buf;

    MPI_Aint lb, extent;
    MPI_Type_get_extent(targettype, &lb, &extent);

    MPI_Aint slb, sextent;
    MPI_Type_get_extent(origtype, &slb, &sextent);

    /* we do two barriers: the first one informs the target(s) that
     * the RMA operation should be visible at the target, and the
     * second one informs the origin that the target is done
     * checking. */
    if (rank == orig) {
        char *resbuf = (char *) calloc(lb + extent * targetcount, sizeof(char));

        if (lock_type == LOCK_TYPE__LOCK) {
            MPI_Win_lock(MPI_LOCK_SHARED, target, 0, win);
        } else {
            MPI_Win_lock_all(0, win);
        }

        int t;
        for (t = target_start_idx; t <= target_end_idx; t++) {
            if (acc == ACC_TYPE__ACC) {
                MPI_Accumulate(origbuf, origcount,
                               origtype, t, 0, targetcount, targettype, MPI_REPLACE, win);
            } else {
                MPI_Get_accumulate(origbuf, origcount,
                                   origtype, resbuf, targetcount, targettype,
                                   t, 0, targetcount, targettype, MPI_REPLACE, win);
            }
        }

        if (flush_local_type == FLUSH_LOCAL_TYPE__FLUSH_LOCAL) {
            for (t = target_start_idx; t <= target_end_idx; t++)
                MPI_Win_flush_local(t, win);

            /* reset the send buffer to test local completion */
            memset(origbuf, 0, slb + sextent * origcount);
        } else if (flush_local_type == FLUSH_LOCAL_TYPE__FLUSH_LOCAL_ALL) {
            MPI_Win_flush_local_all(win);
            /* reset the send buffer to test local completion */
            memset(origbuf, 0, slb + sextent * origcount);
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

        free(resbuf);
    } else if (rank == target) {
        MPI_Type_get_extent(targettype, &lb, &extent);

        char *tmp = (char *) calloc(lb + extent * targetcount, sizeof(char));
        memcpy(tmp, targetbuf, lb + extent * targetcount);

        MPI_Barrier(comm);
        MPI_Win_lock(MPI_LOCK_SHARED, rank, 0, win);

        err = DTP_obj_buf_check(target_dtp, target_idx, 0, 1, count);
        if (err != DTP_SUCCESS)
            errs++;

        /* restore target buffer */
        memcpy(targetbuf, tmp, lb + extent * targetcount);
        free(tmp);

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
    int minsize = 2, count;
    int i, j;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint lb, extent;
    DTP_t orig_dtp, target_dtp;

    MTest_Init(&argc, &argv);

#ifndef USE_DTP_POOL_TYPE__STRUCT       /* set in 'test/mpi/structtypetest.txt' to split tests */
    MPI_Datatype basic_type;
    int len;
    char type_name[MPI_MAX_OBJECT_NAME] = { 0 };

    err = MTestInitBasicSignature(argc, argv, &count, &basic_type);
    if (err)
        return MTestReturnValue(1);

    err = DTP_pool_create(basic_type, count, &orig_dtp);
    if (err != DTP_SUCCESS) {
        MPI_Type_get_name(basic_type, type_name, &len);
        fprintf(stdout, "Error while creating orig pool (%s,%d)\n", type_name, count);
        fflush(stdout);
    }

    err = DTP_pool_create(basic_type, count, &target_dtp);
    if (err != DTP_SUCCESS) {
        MPI_Type_get_name(basic_type, type_name, &len);
        fprintf(stdout, "Error while creating target pool (%s,%d)\n", type_name, count);
        fflush(stdout);
    }
#else
    MPI_Datatype *basic_types = NULL;
    int *basic_type_counts = NULL;
    int basic_type_num;

    err = MTestInitStructSignature(argc, argv, &basic_type_num, &basic_type_counts, &basic_types);
    if (err)
        return MTestReturnValue(1);

    err = DTP_pool_create_struct(basic_type_num, basic_types, basic_type_counts, &orig_dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stdout, "Error while creating struct pool\n");
        fflush(stdout);
    }

    err = DTP_pool_create_struct(basic_type_num, basic_types, basic_type_counts, &target_dtp);
    if (err != DTP_SUCCESS) {
        fprintf(stdout, "Error while creating struct pool\n");
        fflush(stdout);
    }

    /* this is ignored */
    count = 0;
#endif

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        for (i = 0; i < target_dtp->DTP_num_objs; i++) {
            err = DTP_obj_create(target_dtp, i, 0, 0, 0);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            MPI_Aint targetcount = target_dtp->DTP_obj_array[i].DTP_obj_count;
            MPI_Datatype targettype = target_dtp->DTP_obj_array[i].DTP_obj_type;
            void *targetbuf = target_dtp->DTP_obj_array[i].DTP_obj_buf;

            MPI_Aint lb, extent;
            MPI_Type_get_extent(targettype, &lb, &extent);
            MPI_Win_create(targetbuf, lb + targetcount * extent,
                           (int) extent, MPI_INFO_NULL, comm, &win);

            for (j = 0; j < orig_dtp->DTP_num_objs; j++) {
                err = DTP_obj_create(orig_dtp, j, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                run_test(comm, win, orig_dtp, j, target_dtp, i, count, ACC_TYPE__ACC);
                run_test(comm, win, orig_dtp, j, target_dtp, i, count, ACC_TYPE__GACC);

                DTP_obj_free(orig_dtp, j);
            }
            MPI_Win_free(&win);
            DTP_obj_free(target_dtp, i);
        }
        MTestFreeComm(&comm);
    }

    DTP_pool_free(orig_dtp);
    DTP_pool_free(target_dtp);

#ifdef USE_DTP_POOL_TYPE__STRUCT
    /* cleanup array if any */
    if (basic_types) {
        free(basic_types);
    }
    if (basic_type_counts) {
        free(basic_type_counts);
    }
#endif

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
