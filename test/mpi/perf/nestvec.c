/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Tests that basic optimizations are performed on vector of vector datatypes.
 * As the "leaf" element is a large block (when properly optimized), the
 * performance of an MPI datatype should be nearly as good (if not better)
 * than manual packing (the threshold used in this test is *very* forgiving).
 * This test may be run with one process.
 *
 * If PACK_IS_NATIVE is defined, MPI_Pack stores exactly the same bytes as the
 * user would pack manually; in that case, there is a consistency check.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpitestconf.h"

#ifdef MPICH
/* MPICH (as of 6/2012) packs the native bytes */
#define PACK_IS_NATIVE
#endif


static int verbose = 0;

int main(int argc, char **argv)
{
    int vcount = 16, vblock = vcount * vcount / 2, vstride = 2 * vcount * vblock;
    int v2stride, typesize, packsize, i, position, errs = 0;
    char *inbuf, *outbuf, *outbuf2;
    MPI_Datatype ft1type, ft2type, ft3type;
    MPI_Datatype ftopttype;
    MPI_Aint lb, extent;
    double t0, t1;
    double tpack, tmanual, tpackopt;
    int ntry;

    MPI_Init(&argc, &argv);

    MPI_Type_contiguous(6, MPI_FLOAT, &ft1type);
    MPI_Type_size(ft1type, &typesize);
    v2stride = vcount * vcount * vcount * vcount * typesize;
    MPI_Type_vector(vcount, vblock, vstride, ft1type, &ft2type);
    MPI_Type_create_hvector(2, 1, v2stride, ft2type, &ft3type);
    MPI_Type_commit(&ft3type);
    MPI_Type_free(&ft1type);
    MPI_Type_free(&ft2type);
#if defined(MPICH) && defined(PRINT_DATATYPE_INTERNALS)
    /* To use MPIDU_Datatype_debug to print the datatype internals,
     * you must configure MPICH with --enable-g=log */
    if (verbose) {
        printf("Original datatype:\n");
        MPIDU_Datatype_debug(ft3type, 10);
    }
#endif
    /* The same type, but without using the contiguous type */
    MPI_Type_vector(vcount, 6 * vblock, 6 * vstride, MPI_FLOAT, &ft2type);
    MPI_Type_create_hvector(2, 1, v2stride, ft2type, &ftopttype);
    MPI_Type_commit(&ftopttype);
    MPI_Type_free(&ft2type);
#if defined(MPICH) && defined(PRINT_DATATYPE_INTERNALS)
    if (verbose) {
        printf("\n\nMerged datatype:\n");
        MPIDU_Datatype_debug(ftopttype, 10);
    }
#endif

    MPI_Type_get_extent(ft3type, &lb, &extent);
    MPI_Type_size(ft3type, &typesize);

    MPI_Pack_size(1, ft3type, MPI_COMM_WORLD, &packsize);

    inbuf = (char *) malloc(extent);
    outbuf = (char *) malloc(packsize);
    outbuf2 = (char *) malloc(packsize);
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
    for (i = 0; i < extent; i++) {
        inbuf[i] = i & 0x7f;
    }
    position = 0;
    /* Warm up the code and data */
    MPI_Pack(inbuf, 1, ft3type, outbuf, packsize, &position, MPI_COMM_WORLD);

    /* Pack using the vector of vector of contiguous */
    tpack = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        position = 0;
        t0 = MPI_Wtime();
        MPI_Pack(inbuf, 1, ft3type, outbuf, packsize, &position, MPI_COMM_WORLD);
        t1 = MPI_Wtime() - t0;
        if (t1 < tpack)
            tpack = t1;
    }
    MPI_Type_free(&ft3type);

    /* Pack using vector of vector with big blocks (same type map) */
    tpackopt = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        position = 0;
        t0 = MPI_Wtime();
        MPI_Pack(inbuf, 1, ftopttype, outbuf, packsize, &position, MPI_COMM_WORLD);
        t1 = MPI_Wtime() - t0;
        if (t1 < tpackopt)
            tpackopt = t1;
    }
    MPI_Type_free(&ftopttype);

    /* User (manual) packing code.
     * Note that we exploit the fact that the vector type contains vblock
     * instances of a contiguous type of size 24, or equivalently a
     * single block of 24*vblock bytes.
     */
    tmanual = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        const char *ppe = (const char *) inbuf;
        int k, j;
        t0 = MPI_Wtime();
        position = 0;
        for (k = 0; k < 2; k++) {       /* hvector count; blocksize is 1 */
            const char *ptr = ppe;
            for (j = 0; j < vcount; j++) {      /* vector count */
                memcpy(outbuf2 + position, ptr, 24 * vblock);
                ptr += vstride * 24;
                position += 24 * vblock;
            }
            ppe += v2stride;
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
        printf("MPI_Pack time = %e, opt version = %e, manual pack time = %e\n",
               tpack, tpackopt, tmanual);
    }

    /* A factor of 4 is extremely generous, especially since the test suite
     * no longer builds any of the tests with optimization */
    if (4 * tmanual < tpack) {
        errs++;
        printf("MPI_Pack time = %e, manual pack time = %e\n", tpack, tmanual);
        printf("MPI_Pack time should be less than 4 times the manual time\n");
        printf("For most informative results, be sure to compile this test with optimization\n");
    }
    if (4 * tmanual < tpackopt) {
        errs++;
        printf("MPI_Pack with opt = %e, manual pack time = %e\n", tpackopt, tmanual);
        printf("MPI_Pack time should be less than 4 times the manual time\n");
        printf("For most informative results, be sure to compile this test with optimization\n");
    }
    if (errs) {
        printf(" Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }

    free(inbuf);
    free(outbuf);
    free(outbuf2);

    MPI_Finalize();
    return 0;
}
