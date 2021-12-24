/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <assert.h>
#include "squelch.h"

#ifdef MULTI_TESTS
#define run rma_fetch_and_op
int run(const char *arg);
#endif

#define ITER 100

#define MAX_TYPE_SIZE 16

static int do_hwacc_info = 0;
static int do_accop_info = 0;

/* return true if not equal */
static bool cmp_value(MPI_Datatype dt, void *buf, int value)
{
    if (dt == MPI_CHAR) {
        return *(char *) buf != (char) value;
    } else if (dt == MPI_SHORT) {
        return *(short *) buf != (short) value;
    } else if (dt == MPI_INT) {
        return *(int *) buf != (int) value;
    } else if (dt == MPI_LONG) {
        return *(long *) buf != (long) value;
    } else if (dt == MPI_DOUBLE) {
        return *(double *) buf != (double) value;
#ifdef HAVE_LONG_DOUBLE
    } else if (dt == MPI_LONG_DOUBLE) {
        return *(long double *) buf != (long double) value;
#endif
    } else {
        assert(0);
    }
    return false;
}

static void set_value(MPI_Datatype dt, void *buf, int value)
{
    if (dt == MPI_CHAR) {
        *(char *) buf = (char) value;
    } else if (dt == MPI_SHORT) {
        *(short *) buf = (short) value;
    } else if (dt == MPI_INT) {
        *(int *) buf = (int) value;
    } else if (dt == MPI_LONG) {
        *(long *) buf = (long) value;
    } else if (dt == MPI_DOUBLE) {
        *(double *) buf = (double) value;
#ifdef HAVE_LONG_DOUBLE
    } else if (dt == MPI_LONG_DOUBLE) {
        *(long double *) buf = (long double) value;
#endif
    } else {
        assert(0);
    }
}

static int get_value(MPI_Datatype dt, void *buf)
{
    if (dt == MPI_CHAR) {
        return (int) *(char *) buf;
    } else if (dt == MPI_SHORT) {
        return (int) *(short *) buf;
    } else if (dt == MPI_INT) {
        return (int) *(int *) buf;
    } else if (dt == MPI_LONG) {
        return (int) *(long *) buf;
    } else if (dt == MPI_DOUBLE) {
        return (int) *(double *) buf;
#ifdef HAVE_LONG_DOUBLE
    } else if (dt == MPI_LONG_DOUBLE) {
        return (int) *(long double *) buf;
#endif
    } else {
        assert(0);
    }
    return 0;
}

static void reset_vars(MPI_Datatype dt, int type_size, void *val_ptr, void *res_ptr, MPI_Win win)
{
    int i, rank, nproc;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        set_value(dt, (char *) val_ptr + i * type_size, 0);
        set_value(dt, (char *) res_ptr + i * type_size, -1);
    }
    MPI_Win_unlock(rank, win);

    MPI_Barrier(MPI_COMM_WORLD);
}

