/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Get with Fence";
*/

#define MAX_COUNT_SIZE (16000000)
#define MAX_TYPE_SIZE  (16)


static inline int test(MPI_Comm comm, int rank, int orig, int target,
                       int i, int j, DTP_t orig_dtp, DTP_t target_dtp)
{
    int errs = 0, err;
    int disp_unit;
    int len;
    MPI_Aint origcount, targetcount, count;
    MPI_Aint extent, lb;
    MPI_Win win;
    MPI_Datatype origtype, targettype;
    char orig_name[MPI_MAX_OBJECT_NAME] = { 0 };
    char target_name[MPI_MAX_OBJECT_NAME] = { 0 };
    void *origbuf, *targetbuf;

    count = orig_dtp->DTP_type_signature.DTP_pool_basic.DTP_basic_type_count;
    origtype = orig_dtp->DTP_obj_array[i].DTP_obj_type;
    targettype = target_dtp->DTP_obj_array[j].DTP_obj_type;
    origcount = orig_dtp->DTP_obj_array[i].DTP_obj_count;
    targetcount = target_dtp->DTP_obj_array[j].DTP_obj_count;
    origbuf = orig_dtp->DTP_obj_array[i].DTP_obj_buf;
    targetbuf = target_dtp->DTP_obj_array[j].DTP_obj_buf;

    MPI_Type_get_name(origtype, orig_name, &len);
    MPI_Type_get_name(targettype, target_name, &len);

    MTestPrintfMsg(1,
                   "Getting count = %ld of origtype %s - count = %ld target type %s\n",
                   origcount, orig_name, targetcount, target_name);

    MPI_Type_get_extent(targettype, &lb, &extent);
    /* This makes sure that disp_unit does not become negative for large counts */
    disp_unit = extent < INT_MAX ? extent : 1;
    MPI_Win_create(targetbuf, targetcount * extent + lb, disp_unit, MPI_INFO_NULL, comm, &win);
    MPI_Win_fence(0, win);

    if (rank == target) {
        /* The target does not need to do anything besides the
         * fence */
        MPI_Win_fence(0, win);
    } else if (rank == orig) {
        /* To improve reporting of problems about operations, we
         * change the error handler to errors return */
        MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

        /* This should have the same effect, in terms of
         * transfering data, as a send/recv pair */
        err = MPI_Get(origbuf, origcount, origtype, target, 0, targetcount, targettype, win);
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
        err = DTP_obj_buf_check(orig_dtp, i, 0, 1, count);
        if (err != DTP_SUCCESS) {
            errs++;
            if (errs < 10) {
                printf
                    ("Data in orig buffer did not match for orig datatype %s (get with target datatype %s)\n",
                     orig_name, target_name);
            }
        }
    } else {
        MPI_Win_fence(0, win);
    }
    MPI_Win_free(&win);

    return errs;
}


int main(int argc, char *argv[])
{
    int err, errs = 0;
    int rank, size, orig, target;
    int minsize = 2;
    int i, j;
    int count;
    MPI_Aint bufsize;
    MPI_Comm comm;
    DTP_t orig_dtp, target_dtp;

    MTest_Init(&argc, &argv);

#ifndef USE_DTP_POOL_TYPE__STRUCT       /* set in 'test/mpi/structtypetest.txt' to split tests */
    MPI_Datatype basic_type;
    MPI_Aint lb, extent;
    int len;
    char type_name[MPI_MAX_OBJECT_NAME] = { 0 };

    err = MTestInitBasicSignature(argc, argv, &count, &basic_type);
    if (err)
        return MTestReturnValue(1);

    /* compute bufsize to limit number of comm in test */
    MPI_Type_get_extent(basic_type, &lb, &extent);
    bufsize = extent * count;

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

    /* TODO: ignore bufsize for structs for now;
     *       we need to compute bufsize also for
     *       this case */
    bufsize = 0;

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

    /* The following illustrates the use of the routines to
     * run through a selection of communicators and datatypes.
     * Use subsets of these for tests that do not involve combinations
     * of communicators, datatypes, and counts of datatypes */
    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        orig = 0;
        target = size - 1;

        for (i = 0; i < orig_dtp->DTP_num_objs; i++) {
            err = DTP_obj_create(orig_dtp, i, 0, 0, 0);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            for (j = 0; j < target_dtp->DTP_num_objs; j++) {
                err = DTP_obj_create(target_dtp, j, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                errs += test(comm, rank, orig, target, i, j, orig_dtp, target_dtp);

                DTP_obj_free(target_dtp, j);
            }
            DTP_obj_free(orig_dtp, i);
        }
        MTestFreeComm(&comm);

        /* for large buffers only do one communicator */
        if (MAX_COUNT_SIZE * MAX_TYPE_SIZE < bufsize) {
            break;
        }
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
