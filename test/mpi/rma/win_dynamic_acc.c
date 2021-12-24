/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run rma_win_dynamic_acc
int run(const char *arg);
#endif

#define ITER_PER_RANK 25

static const int verbose = 0;
static int do_collattach = 0;

int run(const char *arg)
{
    int i, rank, nproc;
    int errors = 0;
    int val = 0, one = 1;
    int iter;
    MPI_Aint *val_ptrs;
    MPI_Win dyn_win;

    /* Processing input arguments */
    MTestArgList *head = MTestArgListCreate_arg(arg);
    do_collattach = MTestArgListGetInt_with_default(head, "collattach", 0);
    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    iter = ITER_PER_RANK * nproc;

    val_ptrs = malloc(nproc * sizeof(MPI_Aint));
    MPI_Get_address(&val, &val_ptrs[rank]);

    MPI_Allgather(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, val_ptrs, 1, MPI_AINT, MPI_COMM_WORLD);

    MPI_Info info = MPI_INFO_NULL;
    if (do_collattach) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "coll_attach", "true");
    }

    MPI_Win_create_dynamic(info, MPI_COMM_WORLD, &dyn_win);

    if (info != MPI_INFO_NULL) {
        MPI_Info_free(&info);
    }

    MPI_Win_attach(dyn_win, &val, sizeof(int));

    for (i = 0; i < iter; i++) {
        MPI_Win_fence(MPI_MODE_NOPRECEDE, dyn_win);
        MPI_Accumulate(&one, 1, MPI_INT, i % nproc, val_ptrs[i % nproc], 1, MPI_INT, MPI_SUM,
                       dyn_win);
        MPI_Win_fence(MPI_MODE_NOSUCCEED, dyn_win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Read and verify my data */
    if (val != iter) {
        errors++;
        printf("%d -- Got %d, expected %d\n", rank, val, iter);
    }

    MPI_Win_detach(dyn_win, &val);
    MPI_Win_free(&dyn_win);

    free(val_ptrs);

    return errors;
}
