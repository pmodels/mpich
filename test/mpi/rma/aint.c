/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * This program tests MPI_Aint_add/diff in MPI-3.1.
 * The two functions are often used in RMA code.
 * See https://svn.mpi-forum.org/trac/mpi-forum-web/ticket/349
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_aint
int run(const char *arg);
#endif

static int do_collattach = 0;

int run(const char *arg)
{
    int rank, nproc;
    int errs = 0;
    int array[1024];
    int val = 0;
    int target_rank = 0;        /* suppress warnings */
    MPI_Aint bases[2];
    MPI_Aint disp, offset = 0;  /* suppress warnings */
    MPI_Win win;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    do_collattach = MTestArgListGetInt_with_default(head, "collattach", 0);
    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (rank == 0 && nproc != 2) {
        MTestError("Must run with 2 ranks\n");
    }

    /* Get the base address in the middle of the array */
    if (rank == 0) {
        target_rank = 1;
        array[0] = 1234;
        MPI_Get_address(&array[512], &bases[0]);
    } else if (rank == 1) {
        target_rank = 0;
        array[1023] = 1234;
        MPI_Get_address(&array[512], &bases[1]);
    }

    /* Exchange bases */
    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, bases, 1, MPI_AINT, MPI_COMM_WORLD);

    MPI_Info info = MPI_INFO_NULL;
    if (do_collattach) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "coll_attach", "true");
    }

    MPI_Win_create_dynamic(info, MPI_COMM_WORLD, &win);
    MPI_Win_attach(win, array, sizeof(int) * 1024);

    if (info != MPI_INFO_NULL) {
        MPI_Info_free(&info);
    }

    /* Do MPI_Aint addressing arithmetic */
    if (rank == 0) {
        disp = sizeof(int) * 511;
        offset = MPI_Aint_add(bases[1], disp);  /* offset points to array[1023] */
    } else if (rank == 1) {
        disp = sizeof(int) * 512;
        offset = MPI_Aint_diff(bases[0], disp); /* offset points to array[0] */
    }

    /* Get val and verify it */
    MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
    MPI_Get(&val, 1, MPI_INT, target_rank, offset, 1, MPI_INT, win);
    MPI_Win_fence(MPI_MODE_NOSUCCEED, win);

    if (val != 1234) {
        errs++;
        printf("%d -- Got %d, expected 1234\n", rank, val);
    }

    MPI_Win_detach(win, array);
    MPI_Win_free(&win);

    return errs;
}
