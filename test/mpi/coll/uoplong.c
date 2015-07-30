/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Test user-defined operations with a large number of elements.
 * Added because a talk at EuroMPI'12 claimed that these failed with
 * more than 64k elements
 */

#define MAX_ERRS 10
#define MAX_COUNT 1200000

void myop(void *cinPtr, void *coutPtr, int *count, MPI_Datatype * dtype);

/*
 * myop takes a datatype that is a triple of doubles, and computes
 * the sum, max, min of the respective elements of the triple.
 */
void myop(void *cinPtr, void *coutPtr, int *count, MPI_Datatype * dtype)
{
    int i, n = *count;
    double const *cin = (double *) cinPtr;
    double *cout = (double *) coutPtr;

    for (i = 0; i < n; i++) {
        cout[0] += cin[0];
        cout[1] = (cout[1] > cin[1]) ? cout[1] : cin[1];
        cout[2] = (cout[2] < cin[2]) ? cout[2] : cin[2];
        cin += 3;
        cout += 3;
    }
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int wsize, wrank, i, count;
    MPI_Datatype tripleType;
    double *inVal, *outVal;
    double maxval, sumval;
    MPI_Op op;

    MTest_Init(&argc, &argv);
    MPI_Op_create(myop, 0, &op);
    MPI_Type_contiguous(3, MPI_DOUBLE, &tripleType);
    MPI_Type_commit(&tripleType);

    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    for (count = 1; count < MAX_COUNT; count += count) {
        if (wrank == 0)
            MTestPrintfMsg(1, "Count = %d\n", count);
        inVal = (double *) malloc(3 * count * sizeof(double));
        outVal = (double *) malloc(3 * count * sizeof(double));
        if (!inVal || !outVal) {
            fprintf(stderr, "Unable to allocated %d words for data\n", 3 * count);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        for (i = 0; i < count * 3; i++) {
            outVal[i] = -1;
            inVal[i] = 1 + (i & 0x3);
        }
        MPI_Reduce(inVal, outVal, count, tripleType, op, 0, MPI_COMM_WORLD);
        /* Check Result values */
        if (wrank == 0) {
            for (i = 0; i < 3 * count; i += 3) {
                sumval = wsize * (1 + (i & 0x3));
                maxval = 1 + ((i + 1) & 0x3);
                if (outVal[i] != sumval) {
                    if (errs < MAX_ERRS)
                        fprintf(stderr, "%d: outval[%d] = %f, expected %f (sum)\n",
                                count, i, outVal[i], sumval);
                    errs++;
                }
                if (outVal[i + 1] != maxval) {
                    if (errs < MAX_ERRS)
                        fprintf(stderr, "%d: outval[%d] = %f, expected %f (max)\n",
                                count, i + 1, outVal[i + 1], maxval);
                    errs++;
                }
                if (outVal[i + 2] != 1 + ((i + 2) & 0x3)) {
                    if (errs < MAX_ERRS)
                        fprintf(stderr, "%d: outval[%d] = %f, expected %f (min)\n",
                                count, i + 2, outVal[i + 2], (double) (1 + ((i + 2) ^ 0x3)));
                    errs++;
                }
            }
        }

        free(inVal);
        free(outVal);
    }

    MPI_Op_free(&op);
    MPI_Type_free(&tripleType);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