int run(const char *arg)
{
    int i, rank, nproc, mpi_type_size;
    int errors = 0;
    void *val_ptr, *res_ptr;
    MPI_Win win;

    MPI_Datatype mpi_type = MPI_DATATYPE_NULL;
    int type_size = 0;

    MTestArgList *head = MTestArgListCreate_arg(arg);

    do_hwacc_info = MTestArgListGetInt_with_default(head, "hwacc", 0);
    do_accop_info = MTestArgListGetInt_with_default(head, "accop", 0);

    char *name = MTestArgListGetString(head, "type");
    if (strcmp(name, "char") == 0) {
        mpi_type = MPI_CHAR;
        type_size = sizeof(char);
    } else if (strcmp(name, "short") == 0) {
        mpi_type = MPI_SHORT;
        type_size = sizeof(short);
    } else if (strcmp(name, "int") == 0) {
        mpi_type = MPI_INT;
        type_size = sizeof(int);
    } else if (strcmp(name, "long") == 0) {
        mpi_type = MPI_LONG;
        type_size = sizeof(long);
    } else if (strcmp(name, "double") == 0) {
        mpi_type = MPI_DOUBLE;
        type_size = sizeof(double);
#ifdef HAVE_LONG_DOUBLE
    } else if (strcmp(name, "long_double") == 0) {
        mpi_type = MPI_LONG_DOUBLE;
        type_size = sizeof(long double);
#endif
    } else {
        assert(0);
    }

    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Type_size(mpi_type, &mpi_type_size);
    assert(mpi_type_size == type_size);

    val_ptr = malloc(type_size * nproc);
    res_ptr = malloc(type_size * nproc);
    MTEST_VG_MEM_INIT(val_ptr, type_size * nproc);
    MTEST_VG_MEM_INIT(res_ptr, type_size * nproc);

    MPI_Info info = MPI_INFO_NULL;

    if (do_hwacc_info) {
        MPI_Info_create(&info);
        MPI_Info_set(info, "disable_shm_accumulate", "true");
    }

    if (do_accop_info) {
        if (info == MPI_INFO_NULL)
            MPI_Info_create(&info);
        MPI_Info_set(info, "which_accumulate_ops", "sum,no_op");
    }

    MPI_Win_create(val_ptr, type_size * nproc, type_size, info, MPI_COMM_WORLD, &win);

    if (info != MPI_INFO_NULL) {
        MPI_Info_free(&info);
    }

    /* Test self communication */

    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    for (i = 0; i < ITER; i++) {
        char one[MAX_TYPE_SIZE];
        char result[MAX_TYPE_SIZE];
        set_value(mpi_type, one, 1);
        set_value(mpi_type, result, -1);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
        MPI_Fetch_and_op(one, result, mpi_type, rank, 0, MPI_SUM, win);
        MPI_Win_unlock(rank, win);
    }

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (cmp_value(mpi_type, val_ptr, ITER)) {
        SQUELCH(printf("%d->%d -- SELF: expected %d, got %d\n", rank, rank, ITER,
                       get_value(mpi_type, val_ptr)););
        errors++;
    }
    MPI_Win_unlock(rank, win);

    /* Test neighbor communication */

    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    for (i = 0; i < ITER; i++) {
        char one[MAX_TYPE_SIZE];
        char result[MAX_TYPE_SIZE];
        set_value(mpi_type, one, 1);
        set_value(mpi_type, result, -1);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, (rank + 1) % nproc, 0, win);
        MPI_Fetch_and_op(&one, &result, mpi_type, (rank + 1) % nproc, 0, MPI_SUM, win);
        MPI_Win_unlock((rank + 1) % nproc, win);

        if (cmp_value(mpi_type, result, i)) {
            SQUELCH(printf("%d->%d -- NEIGHBOR[%d]: expected result %d, got %d\n",
                           (rank + 1) % nproc, rank, i, i, get_value(mpi_type, result)););
            errors++;
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (cmp_value(mpi_type, val_ptr, ITER)) {
        SQUELCH(printf("%d->%d -- NEIGHBOR: expected %d, got %d\n",
                       (rank + 1) % nproc, rank, ITER, get_value(mpi_type, val_ptr)););
        errors++;
    }
    MPI_Win_unlock(rank, win);

    /* Test contention */

    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    if (rank != 0) {
        for (i = 0; i < ITER; i++) {
            char one[MAX_TYPE_SIZE];
            char result[MAX_TYPE_SIZE];
            set_value(mpi_type, one, 1);

            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
            MPI_Fetch_and_op(&one, &result, mpi_type, 0, 0, MPI_SUM, win);
            MPI_Win_unlock(0, win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (rank == 0 && nproc > 1) {
        if (cmp_value(mpi_type, val_ptr, ITER * (nproc - 1))) {
            SQUELCH(printf("*->%d - CONTENTION: expected=%d, val=%d\n", rank,
                           ITER * (nproc - 1), get_value(mpi_type, val_ptr)););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test all-to-all communication (fence) */

    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    for (i = 0; i < ITER; i++) {
        char rank_cnv[MAX_TYPE_SIZE];
        set_value(mpi_type, rank_cnv, rank);

        MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
        for (int j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + j * type_size;
            MPI_Fetch_and_op(rank_cnv, p, mpi_type, j, rank, MPI_SUM, win);
        }
        MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (int j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(mpi_type, p, i * rank)) {
                SQUELCH(printf("%d->%d -- ALL-TO-ALL (FENCE) [%d]: expected result %d, got %d\n",
                               rank, j, i, i * rank, get_value(mpi_type, p)););
                errors++;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        void *p = (char *) val_ptr + i * type_size;
        if (cmp_value(mpi_type, p, ITER * i)) {
            SQUELCH(printf("%d->%d -- ALL-TO-ALL (FENCE): expected %d, got %d\n", i,
                           rank, ITER * i, get_value(mpi_type, p)););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test all-to-all communication (lock-all) */

    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    for (i = 0; i < ITER; i++) {
        char rank_cnv[MAX_TYPE_SIZE];
        set_value(mpi_type, rank_cnv, rank);

        MPI_Win_lock_all(0, win);
        for (int j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + j * type_size;
            MPI_Fetch_and_op(&rank_cnv, p, mpi_type, j, rank, MPI_SUM, win);
        }
        MPI_Win_unlock_all(win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (int j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(mpi_type, p, i * rank)) {
                SQUELCH(printf("%d->%d -- ALL-TO-ALL (LOCK-ALL) [%d]: expected result %d, got %d\n",
                               rank, j, i, i * rank, get_value(mpi_type, p)););
                errors++;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        void *p = (char *) val_ptr + i * type_size;
        if (cmp_value(mpi_type, p, ITER * i)) {
            SQUELCH(printf("%d->%d -- ALL-TO-ALL (LOCK-ALL): expected %d, got %d\n",
                           i, rank, ITER * i, get_value(mpi_type, p)););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test all-to-all communication (lock-all+flush) */

    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    for (i = 0; i < ITER; i++) {
        int j;

        MPI_Win_lock_all(0, win);
        for (j = 0; j < nproc; j++) {
            char rank_cnv[MAX_TYPE_SIZE];
            set_value(mpi_type, rank_cnv, rank);

            void *p = (char *) res_ptr + j * type_size;
            MPI_Fetch_and_op(rank_cnv, p, mpi_type, j, rank, MPI_SUM, win);
            MPI_Win_flush(j, win);
        }
        MPI_Win_unlock_all(win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(mpi_type, p, i * rank)) {
                SQUELCH(printf
                        ("%d->%d -- ALL-TO-ALL (LOCK-ALL+FLUSH) [%d]: expected result %d, got %d\n",
                         rank, j, i, i * rank, get_value(mpi_type, p)););
                errors++;
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        void *p = (char *) val_ptr + i * type_size;
        if (cmp_value(mpi_type, p, ITER * i)) {
            SQUELCH(printf("%d->%d -- ALL-TO-ALL (LOCK-ALL+FLUSH): expected %d, got %d\n",
                           i, rank, ITER * i, get_value(mpi_type, p)););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test NO_OP (neighbor communication) */

    MPI_Barrier(MPI_COMM_WORLD);
    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        set_value(mpi_type, (char *) val_ptr + i * type_size, rank);
    }
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < ITER; i++) {
        int target = (rank + 1) % nproc;

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Fetch_and_op(NULL, res_ptr, mpi_type, target, 0, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        if (cmp_value(mpi_type, res_ptr, target)) {
            SQUELCH(printf("%d->%d -- NOP[%d]: expected %d, got %d\n",
                           target, rank, i, target, get_value(mpi_type, res_ptr)););
            errors++;
        }
    }

    /* Test NO_OP (self communication) */

    MPI_Barrier(MPI_COMM_WORLD);
    reset_vars(mpi_type, type_size, val_ptr, res_ptr, win);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        set_value(mpi_type, (char *) val_ptr + i * type_size, rank);
    }
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < ITER; i++) {
        int target = rank;

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Fetch_and_op(NULL, res_ptr, mpi_type, target, 0, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        if (cmp_value(mpi_type, res_ptr, target)) {
            SQUELCH(printf("%d->%d -- NOP_SELF[%d]: expected %d, got %d\n",
                           target, rank, i, target, get_value(mpi_type, res_ptr)););
            errors++;
        }
    }

    MPI_Win_free(&win);

    free(val_ptr);
    free(res_ptr);

    return errors;
}
