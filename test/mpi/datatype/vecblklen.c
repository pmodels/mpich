/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpitest.h"

/* Inspired by the Intel MPI_Type_vector_blklen test.
   Added to include a test of a dataloop optimization that failed.
*/
int main(int argc, char *argv[])
{
    MPI_Datatype ot, ot2, newtype;
    int position, psize, insize, outsize;
    signed char *inbuf = 0, *outbuf = 0, *pbuf = 0, *p;
    int i, j, k;
    int errs = 0;
    int veccount = 16, stride = 16;

    MTest_Init(&argc, &argv);
    /*
     * Create a type with some padding
     */
    MPI_Type_contiguous(59, MPI_CHAR, &ot);
    MPI_Type_create_resized(ot, 0, 64, &ot2);
    /*
     * Use a vector type with a block size equal to the stride - thus
     * tiling the target memory with copies of old type.  This is not
     * a contiguous copy since oldtype has a gap at the end.
     */
    MPI_Type_vector(veccount, stride, stride, ot2, &newtype);
    MPI_Type_commit(&newtype);

    insize = veccount * stride * 64;
    outsize = insize;
    inbuf = (char *) malloc(insize);
    outbuf = (char *) malloc(outsize);
    for (i = 0; i < outsize; i++) {
        inbuf[i] = i % 64;
        outbuf[i] = -1;
    }

    MPI_Pack_size(1, newtype, MPI_COMM_WORLD, &psize);
    pbuf = (char *) malloc(psize);

    position = 0;
    MPI_Pack(inbuf, 1, newtype, pbuf, psize, &position, MPI_COMM_WORLD);
    psize = position;
    position = 0;
    MPI_Unpack(pbuf, psize, &position, outbuf, 1, newtype, MPI_COMM_WORLD);


    /* Check the output */
    p = outbuf;
    for (i = 0; i < veccount; i++) {
        for (j = 0; j < stride; j++) {
            for (k = 0; k < 59; k++) {
                if (*p != k % 64) {
                    errs++;
                    fprintf(stderr, "[%d,%d,%d]expected %d but saw %d\n", i, j, k, (k % 64), *p);
                }
                p++;
            }
            for (k = 59; k < 64; k++) {
                if (*p != -1) {
                    errs++;
                    fprintf(stderr, "[%d,%d,%d]expected -1 but saw %d\n", i, j, k, *p);
                }
                p++;
            }
        }
    }

    free(pbuf);
    free(inbuf);
    free(outbuf);

    MPI_Type_free(&ot);
    MPI_Type_free(&ot2);
    MPI_Type_free(&newtype);
    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
