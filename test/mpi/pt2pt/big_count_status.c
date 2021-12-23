/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run pt2pt_big_count_status
int run(const char *arg);
#endif

static int test_count(MPI_Count count)
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

int run(const char *arg)
{
    int errs = 0;

    /* baseline: this tiny value should pose no problems */
    errs += test_count(60);
    /* one with no next-to-high-bits set */
    errs += test_count(0x3654321f71234567);
    /* masking after shift can help the count_high, but count_low is still
     * wrong */
    errs += test_count(0x7654321f71234567);
    /* original problematic count reported by Artem Yalozo */
    errs += test_count(0x7654321ff1234567);

    return errs;
}
