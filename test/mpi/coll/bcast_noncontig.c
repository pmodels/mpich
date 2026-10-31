/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_bcast_noncontig
int run(const char *arg);
#endif

/* Test MPI_Bcast with a non-contiguous datatype (every other int), from the first and the last
 * rank, with sizes that fit in one chunk and that need several. Algorithms that pack the data,
 * such as circ_graph, run their pack path. The gaps must stay untouched. The nonblocking
 * algorithms run through MPIR_CVAR_BCAST_INTRA_ALGORITHM=nb. */

#define NUM_SIZES 3

static int check_buf(int *buf, int nelem, int seed, int rank, int root);

int run(const char *arg)
{
    int errs = 0;
    int rank, size;
    int sizes[NUM_SIZES] = { 64, 1000, 64 * 1024 };

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int *buf = malloc(2 * sizes[NUM_SIZES - 1] * sizeof(int));

    int roots[2] = { 0, size - 1 };
    int seed = 0;
    for (int n = 0; n < NUM_SIZES; n++) {
        int nelem = sizes[n];
        MPI_Datatype vec;
        MPI_Type_vector(nelem, 1, 2, MPI_INT, &vec);
        MPI_Type_commit(&vec);

        for (int r = 0; r < 2; r++) {
            int root = roots[r];
            seed++;
            for (int i = 0; i < 2 * nelem; i++) {
                if (rank == root && i % 2 == 0) {
                    buf[i] = seed * 1000000 + i;
                } else {
                    buf[i] = -seed;
                }
            }

            MPI_Bcast(buf, 1, vec, root, MPI_COMM_WORLD);

            errs += check_buf(buf, nelem, seed, rank, root);
        }

        MPI_Type_free(&vec);
    }

    free(buf);
    return errs;
}

static int check_buf(int *buf, int nelem, int seed, int rank, int root)
{
    int errs = 0;
    for (int i = 0; i < 2 * nelem; i++) {
        int expected = (i % 2 == 0) ? seed * 1000000 + i : -seed;
        if (buf[i] != expected) {
            errs++;
            if (errs < 10) {
                printf("rank %d, root %d, nelem %d: buf[%d] = %d, expected %d\n",
                       rank, root, nelem, i, buf[i], expected);
            }
        }
    }
    return errs;
}
