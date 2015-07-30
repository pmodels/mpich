/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

static MPI_Datatype make_largexfer_type_struct(MPI_Offset nbytes)
{
    int typechunk_size = 1024 * 1024;   /* in bytes: TODO: figure out how big a
                                         * chunk is really needed */
    int chunk_count;
    int remainder = 0;
    MPI_Datatype memtype, chunktype;

    /* need to cook up a new datatype to accomodate large datatypes */
    /* first pass: chunks of 1 MiB plus an additional remainder.  Does require
     * 8 byte MPI_Aint, which should have been checked for earlier */

    chunk_count = nbytes / typechunk_size;
    remainder = nbytes % typechunk_size;
    MPI_Type_contiguous(typechunk_size, MPI_BYTE, &chunktype);
    MPI_Type_commit(&chunktype);

    /* a zero remainder means we can just count contigs */
    if (remainder == 0) {
        MPI_Type_contiguous(chunk_count, chunktype, &memtype);
        MPI_Type_free(&chunktype);
    }
    else {
        if (sizeof(MPI_Aint) <= sizeof(int)) {
            return MPI_DATATYPE_NULL;
        }
        /* struct type: some number of chunks plus remaining bytes tacked
         * on at end */
        int lens[] = { chunk_count, remainder };
        MPI_Aint disp[] = { 0, (MPI_Aint) typechunk_size * (MPI_Aint) chunk_count };
        MPI_Datatype types[] = { chunktype, MPI_BYTE };

        MPI_Type_struct(2, lens, disp, types, &memtype);
        MPI_Type_free(&chunktype);
    }
    MPI_Type_commit(&memtype);
    return memtype;
}

static MPI_Datatype make_largexfer_type_hindexed(MPI_Offset nbytes)
{
    int i, count;
    int chunk_size = 1024 * 1024;
    int *blocklens;
    MPI_Aint *disp;
    MPI_Datatype memtype;

    /* need to cook up a new datatype to accomodate large datatypes */
    /* Does require 8 byte MPI_Aint, which should have been checked for earlier
     */

    if (sizeof(MPI_Aint) <= sizeof(int)) {
        return MPI_DATATYPE_NULL;
    }

    /* ceiling division */
    count = 1 + ((nbytes - 1) / chunk_size);

    blocklens = calloc(count, sizeof(int));
    disp = calloc(count, sizeof(MPI_Aint));


    for (i = 0; i < (count - 1); i++) {
        blocklens[i] = chunk_size;
        disp[i] = (MPI_Aint) chunk_size *i;
    }
    blocklens[count - 1] = nbytes - ((MPI_Aint) chunk_size * i);
    disp[count - 1] = (MPI_Aint) chunk_size *(count - 1);

    MPI_Type_create_hindexed(count, blocklens, disp, MPI_BYTE, &memtype);
    MPI_Type_commit(&memtype);

    return memtype;
}


int testtype(MPI_Datatype type, MPI_Offset expected)
{
    MPI_Count size, lb, extent;
    int nerrors = 0;
    MPI_Type_size_x(type, &size);

    if (size < 0) {
        printf("ERROR: type size apparently overflowed integer\n");
        nerrors++;
    }

    if (size != expected) {
        printf("reported type size %lld does not match expected %lld\n", size, expected);
        nerrors++;
    }

    MPI_Type_get_true_extent_x(type, &lb, &extent);
    if (lb != 0) {
        printf("ERROR: type should have lb of 0, reported %lld\n", lb);
        nerrors++;
    }

    if (extent != size) {
        printf("ERROR: extent should match size, not %lld\n", extent);
        nerrors++;
    }
    return nerrors;
}


int main(int argc, char **argv)
{

    int nerrors = 0, i;
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

#define NR_TYPES 3
    MPI_Offset expected_sizes[NR_TYPES] = { 1024UL * 1024UL * 2400UL,
        2346319872,
        2346319872
    };
    MPI_Datatype types[NR_TYPES];

    /* a contig type, itself large, but does not need 8 byte aints */
    types[0] = make_largexfer_type_struct(expected_sizes[0]);
    /* struct with addresses out past 2 GiB */
    types[1] = make_largexfer_type_struct(expected_sizes[1]);
    /* similar, but with hindexed type */
    types[2] = make_largexfer_type_hindexed(expected_sizes[2]);

    for (i = 0; i < NR_TYPES; i++) {
        if (types[i] != MPI_DATATYPE_NULL) {
            nerrors += testtype(types[i], expected_sizes[i]);
            MPI_Type_free(&(types[i]));
        }
    }

    MPI_Finalize();
    if (rank == 0) {
        if (nerrors) {
            printf("found %d errors\n", nerrors);
        }
        else {
            printf(" No errors\n");
        }
    }

    return 0;
}
