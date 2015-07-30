/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Tests that basic optimizations are performed on indexed datatypes.
 *
 * If PACK_IS_NATIVE is defined, MPI_Pack stores exactly the same bytes as the
 * user would pack manually; in that case, there is a consistency check.
 */

#ifdef MPICH
/* MPICH (as of 6/2012) packs the native bytes */
#define PACK_IS_NATIVE
#endif

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verbose = 0;

int main(int argc, char **argv)
{
    double *inbuf, *outbuf, *outbuf2;
    MPI_Aint lb, extent;
    int *index_displacement;
    int icount, errs = 0;
    int i, packsize, position, inbufsize;
    MPI_Datatype itype1, stype1;
    double t0, t1;
    double tpack, tspack, tmanual;
    int ntry;

    MPI_Init(&argc, &argv);

    icount = 2014;

    /* Create a simple block indexed datatype */
    index_displacement = (int *) malloc(icount * sizeof(int));
    if (!index_displacement) {
        fprintf(stderr, "Unable to allocated index array of size %d\n", icount);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (i = 0; i < icount; i++) {
        index_displacement[i] = (i * 3 + (i % 3));
    }

    MPI_Type_create_indexed_block(icount, 1, index_displacement, MPI_DOUBLE, &itype1);
    MPI_Type_commit(&itype1);

#if defined(MPICH) && defined(PRINT_DATATYPE_INTERNALS)
    /* To use MPIDU_Datatype_debug to print the datatype internals,
     * you must configure MPICH with --enable-g=log */
    if (verbose) {
        printf("Block index datatype:\n");
        MPIDU_Datatype_debug(itype1, 10);
    }
#endif
    MPI_Type_get_extent(itype1, &lb, &extent);

    MPI_Pack_size(1, itype1, MPI_COMM_WORLD, &packsize);

    inbufsize = extent / sizeof(double);

    inbuf = (double *) malloc(extent);
    outbuf = (double *) malloc(packsize);
    outbuf2 = (double *) malloc(icount * sizeof(double));
    if (!inbuf) {
        fprintf(stderr, "Unable to allocate %ld for inbuf\n", (long) extent);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (!outbuf) {
        fprintf(stderr, "Unable to allocate %ld for outbuf\n", (long) packsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (!outbuf2) {
        fprintf(stderr, "Unable to allocate %ld for outbuf2\n", (long) packsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i < inbufsize; i++) {
        inbuf[i] = (double) i;
    }
    position = 0;
    /* Warm up the code and data */
    MPI_Pack(inbuf, 1, itype1, outbuf, packsize, &position, MPI_COMM_WORLD);

    tpack = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        position = 0;
        t0 = MPI_Wtime();
        MPI_Pack(inbuf, 1, itype1, outbuf, packsize, &position, MPI_COMM_WORLD);
        t1 = MPI_Wtime() - t0;
        if (t1 < tpack)
            tpack = t1;
    }

    {
        int one = 1;
        MPI_Aint displ = (MPI_Aint) inbuf;
        MPI_Type_create_struct(1, &one, &displ, &itype1, &stype1);
        MPI_Type_commit(&stype1);
    }

    position = 0;
    /* Warm up the code and data */
    MPI_Pack(MPI_BOTTOM, 1, stype1, outbuf, packsize, &position, MPI_COMM_WORLD);

    tspack = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        position = 0;
        t0 = MPI_Wtime();
        MPI_Pack(MPI_BOTTOM, 1, stype1, outbuf, packsize, &position, MPI_COMM_WORLD);
        t1 = MPI_Wtime() - t0;
        if (t1 < tspack)
            tspack = t1;
    }

    /*
     * Simple manual pack (without explicitly unrolling the index block)
     */
    tmanual = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        const double *ppe = (const double *) inbuf;
        const int *id = (const int *) index_displacement;
        int k, j;
        t0 = MPI_Wtime();
        position = 0;
        for (i = 0; i < icount; i++) {
            outbuf2[position++] = ppe[id[i]];
        }
        t1 = MPI_Wtime() - t0;
        if (t1 < tmanual)
            tmanual = t1;
        /* Check on correctness */
#ifdef PACK_IS_NATIVE
        if (memcmp(outbuf, outbuf2, position) != 0) {
            printf("Panic - pack buffers differ\n");
        }
#endif
    }

    if (verbose) {
        printf("Bytes packed = %d\n", position);
        printf("MPI_Pack time = %e, manual pack time = %e\n", tpack, tmanual);
        printf("Pack with struct = %e\n", tspack);
    }

    /* The threshold here permits the MPI datatype to perform at up to
     * only one half the performance of simple user code.  Note that the
     * example code above may be made faster through careful use of const,
     * restrict, and unrolling if the compiler doesn't already do that. */
    if (2 * tmanual < tpack) {
        errs++;
        printf("MPI_Pack (block index) time = %e, manual pack time = %e\n", tpack, tmanual);
        printf("MPI_Pack time should be less than 2 times the manual time\n");
        printf("For most informative results, be sure to compile this test with optimization\n");
    }
    if (2 * tmanual < tspack) {
        errs++;
        printf("MPI_Pack (struct of block index)) time = %e, manual pack time = %e\n", tspack,
               tmanual);
        printf("MPI_Pack time should be less than 2 times the manual time\n");
        printf("For most informative results, be sure to compile this test with optimization\n");
    }
    if (errs) {
        printf(" Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }

    MPI_Type_free(&itype1);
    MPI_Type_free(&stype1);

    free(inbuf);
    free(outbuf);
    free(outbuf2);
    free(index_displacement);

    MPI_Finalize();
    return 0;
}
