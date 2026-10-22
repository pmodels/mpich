/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>
#include <assert.h>
#include "squelch.h"

#ifdef MULTI_TESTS
#define run rma_get_accumulate
int run(const char *arg);
#endif

static int do_derived;
static MPI_Datatype mpi_base_type = MPI_DATATYPE_NULL;
static int type_size;
static int rank, nproc;

#define ITER  100
#define NUM_COUNT 2
static int test_counts[NUM_COUNT] = { 5, 1000 };

/* return true if not equal */
static bool cmp_value(void *buf, int value)
{
    if (mpi_base_type == MPI_CHAR) {
        return *(char *) buf != (char) value;
    } else if (mpi_base_type == MPI_SHORT) {
        return *(short *) buf != (short) value;
    } else if (mpi_base_type == MPI_INT) {
        return *(int *) buf != (int) value;
    } else if (mpi_base_type == MPI_LONG) {
        return *(long *) buf != (long) value;
    } else if (mpi_base_type == MPI_DOUBLE) {
        return *(double *) buf != (double) value;
    } else if (mpi_base_type == MPI_LONG_DOUBLE) {
        return *(long double *) buf != (long double) value;
    } else {
        assert(0);
    }
    return false;
}

static void set_value(void *buf, int value)
{
    if (mpi_base_type == MPI_CHAR) {
        *(char *) buf = (char) value;
    } else if (mpi_base_type == MPI_SHORT) {
        *(short *) buf = (short) value;
    } else if (mpi_base_type == MPI_INT) {
        *(int *) buf = (int) value;
    } else if (mpi_base_type == MPI_LONG) {
        *(long *) buf = (long) value;
    } else if (mpi_base_type == MPI_DOUBLE) {
        *(double *) buf = (double) value;
    } else if (mpi_base_type == MPI_LONG_DOUBLE) {
        *(long double *) buf = (long double) value;
    } else {
        printf("set_value: mpi_base_type = %x\n", mpi_base_type);
        assert(0);
    }
}

static int get_value(void *buf)
{
    if (mpi_base_type == MPI_CHAR) {
        return (int) *(char *) buf;
    } else if (mpi_base_type == MPI_SHORT) {
        return (int) *(short *) buf;
    } else if (mpi_base_type == MPI_INT) {
        return (int) *(int *) buf;
    } else if (mpi_base_type == MPI_LONG) {
        return (int) *(long *) buf;
    } else if (mpi_base_type == MPI_DOUBLE) {
        return (int) *(double *) buf;
    } else if (mpi_base_type == MPI_LONG_DOUBLE) {
        return (int) *(long double *) buf;
    } else {
        printf("get_value: mpi_base_type = %x\n", mpi_base_type);
        assert(0);
    }
    return 0;
}

static void reset_bufs(void *win_ptr, void *res_ptr, void *val_ptr,
                       int value, int count, MPI_Win win)
{
    int i;

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    memset(win_ptr, 0, type_size * nproc * count);
    MPI_Win_unlock(rank, win);

    memset(res_ptr, -1, type_size * nproc * count);

    for (int i = 0; i < count; i++) {
        set_value((char *) val_ptr + i * type_size, value);
    }

    MPI_Barrier(MPI_COMM_WORLD);
}

