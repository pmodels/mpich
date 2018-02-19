/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
 * Test MPI_Dims_create and the choice of decompositions.  These should match
 * the definition in MPI, which wants a "balanced" decomposition.  There
 * is some ambiguity in the definition, so this test attempts to deal with
 * that.
 */
#define MAX_DIMS 20

typedef struct {
    int size, dim;
    int orderedDecomp[MAX_DIMS];
} DimsTestVal_t;

/* MPI 3.1, page 293, line 31, output values of Dims_create are in
   non-increasing order */
DimsTestVal_t tests[] = {
#include "baddims.h"
};

/* Forward refs */
void zeroDims(int, int[]);
int checkDims(DimsTestVal_t *, const int[]);
int compareDims(int, const int[], const int[], int);

int main(int argc, char *argv[])
{
    int i, k, wrank, errs = 0;
    int dims[MAX_DIMS];

    MTest_Init(0, 0);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    if (wrank == 0) {

        for (k = 0; tests[k].size > 0; k++) {
            zeroDims(tests[k].dim, dims);
            MPI_Dims_create(tests[k].size, tests[k].dim, dims);
            if (checkDims(&tests[k], dims)) {
                errs++;
                MTestPrintfMsg(1, "Test %d failed with mismatched output", k);
                if (errs < 10) {
                    fprintf(stderr, "%d in %dd: ", tests[k].size, tests[k].dim);
                    for (i = 0; i < tests[k].dim - 1; i++)
                        fprintf(stderr, "%d x ", dims[i]);
                    fprintf(stderr, "%d != %d", dims[tests[k].dim - 1], tests[k].orderedDecomp[0]);
                    for (i = 1; i < tests[k].dim; i++)
                        fprintf(stderr, " x %d", tests[k].orderedDecomp[i]);
                    fprintf(stderr, "\n");
                }
            }
        }
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}

void zeroDims(int dim, int dims[])
{
    int k;
    for (k = 0; k < dim; k++)
        dims[k] = 0;
}

int checkDims(DimsTestVal_t * test, const int dims[])
{
    int k, errs = 0;

    for (k = 0; k < test->dim; k++) {
        if (dims[k] != test->orderedDecomp[k]) {
            errs++;
        }
    }
    return errs;
}

int compareDims(int dim, const int d1[], const int d2[], int isweak)
{
    int diff = 0, k;
    if (isweak) {
        diff = d1[0] - d1[dim - 1] - (d2[0] - d2[dim - 1]);
        if (diff < 0)
            diff = -diff;
    } else {
        for (k = 0; k < dim; k++) {
            int d = d1[k] - d2[k];
            if (d < 0)
                d = -d;
            diff += d;
        }
    }
    return diff;
}
