/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "dtpools.h"

/*
static char MTEST_Descrip[] = "Accumulate/Replace with Fence";
*/



int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size, orig, target;
    int minsize = 2, count;
    int i, j, len;
    MPI_Aint origcount, targetcount;
    MPI_Comm comm;
    MPI_Win win;
    MPI_Aint extent, lb;
    MPI_Datatype origtype, targettype;
    DTP_t orig_dtp, target_dtp;
    char orig_name[MPI_MAX_OBJECT_NAME] = { 0 };
    char target_name[MPI_MAX_OBJECT_NAME] = { 0 };
    void *origbuf, *targetbuf;

    MTest_Init(&argc, &argv);

#ifndef USE_DTP_POOL_TYPE__STRUCT       /* set in 'test/mpi/structtypetest.txt' to split tests */
    MPI_Datatype basic_type;
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

        for (i = 0; i < target_dtp->DTP_num_objs; i++) {
            err = DTP_obj_create(target_dtp, i, 0, 0, 0);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            targetcount = target_dtp->DTP_obj_array[i].DTP_obj_count;
            targettype = target_dtp->DTP_obj_array[i].DTP_obj_type;
            targetbuf = target_dtp->DTP_obj_array[i].DTP_obj_buf;

            MPI_Type_get_extent(targettype, &lb, &extent);
            MPI_Win_create(targetbuf, targetcount * extent + lb,
                           (int) extent, MPI_INFO_NULL, comm, &win);
            MPI_Win_fence(0, win);

            /* To improve reporting of problems about operations, we
             * change the error handler to errors return */
            MPI_Win_set_errhandler(win, MPI_ERRORS_RETURN);

            for (j = 0; j < orig_dtp->DTP_num_objs; j++) {
                err = DTP_obj_create(orig_dtp, j, 0, 1, count);
                if (err != DTP_SUCCESS) {
                    errs++;
                    break;
                }

                origcount = orig_dtp->DTP_obj_array[j].DTP_obj_count;
                origtype = orig_dtp->DTP_obj_array[j].DTP_obj_type;
                origbuf = orig_dtp->DTP_obj_array[j].DTP_obj_buf;

                if (rank == orig) {
                    /* MPI_REPLACE on accumulate is almost the same
                     * as MPI_Put; the only difference is in the
                     * handling of overlapping accumulate operations,
                     * which are not tested here */
                    err = MPI_Accumulate(origbuf, origcount,
                                         origtype, target, 0, targetcount, targettype, MPI_REPLACE,
                                         win);
                    if (err) {
                        errs++;
                        if (errs < 10) {
                            MPI_Type_get_name(origtype, orig_name, &len);
                            MPI_Type_get_name(targettype, target_name, &len);
                            fprintf(stdout, "Accumulate types: send %s, recv %s\n", orig_name,
                                    target_name);
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
                } else if (rank == target) {
                    MPI_Win_fence(0, win);
                    /* This should have the same effect, in terms of
                     * transfering data, as a send/recv pair */
                    err = DTP_obj_buf_check(target_dtp, i, 0, 1, count);
                    if (err != DTP_SUCCESS) {
                        if (errs < 10) {
                            MPI_Type_get_name(origtype, orig_name, &len);
                            MPI_Type_get_name(targettype, target_name, &len);
                            fprintf(stdout,
                                    "Data received with type %s does not match data sent with type %s\n",
                                    target_name, orig_name);
                            fflush(stdout);
                        }
                        errs++;
                    }
                } else {
                    MPI_Win_fence(0, win);
                }
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
