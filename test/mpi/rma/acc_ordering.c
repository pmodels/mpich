/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* This test checks accumulate ordering in three cases:
 * 1) Default (most restricted)
 * 2) None (no ordering)
 * 3) Mixture of REPLACE and MIN_LOC/MAX_LOC.
 *    (In CH4/OFI, REPLACE may go through OFI but MIN_LOC/MAX_LOC has to go through active message.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <limits.h>
#include "mpitest.h"

typedef struct {
    int val;
    int loc;
} twoint_t;

#define ARRAY_LEN (8192/(sizeof(twoint_t)))

static int errs = 0;

void verify_result(twoint_t * data, int len, twoint_t expected, const char *test_name)
{
    int i;
    for (i = 0; i < len; i++) {
        if ((data[i].val != expected.val) || (data[i].loc != expected.loc)) {
            errs++;
            printf("%s: Expected: { loc = %d, val = %d }  Actual: { loc = %d, val = %d }\n",
                   test_name, expected.loc, expected.val, data[i].loc, data[i].val);
        }
    }
}

/* Check non-deterministic result of none ordering.
 * Expected result has two possibilities. */
void verify_nondeterministic_result(twoint_t * data,
                                    int len, twoint_t * expected, const char *test_name)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!((data[i].loc == expected[0].loc && data[0].val == expected[0].val)
              || (data[i].loc == expected[1].loc && data[i].val == expected[1].val))) {
            errs++;
            printf
                ("%s: Expected: { loc = %d, val = %d } or { loc = %d, val = %d }; Actual: { loc = %d, val = %d }\n",
                 test_name, expected[0].loc, expected[0].val, expected[1].loc, expected[1].val,
                 data[i].loc, data[i].val);
        }
    }
}

