/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test ensures that MPI_Comm_split breaks ties in key values by using the
 * original rank in the input communicator.  This typically corresponds to
 * the difference between using a stable sort or using an unstable sort.
 *
 * It checks all sizes from 1..comm_size(world)-1, so this test does not need to
 * be run multiple times at process counts from a higher-level test driver. */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define ERRLIMIT (10)

#define my_assert(cond_)                                     \
    do {                                                     \
        if (!(cond_)) {                                      \
            if (errs < ERRLIMIT)                             \
                printf("assertion \"%s\" failed\n", #cond_); \
            ++errs;                                          \
        }                                                    \
    } while (0)

int main(int argc, char **argv)
{
    int i, j, pos, modulus, cs, rank, size;
    int wrank, wsize;
    int newrank, newsize;
    int errs = 0;
    int key;
    int *oldranks = NULL;
    int *identity = NULL;
    int verbose = 0;
    MPI_Comm comm, splitcomm;
    MPI_Group wgroup, newgroup;

    MPI_Init(&argc, &argv);

    if (getenv("MPITEST_VERBOSE"))
        verbose = 1;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    oldranks = malloc(wsize * sizeof(int));
    identity = malloc(wsize * sizeof(int));
    for (i = 0; i < wsize; ++i) {
        identity[i] = i;
    }

    for (cs = 1; cs <= wsize; ++cs) {
        /* yes, we are using comm_split to test comm_split, but this test is
         * mainly about ensuring that the stable sort behavior is correct, not
         * about whether the partitioning by color behavior is correct */
        MPI_Comm_split(MPI_COMM_WORLD, (wrank < cs ? 0 : MPI_UNDEFINED), wrank, &comm);
        if (comm != MPI_COMM_NULL) {
            MPI_Comm_rank(comm, &rank);
            MPI_Comm_size(comm, &size);

            for (modulus = 1; modulus <= size; ++modulus) {
                /* Divide all ranks into one of "modulus" equivalence classes.  Ranks in
                 * output comm will be ordered first by class, then within the class by
                 * rank in comm world. */
                key = rank % modulus;

                /* all pass same color, variable keys */
                MPI_Comm_split(comm, 5, key, &splitcomm);
                MPI_Comm_rank(splitcomm, &newrank);
                MPI_Comm_size(splitcomm, &newsize);
                my_assert(newsize == size);

                MPI_Comm_group(MPI_COMM_WORLD, &wgroup);
                MPI_Comm_group(splitcomm, &newgroup);
                int gsize;
                MPI_Group_size(newgroup, &gsize);
                MPI_Group_translate_ranks(newgroup, size, identity, wgroup, oldranks);
                MPI_Group_free(&wgroup);
                MPI_Group_free(&newgroup);

                if (splitcomm != MPI_COMM_NULL)
                    MPI_Comm_free(&splitcomm);

                /* now check that comm_split broke any ties correctly */
                if (rank == 0) {
                    if (verbose) {
                        /* debugging code that is useful when the test fails */
                        printf("modulus=%d oldranks={", modulus);
                        for (i = 0; i < size - 1; ++i) {
                            printf("%d,", oldranks[i]);
                        }
                        printf("%d} keys={", oldranks[i]);
                        for (i = 0; i < size - 1; ++i) {
                            printf("%d,", i % modulus);
                        }
                        printf("%d}\n", i % modulus);
                    }

                    pos = 0;
                    for (i = 0; i < modulus; ++i) {
                        /* there's probably a better way to write these loop bounds and
                         * indices, but this is the first (correct) way that occurred to me */
                        for (j = 0; j < (size / modulus + (i < size % modulus ? 1 : 0)); ++j) {
                            if (errs < ERRLIMIT && oldranks[pos] != i+modulus*j) {
                                printf("size=%d i=%d j=%d modulus=%d pos=%d i+modulus*j=%d oldranks[pos]=%d\n",
                                       size, i, j, modulus, pos, i+modulus*j, oldranks[pos]);
                            }
                            my_assert(oldranks[pos] == i+modulus*j);
                            ++pos;
                        }
                    }
                }
            }
            MPI_Comm_free(&comm);
        }
    }

    if (oldranks != NULL)
        free(oldranks);
    if (identity != NULL)
        free(identity);

    if (rank == 0) {
        if (errs)
            printf("found %d errors\n", errs);
        else
            printf(" No errors\n");
    }

    MPI_Finalize();
    return 0;
}

