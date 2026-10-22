/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run rma_win_shared_query
int run(const char *arg);
#endif

#define ELEM_PER_PROC 100
#define ELEM_SIZE sizeof(int)
#define SIZE (ELEM_PER_PROC * ELEM_SIZE)

enum {
    CREATE,
    ALLOCATE,
    ALLOCATE_SHARED,
};

int run(const char *arg)
{
    int errors = 0;
    MPI_Comm comm;
    MPI_Win win;

    int opt_win_flavor = ALLOCATE;
    MTestArgList *head = MTestArgListCreate_arg(arg);
    const char *flavor = MTestArgListGetString_with_default(head, "flavor", "allocate");
    if (strcmp(flavor, "create") == 0) {
        opt_win_flavor = CREATE;
    } else if (strcmp(flavor, "allocate_shared") == 0) {
        opt_win_flavor = ALLOCATE_SHARED;
    }
    MTestArgListDestroy(head);


    if (opt_win_flavor == CREATE) {
        MTestPrintfMsg(1, "TEST MPI_Win_create\n");
        comm = MPI_COMM_WORLD;
    } else if (opt_win_flavor == ALLOCATE) {
        MTestPrintfMsg(1, "TEST MPI_Win_allocate\n");
        comm = MPI_COMM_WORLD;
    } else if (opt_win_flavor == ALLOCATE_SHARED) {
        MTestPrintfMsg(1, "TEST MPI_Win_allocate_shared\n");
        MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &comm);
    } else {
        printf("Invalid opt_win_flavor: %d\n", opt_win_flavor);
        goto fn_exit;
    }

    int rank, nproc;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    int *my_base;
    if (opt_win_flavor == CREATE) {
        my_base = malloc(SIZE);
        MPI_Win_create(my_base, SIZE, ELEM_SIZE, MPI_INFO_NULL, comm, &win);
    } else if (opt_win_flavor == ALLOCATE) {
        MPI_Win_allocate(SIZE, ELEM_SIZE, MPI_INFO_NULL, comm, &my_base, &win);
    } else if (opt_win_flavor == ALLOCATE_SHARED) {
        MPI_Win_allocate_shared(SIZE, ELEM_SIZE, MPI_INFO_NULL, comm, &my_base, &win);
    }

    for (int i = 0; i < nproc; i++) {
        MPI_Aint size;
        int disp_unit;
        void *base;
        MPI_Win_shared_query(win, i, &size, &disp_unit, &base);
        MTestPrintfMsg(1, "[%d] query [%d / %d]: %p - size=%ld, disp_unit=%d\n",
                       rank, i, nproc, base, (long) size, disp_unit);
    }

    MPI_Win_free(&win);

    if (opt_win_flavor == CREATE) {
        free(my_base);
    } else if (opt_win_flavor == ALLOCATE_SHARED) {
        MPI_Comm_free(&comm);
    }

  fn_exit:
    return errors;
}
