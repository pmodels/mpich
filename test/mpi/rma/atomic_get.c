/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This test is going to test atomic GET (GACC/FOP+MPI_NO_OP).
 *
 * There are totally three processes involved in this test. Both
 * rank 1 and rank 2 issue RMA operations to rank 0. Rank 2 issues
 * atomic PUT (ACC+MPI_REPLACE), whereas rank 1 issues atomic
 * GET (GACC/FOP+MPI_NO_OP). The datatype used in the test is
 * pair-type. The initial value of pair-type data in origin buffer
 * of atomic PUT is set so that the two basic values equal with
 * each other. Due to the atomicity of GET, the expected resulting
 * data should also have equivalent basic values. */

#include "mpitest.h"
#include <assert.h>

#ifdef MULTI_TESTS
#define run rma_atomic_get
int run(const char *arg);
#endif

#define LOOP 100
#define DATA_SIZE 100
#define OPS_NUM 10000
#define GACC_SZ 10

struct int_pair {
    int a;
    int b;
};

struct long_pair {
    long a;
    int b;
};

struct short_pair {
    short a;
    int b;
};

struct float_pair {
    float a;
    int b;
};

struct double_pair {
    double a;
    int b;
};

struct long_double_pair {
    long double a;
    int b;
};

static void set_value(void *buf, MPI_Datatype pairtype_dt, long a, int b)
{
    if (pairtype_dt == MPI_2INT) {
        struct int_pair *p = buf;
        p->a = (int) a;
        p->b = b;
    } else if (pairtype_dt == MPI_LONG_INT) {
        struct long_pair *p = buf;
        p->a = (long) a;
        p->b = b;
    } else if (pairtype_dt == MPI_SHORT_INT) {
        struct short_pair *p = buf;
        p->a = (short) a;
        p->b = b;
    } else if (pairtype_dt == MPI_FLOAT_INT) {
        struct float_pair *p = buf;
        p->a = (float) a;
        p->b = b;
    } else if (pairtype_dt == MPI_DOUBLE_INT) {
        struct double_pair *p = buf;
        p->a = (double) a;
        p->b = b;
    } else if (pairtype_dt == MPI_LONG_DOUBLE_INT) {
        struct long_double_pair *p = buf;
        p->a = (long double) a;
        p->b = b;
    } else {
        assert(0);
    }
}

static void print_value(void *buf, MPI_Datatype pairtype_dt)
{
    if (pairtype_dt == MPI_2INT) {
        struct int_pair *p = buf;
        printf("a: %d, b: %d", p->a, p->b);
    } else if (pairtype_dt == MPI_LONG_INT) {
        struct long_pair *p = buf;
        printf("a: %ld, b: %d", p->a, p->b);
    } else if (pairtype_dt == MPI_SHORT_INT) {
        struct short_pair *p = buf;
        printf("a: %hd, b: %d", p->a, p->b);
    } else if (pairtype_dt == MPI_FLOAT_INT) {
        struct float_pair *p = buf;
        printf("a: %f, b: %d", p->a, p->b);
    } else if (pairtype_dt == MPI_DOUBLE_INT) {
        struct double_pair *p = buf;
        printf("a: %lf, b: %d", p->a, p->b);
    } else if (pairtype_dt == MPI_LONG_DOUBLE_INT) {
        struct long_double_pair *p = buf;
        printf("a: %Lf, b: %d", p->a, p->b);
    }
}

/* checking whether the two fields in the pairtype is equal due from atomicity.
 * NOTE: the test will overflow the `short` type, and we assume the check still
 * hold after cast.
 */
static int check_value(void *buf, MPI_Datatype pairtype_dt)
{
    if (pairtype_dt == MPI_2INT) {
        struct int_pair *p = buf;
        return p->a == (int) p->b;
    } else if (pairtype_dt == MPI_LONG_INT) {
        struct long_pair *p = buf;
        return p->a == (long) p->b;
    } else if (pairtype_dt == MPI_SHORT_INT) {
        struct short_pair *p = buf;
        return p->a == (short) p->b;
    } else if (pairtype_dt == MPI_FLOAT_INT) {
        struct float_pair *p = buf;
        return p->a == (float) p->b;
    } else if (pairtype_dt == MPI_DOUBLE_INT) {
        struct double_pair *p = buf;
        return p->a == (double) p->b;
    } else if (pairtype_dt == MPI_LONG_DOUBLE_INT) {
        struct long_double_pair *p = buf;
        return p->a == (long double) p->b;
    }
    return -1;
}