static int test_self_communication(void *win_ptr, void *res_ptr, void *val_ptr,
                                   int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, count, win);

    for (int i = 0; i < ITER; i++) {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
        MPI_Get_accumulate(val_ptr, count, mpi_type, res_ptr, count, mpi_type,
                           rank, 0, count, mpi_type, MPI_SUM, win);
        MPI_Win_unlock(rank, win);
    }

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (int i = 0; i < count; i++) {
        void *p = (char *) win_ptr + i * type_size;
        if (cmp_value(p, ITER)) {
            SQUELCH(printf("%d->%d -- SELF[%d]: expected %d, got %d\n",
                           rank, rank, i, ITER, get_value(p)););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    return errors;
}

static int test_neighbor_communication(void *win_ptr, void *res_ptr, void *val_ptr,
                                       int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, count, win);

    for (int i = 0; i < ITER; i++) {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, (rank + 1) % nproc, 0, win);
        MPI_Get_accumulate(val_ptr, count, mpi_type, res_ptr, count, mpi_type,
                           (rank + 1) % nproc, 0, count, mpi_type, MPI_SUM, win);
        MPI_Win_unlock((rank + 1) % nproc, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (int i = 0; i < count; i++) {
        void *p = (char *) win_ptr + i * type_size;
        if (cmp_value(p, ITER)) {
            SQUELCH(printf("%d->%d -- NEIGHBOR[%d]: expected %d, got %d\n",
                           (rank + 1) % nproc, rank, i, ITER, get_value(p)););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    return errors;
}

static int test_contention(void *win_ptr, void *res_ptr, void *val_ptr,
                           int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, count, win);

    if (rank != 0) {
        for (int i = 0; i < ITER; i++) {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
            MPI_Get_accumulate(val_ptr, count, mpi_type, res_ptr, count, mpi_type,
                               0, 0, count, mpi_type, MPI_SUM, win);
            MPI_Win_unlock(0, win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (rank == 0 && nproc > 1) {
        for (int i = 0; i < count; i++) {
            void *p = (char *) win_ptr + i * type_size;
            if (cmp_value(p, ITER * (nproc - 1))) {
                SQUELCH(printf("*->%d - CONTENTION[%d]: expected=%d val=%d\n",
                               rank, i, ITER * (nproc - 1), get_value(p)););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);

    return errors;
}

static int test_alltoall_fence(void *win_ptr, void *res_ptr, void *val_ptr,
                               int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, rank, count, win);

    for (int i = 0; i < ITER; i++) {
        MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
        for (int j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + (j * count) * type_size;
            MPI_Get_accumulate(val_ptr, count, mpi_type, p, count, mpi_type,
                               j, rank * count, count, mpi_type, MPI_SUM, win);
        }
        MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (int j = 0; j < nproc; j++) {
            for (int c = 0; c < count; c++) {
                void *p = (char *) res_ptr + (j * count + c) * type_size;
                if (cmp_value(p, i * rank)) {
                    SQUELCH(printf
                            ("%d->%d -- ALL-TO-ALL (FENCE) [%d]: iter %d, expected result %d, got %d\n",
                             rank, j, c, i, i * rank, get_value(p)););
                    errors++;
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (int i = 0; i < nproc; i++) {
        for (int c = 0; c < count; c++) {
            void *p = (char *) win_ptr + (i * count + c) * type_size;
            if (cmp_value(p, ITER * i)) {
                SQUELCH(printf("%d->%d -- ALL-TO-ALL (FENCE): expected %d, got %d\n",
                               i, rank, ITER * i, get_value(p)););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);

    return errors;
}

static int test_alltoall_lockall(void *win_ptr, void *res_ptr, void *val_ptr,
                                 int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, rank, count, win);

    for (int i = 0; i < ITER; i++) {
        MPI_Win_lock_all(0, win);
        for (int j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + (j * count) * type_size;
            MPI_Get_accumulate(val_ptr, count, mpi_type, p, count, mpi_type,
                               j, rank * count, count, mpi_type, MPI_SUM, win);
        }
        MPI_Win_unlock_all(win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (int j = 0; j < nproc; j++) {
            for (int c = 0; c < count; c++) {
                void *p = (char *) res_ptr + (j * count + c) * type_size;
                if (cmp_value(p, i * rank)) {
                    SQUELCH(printf
                            ("%d->%d -- ALL-TO-ALL (LOCK-ALL) [%d]: iter %d, expected result %d, got %d\n",
                             rank, j, c, i, i * rank, get_value(p)););
                    errors++;
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (int i = 0; i < nproc; i++) {
        for (int c = 0; c < count; c++) {
            void *p = (char *) win_ptr + (i * count + c) * type_size;
            if (cmp_value(p, i * ITER)) {
                SQUELCH(printf("%d->%d -- ALL-TO-ALL (LOCK-ALL): expected %d, got %d\n",
                               i, rank, ITER * i, get_value(p)););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);

    return errors;
}

static int test_alltoall_lockall_flush(void *win_ptr, void *res_ptr, void *val_ptr,
                                       int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, rank, count, win);

    for (int i = 0; i < ITER; i++) {
        MPI_Win_lock_all(0, win);
        for (int j = 0; j < nproc; j++) {
            void *p = (char *) res_ptr + (j * count) * type_size;
            MPI_Get_accumulate(val_ptr, count, mpi_type, p, count, mpi_type,
                               j, rank * count, count, mpi_type, MPI_SUM, win);
            MPI_Win_flush(j, win);
        }
        MPI_Win_unlock_all(win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (int j = 0; j < nproc; j++) {
            for (int c = 0; c < count; c++) {
                void *p = (char *) res_ptr + (j * count + c) * type_size;
                if (cmp_value(p, i * rank)) {
                    SQUELCH(printf
                            ("%d->%d -- ALL-TO-ALL (LOCK-ALL+FLUSH) [%d]: iter %d, expected result %d, got %d\n",
                             rank, j, c, i, i * rank, get_value(p)););
                    errors++;
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (int i = 0; i < nproc; i++) {
        for (int c = 0; c < count; c++) {
            void *p = (char *) win_ptr + (i * count + c) * type_size;
            if (cmp_value(p, ITER * i)) {
                SQUELCH(printf("%d->%d -- ALL-TO-ALL (LOCK-ALL+FLUSH): expected %d, got %d\n",
                               i, rank, ITER * i, get_value(p)););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);
    return errors;
}

static int test_neighbor_noop(void *win_ptr, void *res_ptr, void *val_ptr,
                              int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, count, win);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (int i = 0; i < count * nproc; i++) {
        set_value((char *) win_ptr + i * type_size, rank);
    }
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < ITER; i++) {
        int target = (rank + 1) % nproc;

        /* Test: origin_buf = NULL */
        memset(res_ptr, -1, type_size * nproc * count); /* reset result buffer. */

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, count, mpi_type, res_ptr, count, mpi_type,
                           target, 0, count, mpi_type, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (int j = 0; j < count; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(p, target)) {
                SQUELCH(printf("%d->%d -- NOP(1)[%d]: expected %d, got %d\n",
                               target, rank, i, target, get_value(p)););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0 */
        memset(res_ptr, -1, type_size * nproc * count);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, mpi_type, res_ptr, count, mpi_type,
                           target, 0, count, mpi_type, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (int j = 0; j < count; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(p, target)) {
                SQUELCH(printf("%d->%d -- NOP(2)[%d]: expected %d, got %d\n",
                               target, rank, i, target, get_value(p)););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0, origin_dtype = NULL */
        memset(res_ptr, -1, type_size * nproc * count);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, MPI_DATATYPE_NULL, res_ptr, count, mpi_type,
                           target, 0, count, mpi_type, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (int j = 0; j < count; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(p, target)) {
                SQUELCH(printf("%d->%d -- NOP(2)[%d]: expected %d, got %d\n",
                               target, rank, i, target, get_value(p)););
                errors++;
            }
        }
    }

    return errors;
}

static int test_self_noop(void *win_ptr, void *res_ptr, void *val_ptr,
                          int count, MPI_Datatype mpi_type, MPI_Win win)
{
    int errors = 0;

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, count, win);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (int i = 0; i < count * nproc; i++) {
        set_value((char *) win_ptr + i * type_size, rank);
    }
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < ITER; i++) {
        int target = rank;

        /* Test: origin_buf = NULL */
        memset(res_ptr, -1, type_size * nproc * count);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, count, mpi_type, res_ptr, count, mpi_type,
                           target, 0, count, mpi_type, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (int j = 0; j < count; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(p, target)) {
                SQUELCH(printf("%d->%d -- NOP_SELF(1)[%d]: expected %d, got %d\n",
                               target, rank, i, target, get_value(p)););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0 */
        memset(res_ptr, -1, type_size * nproc * count);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, mpi_type, res_ptr, count, mpi_type,
                           target, 0, count, mpi_type, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (int j = 0; j < count; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(p, target)) {
                SQUELCH(printf("%d->%d -- NOP_SELF(2)[%d]: expected %d, got %d\n",
                               target, rank, i, target, get_value(p)););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0, origin_dtype = NULL */
        memset(res_ptr, -1, type_size * nproc * count);

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, MPI_DATATYPE_NULL, res_ptr, count, mpi_type,
                           target, 0, count, mpi_type, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (int j = 0; j < count; j++) {
            void *p = (char *) res_ptr + j * type_size;
            if (cmp_value(p, target)) {
                SQUELCH(printf("%d->%d -- NOP_SELF(2)[%d]: expected %d, got %d\n",
                               target, rank, i, target, get_value(p)););
                errors++;
            }
        }
    }

    return errors;
}

int run(const char *arg)
{
    int i;
    int count, errors = 0;
    void *win_ptr, *res_ptr, *val_ptr;
    MPI_Win win;
    MPI_Datatype mpi_type = MPI_DATATYPE_NULL;

    MTestArgList *head = MTestArgListCreate_arg(arg);

    do_derived = MTestArgListGetInt_with_default(head, "derived", 0);

    const char *name = MTestArgListGetString_with_default(head, "type", "int");
    if (strcmp(name, "int") == 0) {
        mpi_base_type = MPI_INT;
        type_size = sizeof(int);
    } else if (strcmp(name, "short") == 0) {
        mpi_base_type = MPI_SHORT;
        type_size = sizeof(short);
    } else if (strcmp(name, "long") == 0) {
        mpi_base_type = MPI_LONG;
        type_size = sizeof(long);
    } else if (strcmp(name, "double") == 0) {
        mpi_base_type = MPI_DOUBLE;
        type_size = sizeof(double);
    } else {
        printf("type %s not recognized\n", name);
        assert(0);
    }

    MTestArgListDestroy(head);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    for (int k = 0; k < NUM_COUNT; k++) {
        count = test_counts[k];
        win_ptr = malloc(type_size * nproc * count);
        res_ptr = malloc(type_size * nproc * count);
        val_ptr = malloc(type_size * count);

        if (do_derived) {
            MPI_Type_contiguous(1, mpi_base_type, &mpi_type);
            MPI_Type_commit(&mpi_type);
        } else {
            mpi_type = mpi_base_type;
        }

        MPI_Win_create(win_ptr, type_size * nproc * count, type_size,
                       MPI_INFO_NULL, MPI_COMM_WORLD, &win);

        errors += test_self_communication(win_ptr, res_ptr, val_ptr, count, mpi_type, win);
        errors += test_neighbor_communication(win_ptr, res_ptr, val_ptr, count, mpi_type, win);
        errors += test_contention(win_ptr, res_ptr, val_ptr, count, mpi_type, win);
        errors += test_alltoall_fence(win_ptr, res_ptr, val_ptr, count, mpi_type, win);
        errors += test_alltoall_lockall(win_ptr, res_ptr, val_ptr, count, mpi_type, win);
        errors += test_alltoall_lockall_flush(win_ptr, res_ptr, val_ptr, count, mpi_type, win);
        errors += test_neighbor_noop(win_ptr, res_ptr, val_ptr, count, mpi_type, win);
        errors += test_self_noop(win_ptr, res_ptr, val_ptr, count, mpi_type, win);

        MPI_Win_free(&win);

        if (do_derived) {
            MPI_Type_free(&mpi_type);
        }

        free(win_ptr);
        free(res_ptr);
        free(val_ptr);
    }

    return errors;
}
