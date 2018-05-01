/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2013 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpi.h>
#include <assert.h>
#include <stdio.h>
#include "mpitest.h"

int test_count(MPI_Count count)
{
    MPI_Status stat;
    int cancelled, cancelled2;
    MPI_Count bcount, bcount2;
    int nerrs = 0;

    bcount = count;
    cancelled = 0;
    MPI_Status_set_cancelled(&stat, cancelled);
    MPI_Status_set_elements_x(&stat, MPI_BYTE, bcount);
    MPI_Get_elements_x(&stat, MPI_BYTE, &bcount2);
    MPI_Test_cancelled(&stat, &cancelled2);
    if (bcount != bcount2) {
        fprintf(stderr, "Count Error: expected %llx, got %llx\n",
                (long long int) bcount, (long long int) bcount2);
        nerrs++;
    }
    if (cancelled != cancelled2) {
        fprintf(stderr, "Cancelled Error: expected %d, got %d\n", cancelled, cancelled2);
        nerrs++;
    }
    return nerrs;
}

int main(int argc, char **argv)
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    /* baseline: this tiny value should pose no problems */
    errs += test_count(60);
    /* one with no next-to-high-bits set */
    errs += test_count(0x3654321f71234567);
    /* masking after shift can help the count_high, but count_low is still
     * wrong */
    errs += test_count(0x7654321f71234567);
    /* original problematic count reported by Artem Yalozo */
    errs += test_count(0x7654321ff1234567);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
