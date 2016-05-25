/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Tests that the performance of a struct that contains a vector type
 * exploits the vector type correctly
 *
 * If PACK_IS_NATIVE is defined, MPI_Pack stores exactly the same bytes as the
 * user would pack manually; in that case, there is a consistency check.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpitestconf.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef MPICH
/* MPICH (as of 6/2012) packs the native bytes */
#define PACK_IS_NATIVE
#endif


static int verbose = 0;

int main(int argc, char **argv)
{
    int vcount, vstride;
    int32_t counts[2];
    int v2stride, typesize, packsize, i, position, errs = 0;
    double *outbuf, *outbuf2;
    double *vsource;
    MPI_Datatype vtype, stype;
    MPI_Aint lb, extent;
    double t0, t1;
    double tspack, tvpack, tmanual;
    int ntry;
    int blocklengths[2];
    MPI_Aint displacements[2];
    MPI_Datatype typesArray[2];

    MPI_Init(&argc, &argv);

    /* Create a struct consisting of a two 32-bit ints, followed by a
     * vector of stride 3 but count 128k (less than a few MB of data area) */
    vcount = 128000;
    vstride = 3;
    MPI_Type_vector(vcount, 1, vstride, MPI_DOUBLE, &vtype);

    vsource = (double *) malloc((vcount + 1) * (vstride + 1) * sizeof(double));
    if (!vsource) {
        fprintf(stderr, "Unable to allocate vsource\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (i = 0; i < vcount * vstride; i++) {
        vsource[i] = i;
    }
    blocklengths[0] = 2;
    MPI_Get_address(&counts[0], &displacements[0]);
    blocklengths[1] = 1;
    MPI_Get_address(vsource, &displacements[1]);
    if (verbose) {
        printf("%p = %p?\n", vsource, (void *) displacements[1]);
    }
    typesArray[0] = MPI_INT32_T;
    typesArray[1] = vtype;
    MPI_Type_create_struct(2, blocklengths, displacements, typesArray, &stype);
    MPI_Type_commit(&stype);
    MPI_Type_commit(&vtype);

#if defined(MPICH) && defined(PRINT_DATATYPE_INTERNALS)
    /* To use MPIR_Datatype_debug to print the datatype internals,
     * you must configure MPICH with --enable-g=log */
    if (verbose) {
        printf("Original struct datatype:\n");
        MPIR_Datatype_debug(stype, 10);
    }
#endif

    MPI_Pack_size(1, stype, MPI_COMM_WORLD, &packsize);
    outbuf = (double *) malloc(packsize);
    outbuf2 = (double *) malloc(packsize);
    if (!outbuf) {
        fprintf(stderr, "Unable to allocate %ld for outbuf\n", (long) packsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (!outbuf2) {
        fprintf(stderr, "Unable to allocate %ld for outbuf2\n", (long) packsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    position = 0;
    /* Warm up the code and data */
    MPI_Pack(MPI_BOTTOM, 1, stype, outbuf, packsize, &position, MPI_COMM_WORLD);

    tspack = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        position = 0;
        t0 = MPI_Wtime();
        MPI_Pack(MPI_BOTTOM, 1, stype, outbuf, packsize, &position, MPI_COMM_WORLD);
        t1 = MPI_Wtime() - t0;
        if (t1 < tspack)
            tspack = t1;
    }
    MPI_Type_free(&stype);

    /* An equivalent packing, using the 2 ints and the vector separately */
    tvpack = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        position = 0;
        t0 = MPI_Wtime();
        MPI_Pack(counts, 2, MPI_INT32_T, outbuf, packsize, &position, MPI_COMM_WORLD);
        MPI_Pack(vsource, 1, vtype, outbuf, packsize, &position, MPI_COMM_WORLD);
        t1 = MPI_Wtime() - t0;
        if (t1 < tvpack)
            tvpack = t1;
    }
    MPI_Type_free(&vtype);

    /* Note that we exploit the fact that the vector type contains vblock
     * instances of a contiguous type of size 24, or a single block of 24*vblock
     * bytes.
     */
    tmanual = 1e12;
    for (ntry = 0; ntry < 5; ntry++) {
        const double *restrict ppe = (const double *) vsource;
        double *restrict ppo = outbuf2;
        int j;
        t0 = MPI_Wtime();
        position = 0;
        *(int32_t *) ppo = counts[0];
        *(((int32_t *) ppo) + 1) = counts[1];
        ppo++;
        /* Some hand optimization because this file is not normally
         * compiled with optimization by the test suite */
        j = vcount;
        while (j) {
            *ppo++ = *ppe;
            ppe += vstride;
            *ppo++ = *ppe;
            ppe += vstride;
            *ppo++ = *ppe;
            ppe += vstride;
            *ppo++ = *ppe;
            ppe += vstride;
            j -= 4;
        }
        position += (1 + vcount);
        position *= sizeof(double);
        t1 = MPI_Wtime() - t0;
        if (t1 < tmanual)
            tmanual = t1;

        /* Check on correctness */
#ifdef PACK_IS_NATIVE
        if (memcmp(outbuf, outbuf2, position) != 0) {
            printf("Panic(manual) - pack buffers differ\n");
            for (j = 0; j < 8; j++) {
                printf("%d: %llx\t%llx\n", j, (long long unsigned) outbuf[j],
                       (long long unsigned) outbuf2[j]);
            }
        }
#endif
    }

    if (verbose) {
        printf("Bytes packed = %d\n", position);
        printf("MPI_Pack time = %e (struct), = %e (vector), manual pack time = %e\n",
               tspack, tvpack, tmanual);
    }

    if (4 * tmanual < tspack) {
        errs++;
        printf("MPI_Pack time using struct with vector = %e, manual pack time = %e\n", tspack,
               tmanual);
        printf("MPI_Pack time should be less than 4 times the manual time\n");
        printf("For most informative results, be sure to compile this test with optimization\n");
    }
    if (4 * tmanual < tvpack) {
        errs++;
        printf("MPI_Pack using vector = %e, manual pack time = %e\n", tvpack, tmanual);
        printf("MPI_Pack time should be less than 4 times the manual time\n");
        printf("For most informative results, be sure to compile this test with optimization\n");
    }
    if (4 * tvpack < tspack) {
        errs++;
        printf("MPI_Pack using a vector = %e, using a struct with vector = %e\n", tvpack, tspack);
        printf
            ("MPI_Pack time using vector should be about the same as the struct containing the vector\n");
        printf("For most informative results, be sure to compile this test with optimization\n");
    }

    if (errs) {
        printf(" Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }

    free(vsource);
    free(outbuf);
    free(outbuf2);

    MPI_Finalize();
    return 0;
}