int main(int argc, char **argv)
{
    int me, nproc, i;
    twoint_t *data, *mine, *mine_plus, *expected;
    MPI_Win win;
    MPI_Info info_in;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (nproc < 2) {
        printf("Run this program with 2 or more processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    data = (twoint_t *) calloc(ARRAY_LEN, sizeof(twoint_t));
    if (me == 0) {
        /* length of 2 in order to store all results for none ordering. */
        expected = (twoint_t *) malloc(2 * sizeof(twoint_t));
    }
    if (me == nproc - 1) {
        mine = (twoint_t *) malloc(ARRAY_LEN * sizeof(twoint_t));
        mine_plus = (twoint_t *) malloc(ARRAY_LEN * sizeof(twoint_t));
        for (i = 0; i < ARRAY_LEN; i++) {
            mine[i].val = me + 1;
            mine[i].loc = me;
            mine_plus[i].val = me + 2;
            mine_plus[i].loc = me + 1;
        }
    }

    /* 1. Default ordering */
    MPI_Win_create(data, sizeof(twoint_t) * ARRAY_LEN, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    MPI_Win_fence(0, win);

    /* Rank np-1 performs 2 WRITE to rank 0. */
    /* 1.a. Single data test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_REPLACE, win);
        MPI_Accumulate(mine_plus, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_REPLACE, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        expected[0].loc = nproc;
        expected[0].val = nproc + 1;
        verify_result(data, 1, expected[0], "Single data test case for default ordering");
    }

    if (me == 0) {
        data[0].loc = 0;
        data[0].val = 0;
    }
    MPI_Win_fence(0, win);
    /* 1.b. Large arrary test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_REPLACE, win);
        MPI_Accumulate(mine_plus, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_REPLACE, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        verify_result(data, ARRAY_LEN, expected[0], "Large array test case for default ordering");
    }
    /* MPI Implementation may ignore window info changes once created.
     * Thus we should free current window and create a new window with required info. */
    MPI_Win_free(&win);

    /* 2. None ordering */
    if (me == 0) {
        /* reset data on rank 0. */
        memset((void *) data, 0, ARRAY_LEN * sizeof(twoint_t));
    }

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, "accumulate_ordering", "none");
    MPI_Win_create(data, sizeof(twoint_t) * ARRAY_LEN, 1, info_in, MPI_COMM_WORLD, &win);
    MPI_Info_free(&info_in);

    MPI_Win_fence(0, win);

    /* Rank np-1 performs 2 WRITE to rank 0. */
    /* 2.a. Single data test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_REPLACE, win);
        MPI_Accumulate(mine_plus, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_REPLACE, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        expected[0].loc = nproc - 1;
        expected[0].val = nproc;
        expected[1].loc = nproc;
        expected[1].val = nproc + 1;
        verify_nondeterministic_result(data, 1, expected,
                                       "Single data test case for none ordering");
    }

    if (me == 0) {
        data[0].loc = 0;
        data[0].val = 0;
    }
    MPI_Win_fence(0, win);
    /* 2.b. Large array test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_REPLACE, win);
        MPI_Accumulate(mine_plus, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_REPLACE, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        verify_nondeterministic_result(data, ARRAY_LEN, expected,
                                       "Large array test case for none ordering");
    }

    MPI_Win_free(&win);

    /* 3. Mix MAX_LOC/MIN_LOC and MPI_REPLACE */
    if (me == 0) {
        /* reset data on rank 0. */
        memset((void *) data, 0, ARRAY_LEN * sizeof(twoint_t));
    }

    MPI_Win_create(data, sizeof(twoint_t) * ARRAY_LEN, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Win_fence(0, win);

    /* Rank np-1 performs 2 WRITE to rank 0. */
    /* Test MAXLOC */
    /* 3.a. Single data test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine_plus, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_MAXLOC, win);
        MPI_Accumulate(mine, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_REPLACE, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        expected[0].loc = nproc - 1;
        expected[0].val = nproc;
        verify_result(data, 1, expected[0], "Single data test case for MAXLOC and REPLACE");
    }

    if (me == 0) {
        data[0].loc = 0;
        data[0].val = 0;
    }
    MPI_Win_fence(0, win);
    /* 3.b. Large array test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine_plus, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_MAXLOC, win);
        MPI_Accumulate(mine, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_REPLACE, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        verify_result(data, ARRAY_LEN, expected[0], "Large array test case for MAXLOC and REPLACE");
    }

    /* Test MINLOC */
    if (me == 0) {
        for (i = 0; i < ARRAY_LEN; i++) {
            data[i].loc = INT_MAX;
            data[i].val = INT_MAX;
        }
    }
    MPI_Win_fence(0, win);
    /* Rank np-1 performs 2 WRITE to rank 0. */
    /* 3.c. Single data test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine_plus, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_REPLACE, win);
        MPI_Accumulate(mine, 1, MPI_2INT, 0, 0, 1, MPI_2INT, MPI_MINLOC, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        expected[0].loc = nproc - 1;
        expected[0].val = nproc;
        verify_result(data, 1, expected[0], "Single data test case for REPLACE and MINLOC");
    }

    if (me == 0) {
        data[0].loc = INT_MAX;
        data[0].val = INT_MAX;
    }
    MPI_Win_fence(0, win);
    /* 3.d. Large array test */
    if (me == nproc - 1) {
        MPI_Accumulate(mine_plus, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_REPLACE, win);
        MPI_Accumulate(mine, ARRAY_LEN, MPI_2INT, 0, 0, ARRAY_LEN, MPI_2INT, MPI_MINLOC, win);
    }

    MPI_Win_fence(0, win);
    if (me == 0) {
        verify_result(data, ARRAY_LEN, expected[0], "Single data test case for REPLACE and MINLOC");
    }

    MPI_Win_free(&win);
    if (me == nproc - 1) {
        free(mine);
        free(mine_plus);
    }
    if (me == 0)
        free(expected);

    MTest_Finalize(errs);
    free(data);

    return MTestReturnValue(errs);
}
