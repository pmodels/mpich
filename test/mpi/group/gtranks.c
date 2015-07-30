/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#define MAX_WORLD_SIZE 1024

int main(int argc, char *argv[])
{
    int errs = 0;
    int ranks[MAX_WORLD_SIZE], ranksout[MAX_WORLD_SIZE], ranksin[MAX_WORLD_SIZE];
    int range[1][3];
    MPI_Group gworld, gself, ngroup, galt;
    MPI_Comm comm;
    int rank, size, i, nelms;

    MTest_Init(&argc, &argv);

    MPI_Comm_group(MPI_COMM_SELF, &gself);

    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    if (size > MAX_WORLD_SIZE) {
        fprintf(stderr,
                "This test requires a comm world with no more than %d processes\n", MAX_WORLD_SIZE);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (size < 4) {
        fprintf(stderr, "This test requiers at least 4 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_group(comm, &gworld);
    for (i = 0; i < size; i++) {
        ranks[i] = i;
        ranksout[i] = -1;
    }
    /* Try translating ranks from comm world compared against
     * comm self, so most will be UNDEFINED */
    MPI_Group_translate_ranks(gworld, size, ranks, gself, ranksout);

    for (i = 0; i < size; i++) {
        if (i == rank) {
            if (ranksout[i] != 0) {
                printf("[%d] Rank %d is %d but should be 0\n", rank, i, ranksout[i]);
                errs++;
            }
        }
        else {
            if (ranksout[i] != MPI_UNDEFINED) {
                printf("[%d] Rank %d is %d but should be undefined\n", rank, i, ranksout[i]);
                errs++;
            }
        }
    }

    /* MPI-2 Errata requires that MPI_PROC_NULL is mapped to MPI_PROC_NULL */
    ranks[0] = MPI_PROC_NULL;
    ranks[1] = 1;
    ranks[2] = rank;
    ranks[3] = MPI_PROC_NULL;
    for (i = 0; i < 4; i++)
        ranksout[i] = -1;

    MPI_Group_translate_ranks(gworld, 4, ranks, gself, ranksout);
    if (ranksout[0] != MPI_PROC_NULL) {
        printf("[%d] Rank[0] should be MPI_PROC_NULL but is %d\n", rank, ranksout[0]);
        errs++;
    }
    if (rank != 1 && ranksout[1] != MPI_UNDEFINED) {
        printf("[%d] Rank[1] should be MPI_UNDEFINED but is %d\n", rank, ranksout[1]);
        errs++;
    }
    if (rank == 1 && ranksout[1] != 0) {
        printf("[%d] Rank[1] should be 0 but is %d\n", rank, ranksout[1]);
        errs++;
    }
    if (ranksout[2] != 0) {
        printf("[%d] Rank[2] should be 0 but is %d\n", rank, ranksout[2]);
        errs++;
    }
    if (ranksout[3] != MPI_PROC_NULL) {
        printf("[%d] Rank[3] should be MPI_PROC_NULL but is %d\n", rank, ranksout[3]);
        errs++;
    }

    MPI_Group_free(&gself);

    /* Now, try comparing small groups against larger groups, and use groups
     * with irregular members (to bypass optimizations in group_translate_ranks
     * for simple groups)
     */
    nelms = 0;
    ranks[nelms++] = size - 2;
    ranks[nelms++] = 0;
    if (rank != 0 && rank != size - 2) {
        ranks[nelms++] = rank;
    }

    MPI_Group_incl(gworld, nelms, ranks, &ngroup);

    for (i = 0; i < nelms; i++)
        ranksout[i] = -1;
    ranksin[0] = 1;
    ranksin[1] = 0;
    ranksin[2] = MPI_PROC_NULL;
    ranksin[3] = 2;
    MPI_Group_translate_ranks(ngroup, nelms + 1, ranksin, gworld, ranksout);
    for (i = 0; i < nelms + 1; i++) {
        if (ranksin[i] == MPI_PROC_NULL) {
            if (ranksout[i] != MPI_PROC_NULL) {
                fprintf(stderr, "Input rank for proc_null but output was %d\n", ranksout[i]);
                errs++;
            }
        }
        else if (ranksout[i] != ranks[ranksin[i]]) {
            fprintf(stderr, "Expected ranksout[%d] = %d but found %d\n",
                    i, ranks[ranksin[i]], ranksout[i]);
            errs++;
        }
    }

    range[0][0] = size - 1;
    range[0][1] = 0;
    range[0][2] = -1;
    MPI_Group_range_incl(gworld, 1, range, &galt);
    for (i = 0; i < nelms + 1; i++)
        ranksout[i] = -1;
    MPI_Group_translate_ranks(ngroup, nelms + 1, ranksin, galt, ranksout);
    for (i = 0; i < nelms + 1; i++) {
        if (ranksin[i] == MPI_PROC_NULL) {
            if (ranksout[i] != MPI_PROC_NULL) {
                fprintf(stderr, "Input rank for proc_null but output was %d\n", ranksout[i]);
                errs++;
            }
        }
        else if (ranksout[i] != (size - 1) - ranks[ranksin[i]]) {
            fprintf(stderr, "Expected ranksout[%d] = %d but found %d\n",
                    i, (size - 1) - ranks[ranksin[i]], ranksout[i]);
            errs++;
        }
    }


    MPI_Group_free(&gworld);
    MPI_Group_free(&galt);
    MPI_Group_free(&ngroup);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
