/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/* This test program checks that the point-to-point completion routines
   can be applied to an inactive persistent request, as required by the
   MPI-1 standard. See section 3.7.3, for example,

   One is allowed to call MPI TEST with a null or inactive request argument.
   In such a case the operation returns with flag = true and empty status.

*/

static int StatusEmpty(MPI_Status * s)
{
    int errs = 0;
    int count = 10;

    if (s->MPI_TAG != MPI_ANY_TAG) {
        errs++;
        printf("MPI_TAG not MPI_ANY_TAG in status\n");
    }
    if (s->MPI_SOURCE != MPI_ANY_SOURCE) {
        errs++;
        printf("MPI_SOURCE not MPI_ANY_SOURCE in status\n");
    }
    MPI_Get_count(s, MPI_INT, &count);
    if (count != 0) {
        errs++;
        printf("count in status is not 0\n");
    }
    /* Return true only if status passed all tests */
    return errs ? 0 : 1;
}

static int test_recv_init(int src_rank, const char *test_name)
{
    MPI_Request r;
    MPI_Status s;
    int errs = 0;
    int flag;
    int buf[10];
    int tag = 27;

    MPI_Recv_init(buf, 10, MPI_INT, src_rank, tag, MPI_COMM_WORLD, &r);

    flag = 0;
    s.MPI_TAG = 10;
    s.MPI_SOURCE = 10;
    MPI_Test(&r, &flag, &s);
    if (!flag) {
        errs++;
        printf("Flag not true after MPI_Test (%s)\n", test_name);
        printf("Aborting further tests to avoid hanging in MPI_Wait\n");
        return errs;
    }
    if (!StatusEmpty(&s)) {
        errs++;
        printf("Status not empty after MPI_Test (%s)\n", test_name);
    }

    s.MPI_TAG = 10;
    s.MPI_SOURCE = 10;
    MPI_Wait(&r, &s);
    if (!StatusEmpty(&s)) {
        errs++;
        printf("Status not empty after MPI_Wait (%s)\n", test_name);
    }

    MPI_Request_free(&r);

    return errs;
}

static void test_proc_null(void)
{
    MPI_Request r;
    MPI_Status s;
    int buf[10];
    int tag = 27;

    MPI_Recv_init(buf, 10, MPI_INT, MPI_PROC_NULL, tag, MPI_COMM_WORLD, &r);
    MPI_Start(&r);
    MPI_Wait(&r, &s);
    MPI_Request_free(&r);

    MPI_Send_init(buf, 10, MPI_INT, MPI_PROC_NULL, tag, MPI_COMM_WORLD, &r);
    MPI_Start(&r);
    MPI_Wait(&r, &s);

    MPI_Request_free(&r);
}

int main(int argc, char *argv[])
{
    MPI_Request r;
    MPI_Status s;
    int errs = 0, e;
    int flag;
    int buf[10];
    int rbuf[10];
    int tag = 27;
    int dest = 0;
    int rank, size;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
        MTestPrintfMsg(1, "Create a persistent send request\n");

    MPI_Send_init(buf, 10, MPI_INT, dest, tag, MPI_COMM_WORLD, &r);

    flag = 0;
    s.MPI_TAG = 10;
    s.MPI_SOURCE = 10;
    MPI_Test(&r, &flag, &s);
    if (!flag) {
        errs++;
        printf("Flag not true after MPI_Test (send)\n");
        printf("Aborting further tests to avoid hanging in MPI_Wait\n");
        MTest_Finalize(errs);
        return MTestReturnValue(errs);
    }
    if (!StatusEmpty(&s)) {
        errs++;
        printf("Status not empty after MPI_Test (send)\n");
    }

    s.MPI_TAG = 10;
    s.MPI_SOURCE = 10;
    MPI_Wait(&r, &s);
    if (!StatusEmpty(&s)) {
        errs++;
        printf("Status not empty after MPI_Wait (send)\n");
    }

    /* Now try to use that request, then check again */
    if (rank == 0) {
        int i;
        MPI_Request *rr = (MPI_Request *) malloc(size * sizeof(MPI_Request));
        for (i = 0; i < size; i++) {
            MPI_Irecv(rbuf, 10, MPI_INT, i, tag, MPI_COMM_WORLD, &rr[i]);
        }
        MPI_Start(&r);
        MPI_Wait(&r, &s);
        MPI_Waitall(size, rr, MPI_STATUSES_IGNORE);
        free(rr);
    } else {
        MPI_Start(&r);
        MPI_Wait(&r, &s);
    }

    flag = 0;
    s.MPI_TAG = 10;
    s.MPI_SOURCE = 10;
    MPI_Test(&r, &flag, &s);
    if (!flag) {
        errs++;
        printf("Flag not true after MPI_Test (send)\n");
        printf("Aborting further tests to avoid hanging in MPI_Wait\n");
        MTest_Finalize(errs);
        return MTestReturnValue(errs);
    }
    if (!StatusEmpty(&s)) {
        errs++;
        printf("Status not empty after MPI_Test (send)\n");
    }

    s.MPI_TAG = 10;
    s.MPI_SOURCE = 10;
    MPI_Wait(&r, &s);
    if (!StatusEmpty(&s)) {
        errs++;
        printf("Status not empty after MPI_Wait (send)\n");
    }

    MPI_Request_free(&r);

    if (rank == 0)
        MTestPrintfMsg(1, "Create a persistent receive request\n");

    e = test_recv_init(dest, "recv");
    errs += e;
    if (e)
        goto fn_exit;

    if (rank == 0)
        MTestPrintfMsg(1, "Create a persistent receive (ANY_SOURCE) request\n");

    errs += test_recv_init(MPI_ANY_SOURCE, "recv-anysource");
    test_proc_null();

  fn_exit:
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
