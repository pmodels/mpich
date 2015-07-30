/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* test based on a bug report from Lisandro Dalcin:
 * http://lists.mcs.anl.gov/pipermail/mpich-dev/2012-October/000978.html */

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
/* USE_STRICT_MPI may be defined in mpitestconf.h */
#include "mpitestconf.h"

/* assert-like macro that bumps the err count and emits a message */
#define check(x_)                                                                 \
    do {                                                                          \
        if (!(x_)) {                                                              \
            ++errs;                                                               \
            if (errs < 10) {                                                      \
                fprintf(stderr, "check failed: (%s), line %d\n", #x_, __LINE__); \
            }                                                                     \
        }                                                                         \
    } while (0)

int main(int argc, char **argv)
{
    int errs = 0;
    int rank;
    MPI_Datatype t;
    int count = 4;
    int blocklength = 2;
    MPI_Aint displacements[] = { 0, 8, 16, 24 };

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (!rank) {
        MPI_Type_create_hindexed_block(count, blocklength, displacements, MPI_INT, &t);
        MPI_Type_commit(&t);
        {
            int ni, na, nd, combiner;
            int i[1024];
            MPI_Aint a[1024];
            MPI_Datatype d[1024];
            int k;
            MPI_Type_get_envelope(t, &ni, &na, &nd, &combiner);
            MPI_Type_get_contents(t, ni, na, nd, i, a, d);

            check(ni == 2);
            check(i[0] == 4);
            check(i[1] == 2);

            check(na == 4);
            for (k = 0; k < na; k++)
                check(a[k] == (k * 8));

            check(nd == 1);
            check(d[0] == MPI_INT);
        }

        MPI_Type_free(&t);
    }

    if (rank == 0) {
        if (errs) {
            printf("found %d errors\n", errs);
        }
        else {
            printf(" No errors\n");
        }
    }
    MPI_Finalize();
    return 0;
}
