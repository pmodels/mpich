/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "mpitest.h"
#include "squelch.h"

#define ITER  100
#define COUNT 5

#if defined (GACC_TYPE_SHORT)
#  define TYPE_C   short
#  define TYPE_MPI_BASE MPI_SHORT
#  define TYPE_FMT "%d"
#elif defined (GACC_TYPE_LONG)
#  define TYPE_C   long
#  define TYPE_MPI_BASE MPI_LONG
#  define TYPE_FMT "%ld"
#elif defined (GACC_TYPE_DOUBLE)
#  define TYPE_C   double
#  define TYPE_MPI_BASE MPI_DOUBLE
#  define TYPE_FMT "%f"
#else
#  define TYPE_C   int
#  define TYPE_MPI_BASE MPI_INT
#  define TYPE_FMT "%d"
#endif

#if defined(GACC_TYPE_DERIVED)
#  define TYPE_MPI derived_type
#else
#  define TYPE_MPI TYPE_MPI_BASE
#endif

void reset_bufs(TYPE_C * win_ptr, TYPE_C * res_ptr, TYPE_C * val_ptr, TYPE_C value, MPI_Win win)
{
    int rank, nproc, i;

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    memset(win_ptr, 0, sizeof(TYPE_C) * nproc * COUNT);
    MPI_Win_unlock(rank, win);

    memset(res_ptr, -1, sizeof(TYPE_C) * nproc * COUNT);

    for (i = 0; i < COUNT; i++)
        val_ptr[i] = value;

    MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
    int i, rank, nproc;
    int errors = 0, all_errors = 0;
    TYPE_C *win_ptr, *res_ptr, *val_ptr;
    MPI_Win win;
#if defined (GACC_TYPE_DERIVED)
    MPI_Datatype derived_type;
#endif

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    win_ptr = malloc(sizeof(TYPE_C) * nproc * COUNT);
    res_ptr = malloc(sizeof(TYPE_C) * nproc * COUNT);
    val_ptr = malloc(sizeof(TYPE_C) * COUNT);

#if defined (GACC_TYPE_DERIVED)
    MPI_Type_contiguous(1, TYPE_MPI_BASE, &derived_type);
    MPI_Type_commit(&derived_type);
#endif

    MPI_Win_create(win_ptr, sizeof(TYPE_C) * nproc * COUNT, sizeof(TYPE_C),
                   MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    /* Test self communication */

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, win);

    for (i = 0; i < ITER; i++) {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
        MPI_Get_accumulate(val_ptr, COUNT, TYPE_MPI, res_ptr, COUNT, TYPE_MPI,
                           rank, 0, COUNT, TYPE_MPI, MPI_SUM, win);
        MPI_Win_unlock(rank, win);
    }

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < COUNT; i++) {
        if (win_ptr[i] != ITER) {
            SQUELCH(printf("%d->%d -- SELF[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                           rank, rank, i, (TYPE_C) ITER, win_ptr[i]););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test neighbor communication */

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, win);

    for (i = 0; i < ITER; i++) {
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, (rank + 1) % nproc, 0, win);
        MPI_Get_accumulate(val_ptr, COUNT, TYPE_MPI, res_ptr, COUNT, TYPE_MPI,
                           (rank + 1) % nproc, 0, COUNT, TYPE_MPI, MPI_SUM, win);
        MPI_Win_unlock((rank + 1) % nproc, win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < COUNT; i++) {
        if (win_ptr[i] != ITER) {
            SQUELCH(printf("%d->%d -- NEIGHBOR[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                           (rank + 1) % nproc, rank, i, (TYPE_C) ITER, win_ptr[i]););
            errors++;
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test contention */

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, win);

    if (rank != 0) {
        for (i = 0; i < ITER; i++) {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
            MPI_Get_accumulate(val_ptr, COUNT, TYPE_MPI, res_ptr, COUNT, TYPE_MPI,
                               0, 0, COUNT, TYPE_MPI, MPI_SUM, win);
            MPI_Win_unlock(0, win);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    if (rank == 0 && nproc > 1) {
        for (i = 0; i < COUNT; i++) {
            if (win_ptr[i] != ITER * (nproc - 1)) {
                SQUELCH(printf("*->%d - CONTENTION[%d]: expected=" TYPE_FMT " val=" TYPE_FMT "\n",
                               rank, i, (TYPE_C) ITER * (nproc - 1), win_ptr[i]););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test all-to-all communication (fence) */

    reset_bufs(win_ptr, res_ptr, val_ptr, rank, win);

    for (i = 0; i < ITER; i++) {
        int j;

        MPI_Win_fence(MPI_MODE_NOPRECEDE, win);
        for (j = 0; j < nproc; j++) {
            MPI_Get_accumulate(val_ptr, COUNT, TYPE_MPI, &res_ptr[j * COUNT], COUNT, TYPE_MPI,
                               j, rank * COUNT, COUNT, TYPE_MPI, MPI_SUM, win);
        }
        MPI_Win_fence(MPI_MODE_NOSUCCEED, win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (j = 0; j < nproc; j++) {
            int c;
            for (c = 0; c < COUNT; c++) {
                if (res_ptr[j * COUNT + c] != i * rank) {
                    SQUELCH(printf
                            ("%d->%d -- ALL-TO-ALL (FENCE) [%d]: iter %d, expected result " TYPE_FMT
                             ", got " TYPE_FMT "\n", rank, j, c, i, (TYPE_C) i * rank,
                             res_ptr[j * COUNT + c]););
                    errors++;
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        int c;
        for (c = 0; c < COUNT; c++) {
            if (win_ptr[i * COUNT + c] != ITER * i) {
                SQUELCH(printf
                        ("%d->%d -- ALL-TO-ALL (FENCE): expected " TYPE_FMT ", got " TYPE_FMT "\n",
                         i, rank, (TYPE_C) ITER * i, win_ptr[i * COUNT + c]););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test all-to-all communication (lock-all) */

    reset_bufs(win_ptr, res_ptr, val_ptr, rank, win);

    for (i = 0; i < ITER; i++) {
        int j;

        MPI_Win_lock_all(0, win);
        for (j = 0; j < nproc; j++) {
            MPI_Get_accumulate(val_ptr, COUNT, TYPE_MPI, &res_ptr[j * COUNT], COUNT, TYPE_MPI,
                               j, rank * COUNT, COUNT, TYPE_MPI, MPI_SUM, win);
        }
        MPI_Win_unlock_all(win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (j = 0; j < nproc; j++) {
            int c;
            for (c = 0; c < COUNT; c++) {
                if (res_ptr[j * COUNT + c] != i * rank) {
                    SQUELCH(printf
                            ("%d->%d -- ALL-TO-ALL (LOCK-ALL) [%d]: iter %d, expected result "
                             TYPE_FMT ", got " TYPE_FMT "\n", rank, j, c, i, (TYPE_C) i * rank,
                             res_ptr[j * COUNT + c]););
                    errors++;
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        int c;
        for (c = 0; c < COUNT; c++) {
            if (win_ptr[i * COUNT + c] != ITER * i) {
                SQUELCH(printf
                        ("%d->%d -- ALL-TO-ALL (LOCK-ALL): expected " TYPE_FMT ", got " TYPE_FMT
                         "\n", i, rank, (TYPE_C) ITER * i, win_ptr[i * COUNT + c]););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test all-to-all communication (lock-all+flush) */

    reset_bufs(win_ptr, res_ptr, val_ptr, rank, win);

    for (i = 0; i < ITER; i++) {
        int j;

        MPI_Win_lock_all(0, win);
        for (j = 0; j < nproc; j++) {
            MPI_Get_accumulate(val_ptr, COUNT, TYPE_MPI, &res_ptr[j * COUNT], COUNT, TYPE_MPI,
                               j, rank * COUNT, COUNT, TYPE_MPI, MPI_SUM, win);
            MPI_Win_flush(j, win);
        }
        MPI_Win_unlock_all(win);
        MPI_Barrier(MPI_COMM_WORLD);

        for (j = 0; j < nproc; j++) {
            int c;
            for (c = 0; c < COUNT; c++) {
                if (res_ptr[j * COUNT + c] != i * rank) {
                    SQUELCH(printf
                            ("%d->%d -- ALL-TO-ALL (LOCK-ALL+FLUSH) [%d]: iter %d, expected result "
                             TYPE_FMT ", got " TYPE_FMT "\n", rank, j, c, i, (TYPE_C) i * rank,
                             res_ptr[j * COUNT + c]););
                    errors++;
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < nproc; i++) {
        int c;
        for (c = 0; c < COUNT; c++) {
            if (win_ptr[i * COUNT + c] != ITER * i) {
                SQUELCH(printf
                        ("%d->%d -- ALL-TO-ALL (LOCK-ALL+FLUSH): expected " TYPE_FMT ", got "
                         TYPE_FMT "\n", i, rank, (TYPE_C) ITER * i, win_ptr[i * COUNT + c]););
                errors++;
            }
        }
    }
    MPI_Win_unlock(rank, win);

    /* Test NO_OP (neighbor communication) */

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, win);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < COUNT * nproc; i++)
        win_ptr[i] = (TYPE_C) rank;
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < ITER; i++) {
        int j, target = (rank + 1) % nproc;

        /* Test: origin_buf = NULL */
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, COUNT, TYPE_MPI, res_ptr, COUNT, TYPE_MPI,
                           target, 0, COUNT, TYPE_MPI, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (j = 0; j < COUNT; j++) {
            if (res_ptr[j] != (TYPE_C) target) {
                SQUELCH(printf("%d->%d -- NOP(1)[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                               target, rank, i, (TYPE_C) target, res_ptr[i]););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0 */
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, TYPE_MPI, res_ptr, COUNT, TYPE_MPI,
                           target, 0, COUNT, TYPE_MPI, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (j = 0; j < COUNT; j++) {
            if (res_ptr[j] != (TYPE_C) target) {
                SQUELCH(printf("%d->%d -- NOP(2)[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                               target, rank, i, (TYPE_C) target, res_ptr[i]););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0, origin_dtype = NULL */
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, MPI_DATATYPE_NULL, res_ptr, COUNT, TYPE_MPI,
                           target, 0, COUNT, TYPE_MPI, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (j = 0; j < COUNT; j++) {
            if (res_ptr[j] != (TYPE_C) target) {
                SQUELCH(printf("%d->%d -- NOP(2)[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                               target, rank, i, (TYPE_C) target, res_ptr[i]););
                errors++;
            }
        }
    }

    /* Test NO_OP (self communication) */

    reset_bufs(win_ptr, res_ptr, val_ptr, 1, win);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);
    for (i = 0; i < COUNT * nproc; i++)
        win_ptr[i] = (TYPE_C) rank;
    MPI_Win_unlock(rank, win);
    MPI_Barrier(MPI_COMM_WORLD);

    for (i = 0; i < ITER; i++) {
        int j, target = rank;

        /* Test: origin_buf = NULL */
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, COUNT, TYPE_MPI, res_ptr, COUNT, TYPE_MPI,
                           target, 0, COUNT, TYPE_MPI, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (j = 0; j < COUNT; j++) {
            if (res_ptr[j] != (TYPE_C) target) {
                SQUELCH(printf
                        ("%d->%d -- NOP_SELF(1)[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                         target, rank, i, (TYPE_C) target, res_ptr[i]););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0 */
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, TYPE_MPI, res_ptr, COUNT, TYPE_MPI,
                           target, 0, COUNT, TYPE_MPI, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (j = 0; j < COUNT; j++) {
            if (res_ptr[j] != (TYPE_C) target) {
                SQUELCH(printf
                        ("%d->%d -- NOP_SELF(2)[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                         target, rank, i, (TYPE_C) target, res_ptr[i]););
                errors++;
            }
        }

        /* Test: origin_buf = NULL, origin_count = 0, origin_dtype = NULL */
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target, 0, win);
        MPI_Get_accumulate(NULL, 0, MPI_DATATYPE_NULL, res_ptr, COUNT, TYPE_MPI,
                           target, 0, COUNT, TYPE_MPI, MPI_NO_OP, win);
        MPI_Win_unlock(target, win);

        for (j = 0; j < COUNT; j++) {
            if (res_ptr[j] != (TYPE_C) target) {
                SQUELCH(printf
                        ("%d->%d -- NOP_SELF(2)[%d]: expected " TYPE_FMT ", got " TYPE_FMT "\n",
                         target, rank, i, (TYPE_C) target, res_ptr[i]););
                errors++;
            }
        }
    }

    MPI_Win_free(&win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

#if defined (GACC_TYPE_DERIVED)
    MPI_Type_free(&derived_type);
#endif

    free(win_ptr);
    free(res_ptr);
    free(val_ptr);

    MPI_Finalize();

    return 0;
}
