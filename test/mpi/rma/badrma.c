/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "stdio.h"

#define SIZE 100

MPI_Win win;
int win_buf[SIZE], origin_buf[SIZE], result_buf[SIZE];

int do_test(int origin_count, MPI_Datatype origin_type, int result_count,
            MPI_Datatype result_type, int target_count, MPI_Datatype target_type)
{
    int errs = 0, ret, origin_type_size, result_type_size;

    ret = MPI_Put(origin_buf, origin_count, origin_type, 1, 0, target_count, target_type, win);
    if (ret)
        errs++;

    ret = MPI_Get(origin_buf, origin_count, origin_type, 1, 0, target_count, target_type, win);
    if (ret)
        errs++;

    ret = MPI_Accumulate(origin_buf, origin_count, origin_type, 1, 0, target_count,
                         target_type, MPI_SUM, win);
    if (ret)
        errs++;

    ret = MPI_Get_accumulate(origin_buf, origin_count, origin_type, result_buf, result_count,
                             result_type, 1, 0, target_count, target_type, MPI_SUM, win);
    if (ret)
        errs++;

    MPI_Type_size(origin_type, &origin_type_size);
    MPI_Type_size(result_type, &result_type_size);

    if (origin_count == 0 || origin_type_size == 0) {
        ret = MPI_Put(NULL, origin_count, origin_type, 1, 0, target_count, target_type, win);
        if (ret)
            errs++;

        ret = MPI_Get(NULL, origin_count, origin_type, 1, 0, target_count, target_type, win);
        if (ret)
            errs++;

        ret = MPI_Accumulate(NULL, origin_count, origin_type, 1, 0, target_count, target_type,
                             MPI_SUM, win);
        if (ret)
            errs++;

        ret = MPI_Get_accumulate(NULL, origin_count, origin_type, result_buf, result_count,
                                 result_type, 1, 0, target_count, target_type, MPI_SUM, win);
        if (ret)
            errs++;

        if (result_count == 0 || result_type_size == 0) {
            ret = MPI_Get_accumulate(NULL, origin_count, origin_type, NULL, result_count,
                                     result_type, 1, 0, target_count, target_type, MPI_SUM, win);
            if (ret)
                errs++;
        }
    }

    return errs;
}

int main(int argc, char *argv[])
{
    int rank, nprocs, i, j, k;
    int errs = 0;
    MPI_Datatype types[4];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* types[0] is of zero size.  Everything else is non-zero size */
    MPI_Type_contiguous(0, MPI_INT, &types[0]);
    MPI_Type_commit(&types[0]);

    MPI_Type_contiguous(1, MPI_INT, &types[1]);
    MPI_Type_commit(&types[1]);

    types[2] = MPI_INT;
    types[3] = MPI_DOUBLE;

    MPI_Win_create(win_buf, SIZE * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_fence(0, win);

    if (rank == 0) {
        /* zero-count */
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                for (k = 0; k < 4; k++)
                    do_test(0, types[i], 0, types[j], 0, types[k]);

        /* single zero-size datatype, but non-zero count */
        for (i = 1; i < 4; i++) {
            for (j = 1; j < 4; j++) {
                do_test(1, types[0], 0, types[i], 0, types[j]);
                do_test(0, types[i], 1, types[0], 0, types[j]);
                do_test(0, types[i], 0, types[j], 1, types[0]);
            }
        }

        /* two zero-size datatypes, but non-zero count */
        for (i = 1; i < 4; i++) {
            do_test(1, types[0], 1, types[0], 0, types[i]);
            do_test(1, types[0], 0, types[i], 1, types[0]);
            do_test(0, types[i], 1, types[0], 1, types[0]);

            do_test(1, types[0], 2, types[0], 0, types[i]);
            do_test(2, types[0], 1, types[0], 0, types[i]);

            do_test(1, types[0], 0, types[i], 2, types[0]);
            do_test(2, types[0], 0, types[i], 1, types[0]);

            do_test(0, types[i], 1, types[0], 2, types[0]);
            do_test(0, types[i], 2, types[0], 1, types[0]);
        }

        /* three zero-size datatypes, but non-zero count */
        do_test(1, types[0], 1, types[0], 1, types[0]);
        do_test(1, types[0], 1, types[0], 2, types[0]);
        do_test(1, types[0], 2, types[0], 1, types[0]);
        do_test(1, types[0], 2, types[0], 2, types[0]);
        do_test(2, types[0], 1, types[0], 1, types[0]);
        do_test(2, types[0], 1, types[0], 2, types[0]);
        do_test(2, types[0], 2, types[0], 1, types[0]);
    }
    MPI_Win_fence(0, win);

    MPI_Win_free(&win);
    MPI_Type_free(&types[0]);
    MPI_Type_free(&types[1]);

    if (!errs && !rank)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
