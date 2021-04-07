/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mpitest.h"

/* Basic test to test MPI 4.0 large API */

static MPI_Datatype make_large_contig(MPI_Count nbytes)
{
    MPI_Datatype newtype;
    MPI_Type_contiguous_c(nbytes, MPI_CHAR, &newtype);
    MPI_Type_commit(&newtype);
    return newtype;
}

static MPI_Datatype make_large_indexed_block(MPI_Count nbytes)
{
    MPI_Datatype newtype;

#undef NBLK
#define NBLK 256
    MPI_Count num_blks = NBLK;
    MPI_Count blklen = nbytes / num_blks / sizeof(int);
    MPI_Count displs[NBLK];

    if (num_blks * blklen * sizeof(int) != nbytes) {
        return MPI_DATATYPE_NULL;
    }

    for (int i = 0; i < NBLK; i++) {
        displs[i] = i * blklen;
    }

    MPI_Type_create_indexed_block_c(NBLK, blklen, displs, MPI_INT, &newtype);
    MPI_Type_commit(&newtype);

    return newtype;
}

static MPI_Datatype make_large_hindexed(MPI_Count nbytes)
{
    MPI_Datatype newtype;

#undef NBLK
#define NBLK 4
    MPI_Count blkls[NBLK];
    MPI_Count displs[NBLK];

    MPI_Count pos = 0;
    for (int i = 0; i < NBLK - 1; i++) {
        blkls[i] = i;
        displs[i] = pos;
        pos += blkls[i];
    }
    blkls[NBLK - 1] = nbytes - pos;
    displs[NBLK - 1] = pos;

    MPI_Type_create_hindexed_c(NBLK, blkls, displs, MPI_CHAR, &newtype);
    MPI_Type_commit(&newtype);

    return newtype;
}


static int testtype(MPI_Datatype type, MPI_Count expected)
{
    MPI_Count size, lb, extent;
    int errs = 0;
    MPI_Type_size_c(type, &size);

    if (size < 0) {
        printf("ERROR: type size apparently overflowed integer\n");
        errs++;
    }

    if (size != expected) {
        printf("reported type size %lld does not match expected %lld\n", (long long) size,
               (long long) expected);
        errs++;
    }

    MPI_Type_get_true_extent_c(type, &lb, &extent);
    if (lb != 0) {
        printf("ERROR: type should have lb of 0, reported %lld\n", (long long) lb);
        errs++;
    }

    if (extent != size) {
        printf("ERROR: extent should match size, not %lld\n", (long long) extent);
        errs++;
    }
    return errs;
}


int main(int argc, char **argv)
{

    int errs = 0;
    int rank, size;
    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

#define NR_TYPES 3
    MPI_Offset expected_sizes[NR_TYPES] = { 1024UL * 1024UL * 2400UL * 2,
        2346319872 * 3,
        2346319872 * 4
    };
    MPI_Datatype types[NR_TYPES];

    if (sizeof(MPI_Aint) == 4) {
        /* 32-bit system can't work with sizes more than address int */
        for (int i = 0; i < NR_TYPES; i++) {
            expected_sizes[i] /= 8;
        }
    }

    types[0] = make_large_contig(expected_sizes[0]);
    types[1] = make_large_indexed_block(expected_sizes[1]);
    types[2] = make_large_hindexed(expected_sizes[2]);

    for (int i = 0; i < NR_TYPES; i++) {
        if (types[i] != MPI_DATATYPE_NULL) {
            errs += testtype(types[i], expected_sizes[i]);
            MPI_Type_free(&(types[i]));
        }
    }

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
