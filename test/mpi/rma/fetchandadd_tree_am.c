/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "mpitest.h"

/* This is the tree-based scalable version of the fetch-and-add
   example from Using MPI-2, pg 206-207. The code in the book (Fig
   6.16) has bugs that are fixed below. */

/* same as fetchandadd_tree.c but uses alloc_mem */

#define NTIMES 20       /* no of times each process calls the counter
                         * routine */

int localvalue = 0;             /* contribution of this process to the counter. We
                                 * define it as a global variable because attribute
                                 * caching on the window is not enabled yet. */

void Get_nextval_tree(MPI_Win win, int *get_array, MPI_Datatype get_type,
                      MPI_Datatype acc_type, int nlevels, int *value);

int compar(const void *a, const void *b);

int main(int argc, char *argv[])
{
    int rank, nprocs, i, *counter_mem, *get_array, *get_idx, *acc_idx,
        mask, nlevels, level, idx, tmp_rank, pof2;
    MPI_Datatype get_type, acc_type;
    MPI_Win win;
    int errs = 0, *results, *counter_vals;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        /* allocate counter memory and initialize to 0 */

        /* find the next power-of-two >= nprocs */
        pof2 = 1;
        while (pof2 < nprocs)
            pof2 *= 2;

        /* counter_mem = (int *) calloc(pof2*2, sizeof(int)); */

        i = MPI_Alloc_mem(pof2 * 2 * sizeof(int), MPI_INFO_NULL, &counter_mem);
        if (i) {
            printf("Can't allocate memory in test program\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (i = 0; i < (pof2 * 2); i++)
            counter_mem[i] = 0;

        MPI_Win_create(counter_mem, pof2 * 2 * sizeof(int), sizeof(int),
                       MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        MPI_Win_free(&win);

        /* free(counter_mem) */
        MPI_Free_mem(counter_mem);

        /* gather the results from other processes, sort them, and check
         * whether they represent a counter being incremented by 1 */

        results = (int *) malloc(NTIMES * nprocs * sizeof(int));
        for (i = 0; i < NTIMES * nprocs; i++)
            results[i] = -1;

        MPI_Gather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, results, NTIMES, MPI_INT, 0, MPI_COMM_WORLD);

        qsort(results + NTIMES, NTIMES * (nprocs - 1), sizeof(int), compar);

        for (i = NTIMES + 1; i < (NTIMES * nprocs); i++)
            if (results[i] != results[i - 1] + 1)
                errs++;

        free(results);
    }
    else {
        /* Get the largest power of two smaller than nprocs */
        mask = 1;
        nlevels = 0;
        while (mask < nprocs) {
            mask <<= 1;
            nlevels++;
        }
        mask >>= 1;

        get_array = (int *) malloc(nlevels * sizeof(int));
        get_idx = (int *) malloc(nlevels * sizeof(int));
        acc_idx = (int *) malloc(nlevels * sizeof(int));

        level = 0;
        idx = 0;
        tmp_rank = rank;
        while (mask >= 1) {
            if (tmp_rank < mask) {
                /* go to left for acc_idx, go to right for
                 * get_idx. set idx=acc_idx for next iteration */
                acc_idx[level] = idx + 1;
                get_idx[level] = idx + mask * 2;
                idx = idx + 1;
            }
            else {
                /* go to right for acc_idx, go to left for
                 * get_idx. set idx=acc_idx for next iteration */
                acc_idx[level] = idx + mask * 2;
                get_idx[level] = idx + 1;
                idx = idx + mask * 2;
            }
            level++;
            tmp_rank = tmp_rank % mask;
            mask >>= 1;
        }

/*        for (i=0; i<nlevels; i++)
            printf("Rank %d, acc_idx[%d]=%d, get_idx[%d]=%d\n", rank,
                   i, acc_idx[i], i, get_idx[i]);
*/

        MPI_Type_create_indexed_block(nlevels, 1, get_idx, MPI_INT, &get_type);
        MPI_Type_create_indexed_block(nlevels, 1, acc_idx, MPI_INT, &acc_type);
        MPI_Type_commit(&get_type);
        MPI_Type_commit(&acc_type);

        /* allocate array to store the values obtained from the
         * fetch-and-add counter */
        counter_vals = (int *) malloc(NTIMES * sizeof(int));

        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        for (i = 0; i < NTIMES; i++) {
            Get_nextval_tree(win, get_array, get_type, acc_type, nlevels, counter_vals + i);
            /* printf("Rank %d, counter %d\n", rank, value); */
        }

        MPI_Win_free(&win);
        free(get_array);
        free(get_idx);
        free(acc_idx);
        MPI_Type_free(&get_type);
        MPI_Type_free(&acc_type);

        /* gather the results to the root */
        MPI_Gather(counter_vals, NTIMES, MPI_INT, NULL, 0, MPI_DATATYPE_NULL, 0, MPI_COMM_WORLD);
        free(counter_vals);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return MTestReturnValue(errs);
}


void Get_nextval_tree(MPI_Win win, int *get_array, MPI_Datatype get_type,
                      MPI_Datatype acc_type, int nlevels, int *value)
{
    int *one, i;

    one = (int *) malloc(nlevels * sizeof(int));
    for (i = 0; i < nlevels; i++)
        one[i] = 1;

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
    MPI_Accumulate(one, nlevels, MPI_INT, 0, 0, 1, acc_type, MPI_SUM, win);
    MPI_Get(get_array, nlevels, MPI_INT, 0, 0, 1, get_type, win);
    MPI_Win_unlock(0, win);

    *value = localvalue;
    for (i = 0; i < nlevels; i++)
        *value = *value + get_array[i];

    localvalue++;

    free(one);
}

int compar(const void *a, const void *b)
{
    return (*((int *) a) - *((int *) b));
}
