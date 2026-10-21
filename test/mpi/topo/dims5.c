/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run topo_dims5
int run(const char *arg);
#endif

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
#include "baddims.h"

/* Forward refs */
static void zeroDims(int, int[]);
static int checkDims(DimsTestVal_t *, const int[]);

int run(const char *arg)
{
    int i, k, wrank, errs = 0;
    int dims[MAX_DIMS];

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

    return errs;
}

static void zeroDims(int dim, int dims[])
{
    int k;
    for (k = 0; k < dim; k++)
        dims[k] = 0;
}

static int checkDims(DimsTestVal_t * test, const int dims[])
{
    int k, errs = 0;

    for (k = 0; k < test->dim; k++) {
        if (dims[k] != test->orderedDecomp[k]) {
            errs++;
        }
    }
    return errs;
}