int run(const char *arg)
{
    int rank, nproc;
    int i, j, k;
    int errs = 0, curr_errors = 0;
    MPI_Win win;
    void *tar_buf = NULL;
    void *orig_buf = NULL;
    void *result_buf = NULL;

    int pairtype_size;
    MPI_Datatype pairtype_dt;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    char *name = MTestArgListGetString(head, "pairtype");
    if (strcmp(name, "int") == 0) {
        pairtype_dt = MPI_2INT;
        pairtype_size = sizeof(struct int_pair);
    } else if (strcmp(name, "long") == 0) {
        pairtype_dt = MPI_LONG_INT;
        pairtype_size = sizeof(struct long_pair);
    } else if (strcmp(name, "short") == 0) {
        pairtype_dt = MPI_SHORT_INT;
        pairtype_size = sizeof(struct short_pair);
    } else if (strcmp(name, "float") == 0) {
        pairtype_dt = MPI_FLOAT_INT;
        pairtype_size = sizeof(struct float_pair);
    } else if (strcmp(name, "double") == 0) {
        pairtype_dt = MPI_DOUBLE_INT;
        pairtype_size = sizeof(struct double_pair);
    } else if (strcmp(name, "long_double") == 0) {
        pairtype_dt = MPI_LONG_DOUBLE_INT;
        pairtype_size = sizeof(struct long_double_pair);
    } else {
        assert(0);
    }
    MTestArgListDestroy(head);

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    /* This test needs to work with 3 processes. */
    assert(nproc >= 3);

    MPI_Alloc_mem(pairtype_size * DATA_SIZE, MPI_INFO_NULL, &orig_buf);
    MPI_Alloc_mem(pairtype_size * DATA_SIZE, MPI_INFO_NULL, &result_buf);

    MPI_Win_allocate(pairtype_size * DATA_SIZE, pairtype_size,
                     MPI_INFO_NULL, MPI_COMM_WORLD, &tar_buf, &win);

    for (j = 0; j < LOOP * 6; j++) {

        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, rank, 0, win);

        /* initialize data */
        for (i = 0; i < DATA_SIZE; i++) {
            set_value((char *) tar_buf + pairtype_size * i, pairtype_dt, 0, 0);
            set_value((char *) result_buf + pairtype_size * i, pairtype_dt, 0, 0);
        }

        MPI_Win_unlock(rank, win);

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Win_fence(0, win);

        if (rank == 2) {
            if (j < 2 * LOOP) {
                /* Work with FOP test (Test #1 to Test #2) */
                for (i = 0; i < OPS_NUM; i++) {

                    int curr_val = j * OPS_NUM + i;
                    set_value(orig_buf, pairtype_dt, curr_val, curr_val);

                    MPI_Accumulate(orig_buf, 1, pairtype_dt,
                                   0, 0, 1, pairtype_dt, MPI_REPLACE, win);
                }
            } else {
                /* Work with GACC test (Test #3 to Test #6) */
                for (i = 0; i < OPS_NUM / GACC_SZ; i++) {

                    for (k = 0; k < GACC_SZ; k++) {
                        int curr_val = j * OPS_NUM + i * GACC_SZ + k;
                        void *p = (char *) orig_buf + pairtype_size * k;
                        set_value(p, pairtype_dt, curr_val, curr_val);
                    }

                    MPI_Accumulate(orig_buf, GACC_SZ, pairtype_dt,
                                   0, 0, GACC_SZ, pairtype_dt, MPI_REPLACE, win);
                }
            }
        } else if (rank == 1) {
            /* equals to an atomic GET */
            if (j < LOOP) {
                for (i = 0; i < DATA_SIZE; i++) {
                    /* Test #1: FOP + MPI_NO_OP */
                    void *p = (char *) result_buf + pairtype_size * i;
                    MPI_Fetch_and_op(orig_buf, p, pairtype_dt, 0, 0, MPI_NO_OP, win);
                }
            } else if (j < 2 * LOOP) {
                for (i = 0; i < DATA_SIZE; i++) {
                    /* Test #2: FOP + MPI_NO_OP + NULL origin buffer address */
                    void *p = (char *) result_buf + pairtype_size * i;
                    MPI_Fetch_and_op(NULL, p, pairtype_dt, 0, 0, MPI_NO_OP, win);
                }
            } else if (j < 3 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #3: GACC + MPI_NO_OP */
                    void *p = (char *) result_buf + pairtype_size * (i * GACC_SZ);
                    MPI_Get_accumulate(orig_buf, GACC_SZ, pairtype_dt,
                                       p, GACC_SZ, pairtype_dt,
                                       0, 0, GACC_SZ, pairtype_dt, MPI_NO_OP, win);
                }
            } else if (j < 4 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #4: GACC + MPI_NO_OP + NULL origin buffer address */
                    void *p = (char *) result_buf + pairtype_size * (i * GACC_SZ);
                    MPI_Get_accumulate(NULL, GACC_SZ, pairtype_dt,
                                       p, GACC_SZ, pairtype_dt,
                                       0, 0, GACC_SZ, pairtype_dt, MPI_NO_OP, win);
                }
            } else if (j < 5 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #5: GACC + MPI_NO_OP + zero origin count */
                    void *p = (char *) result_buf + pairtype_size * (i * GACC_SZ);
                    MPI_Get_accumulate(orig_buf, 0, pairtype_dt,
                                       p, GACC_SZ, pairtype_dt,
                                       0, 0, GACC_SZ, pairtype_dt, MPI_NO_OP, win);
                }
            } else if (j < 6 * LOOP) {
                for (i = 0; i < DATA_SIZE / GACC_SZ; i++) {
                    /* Test #5: GACC + MPI_NO_OP + NULL origin datatype */
                    void *p = (char *) result_buf + pairtype_size * (i * GACC_SZ);
                    MPI_Get_accumulate(orig_buf, GACC_SZ, MPI_DATATYPE_NULL,
                                       p, GACC_SZ, pairtype_dt,
                                       0, 0, GACC_SZ, pairtype_dt, MPI_NO_OP, win);
                }
            }
        }

        MPI_Win_fence(0, win);

        /* check results */
        if (rank == 1) {
            for (i = 0; i < DATA_SIZE; i++) {
                void *p = (char *) result_buf + pairtype_size * i;
                if (!check_value(p, pairtype_dt)) {
                    if (curr_errors < 10) {
                        printf("result_buf check eroor: LOOP %d, i %d: ", j, i);
                        print_value(p, pairtype_dt);
                        printf("\n");
                    }
                    curr_errors++;
                }
            }
        }

        if (j % LOOP == 0) {
            errs += curr_errors;
            curr_errors = 0;
        }
    }

    MPI_Win_free(&win);

    MPI_Free_mem(orig_buf);
    MPI_Free_mem(result_buf);

    return errs;
}
