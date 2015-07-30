/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

int AllocateGrid(int nx, int ny, int **srcArray, int **destArray);
int PackUnpack(MPI_Datatype, const int[], int[], int);

int main(int argc, char *argv[])
{
    int errs = 0;
    int wrank, wsize;
    int gsizes[3], distribs[3], dargs[3], psizes[3];
    int px, py, nx, ny, rx, ry, bx, by;
    int *srcArray, *destArray;
    int i, j, ii, jj, loc;
    MPI_Datatype darraytype;

    MTest_Init(0, 0);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    /* Test 1: Simple, 1-D cyclic decomposition */
    if (AllocateGrid(1, 3 * wsize, &srcArray, &destArray)) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Simple cyclic with 1-dim global array */
    gsizes[0] = 3 * wsize;
    distribs[0] = MPI_DISTRIBUTE_CYCLIC;
    dargs[0] = 1;
    psizes[0] = wsize;
    MPI_Type_create_darray(wsize, wrank, 1,
                           gsizes, distribs, dargs, psizes, MPI_ORDER_C, MPI_INT, &darraytype);

    /* Check the created datatype.  Because cyclic, should represent
     * a strided type */
    if (PackUnpack(darraytype, srcArray, destArray, 3)) {
        fprintf(stderr, "Error in pack/unpack check\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    /* Now, check for correct data */
    for (i = 0; i < 3; i++) {
        if (destArray[i] != wrank + i * wsize) {
            fprintf(stderr, "1D: %d: Expected %d but saw %d\n", i, wrank + i * wsize, destArray[i]);
            errs++;
        }
    }

    free(destArray);
    free(srcArray);
    MPI_Type_free(&darraytype);

    /* Test 2: Simple, 1-D cyclic decomposition, with block size=2 */
    if (AllocateGrid(1, 4 * wsize, &srcArray, &destArray)) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Simple cyclic with 1-dim global array */
    gsizes[0] = 4 * wsize;
    distribs[0] = MPI_DISTRIBUTE_CYCLIC;
    dargs[0] = 2;
    psizes[0] = wsize;
    MPI_Type_create_darray(wsize, wrank, 1,
                           gsizes, distribs, dargs, psizes, MPI_ORDER_C, MPI_INT, &darraytype);

    /* Check the created datatype.  Because cyclic, should represent
     * a strided type */
    if (PackUnpack(darraytype, srcArray, destArray, 4)) {
        fprintf(stderr, "Error in pack/unpack check\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    loc = 0;
    /* for each cyclic element */
    for (i = 0; i < 2; i++) {
        /* For each element in block */
        for (j = 0; j < 2; j++) {
            if (destArray[loc] != 2 * wrank + i * 2 * wsize + j) {
                fprintf(stderr, "1D(2): %d: Expected %d but saw %d\n",
                        i, 2 * wrank + i * 2 * wsize + j, destArray[loc]);
                errs++;
            }
            loc++;
        }
    }

    free(destArray);
    free(srcArray);
    MPI_Type_free(&darraytype);

    /* 2D: Create some 2-D decompositions */
    px = wsize / 2;
    py = 2;
    rx = wrank % px;
    ry = wrank / px;

    if (px * py != wsize) {
        fprintf(stderr, "An even number of processes is required\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Cyclic/Cyclic */
    if (AllocateGrid(5 * px, 7 * py, &srcArray, &destArray)) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Simple cyclic/cyclic. Note in C order, the [1] index varies most
     * rapidly */
    gsizes[0] = ny = 7 * py;
    gsizes[1] = nx = 5 * px;
    distribs[0] = MPI_DISTRIBUTE_CYCLIC;
    distribs[1] = MPI_DISTRIBUTE_CYCLIC;
    dargs[0] = 1;
    dargs[1] = 1;
    psizes[0] = py;
    psizes[1] = px;
    MPI_Type_create_darray(wsize, wrank, 2,
                           gsizes, distribs, dargs, psizes, MPI_ORDER_C, MPI_INT, &darraytype);

    /* Check the created datatype.  Because cyclic, should represent
     * a strided type */
    if (PackUnpack(darraytype, srcArray, destArray, 5 * 7)) {
        fprintf(stderr, "Error in pack/unpack check\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    loc = 0;
    for (j = 0; j < 7; j++) {
        for (i = 0; i < 5; i++) {
            int expected = rx + ry * nx + i * px + j * nx * py;
            if (destArray[loc] != expected) {
                errs++;
                fprintf(stderr, "2D(cc): [%d,%d] = %d, expected %d\n",
                        i, j, destArray[loc], expected);
            }
            loc++;
        }
    }

    free(srcArray);
    free(destArray);
    MPI_Type_free(&darraytype);

    /* Cyclic(2)/Cyclic(3) */
    if (AllocateGrid(6 * px, 4 * py, &srcArray, &destArray)) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* Block cyclic/cyclic. Note in C order, the [1] index varies most
     * rapidly */
    gsizes[0] = ny = 4 * py;
    gsizes[1] = nx = 6 * px;
    distribs[0] = MPI_DISTRIBUTE_CYCLIC;
    distribs[1] = MPI_DISTRIBUTE_CYCLIC;
    dargs[0] = by = 2;
    dargs[1] = bx = 3;
    psizes[0] = py;
    psizes[1] = px;
    MPI_Type_create_darray(wsize, wrank, 2,
                           gsizes, distribs, dargs, psizes, MPI_ORDER_C, MPI_INT, &darraytype);

    /* Check the created datatype.  Because cyclic, should represent
     * a strided type */
    if (PackUnpack(darraytype, srcArray, destArray, 4 * 6)) {
        fprintf(stderr, "Error in pack/unpack check\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    loc = 0;
    for (j = 0; j < 4 / by; j++) {
        for (jj = 0; jj < by; jj++) {
            for (i = 0; i < 6 / bx; i++) {
                for (ii = 0; ii < bx; ii++) {
                    int expected = rx * bx + ry * by * nx + i * bx * px + ii +
                        (j * by * py + jj) * nx;
                    if (destArray[loc] != expected) {
                        errs++;
                        fprintf(stderr, "2D(c(2)c(3)): [%d,%d] = %d, expected %d\n",
                                i * bx + ii, j * by + jj, destArray[loc], expected);
                    }
                    loc++;
                }
            }
        }
    }

    free(srcArray);
    free(destArray);
    MPI_Type_free(&darraytype);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}

int AllocateGrid(int nx, int ny, int **srcArray, int **destArray)
{
    int *src, *dest;
    int i, j;
    src = (int *) malloc(nx * ny * sizeof(int));
    dest = (int *) malloc(nx * ny * sizeof(int));
    if (!src || !dest) {
        fprintf(stderr, "Unable to allocate test arrays of size (%d x %d)\n", nx, ny);
        return 1;
    }
    for (i = 0; i < nx * ny; i++) {
        src[i] = i;
        dest[i] = -i - 1;
    }
    *srcArray = src;
    *destArray = dest;
    return 0;
}

/* Extract the source array into the dest array using the DARRAY datatype.
   "count" integers are returned in destArray */
int PackUnpack(MPI_Datatype darraytype, const int srcArray[], int destArray[], int count)
{
    int packsize, position;
    int *packArray;

    MPI_Type_commit(&darraytype);
    MPI_Pack_size(1, darraytype, MPI_COMM_SELF, &packsize);
    packArray = (int *) malloc(packsize);
    if (!packArray) {
        fprintf(stderr, "Unable to allocate pack array of size %d\n", packsize);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    position = 0;
    MPI_Pack((int *) srcArray, 1, darraytype, packArray, packsize, &position, MPI_COMM_SELF);
    packsize = position;
    position = 0;
    MPI_Unpack(packArray, packsize, &position, destArray, count, MPI_INT, MPI_COMM_SELF);
    free(packArray);
    return 0;
}
