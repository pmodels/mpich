/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdbool.h>
#include <string.h>

#ifdef MULTI_TESTS
#define run coll_red_negextent
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test reduction collectives with a negative-extent datatype";
*/

/* Each element is one int at byte offset sizeof(int), and the extent is -3 ints, so
 * element i lies at int index 1 - 3 * i from the buffer pointer. Temporary buffers
 * inside the collectives must hold elements 1 to count-1 below element 0. */
#define STRIDE 3
#define ELEM(buf, i) (((int *) (buf))[1 - STRIDE * (i)])
#define GUARD -1

static int errs;
/* the collective to test, from -coll=, or "all" */
static const char *g_coll;

static bool want(const char *name)
{
    return strcmp(g_coll, "all") == 0 || strcmp(g_coll, name) == 0;
}

static void uop(void *invec, void *inoutvec, int *count, MPI_Datatype * dtype)
{
    for (int i = 0; i < *count; i++) {
        ELEM(inoutvec, i) += ELEM(invec, i);
    }
}

/* Returns the buffer pointer, and the allocated array in *base */
static void *alloc_buf(int count, int **base)
{
    if (count == 0) {
        count = 1;
    }
    *base = malloc(STRIDE * count * sizeof(int));
    if (!*base) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    for (int i = 0; i < STRIDE * count; i++) {
        (*base)[i] = GUARD;
    }
    return *base + STRIDE * (count - 1);
}

/* element i on rank r holds r + i */
static void init_buf(void *buf, int count, int rank, int offset)
{
    for (int i = 0; i < count; i++) {
        ELEM(buf, i) = rank + offset + i;
    }
}

/* expected value of element i: sum of (q + i) over ranks q in [0, nranks) */
static int check_buf(const char *name, void *buf, int *base, int count, int nranks, int offset)
{
    int lerrs = 0;
    for (int i = 0; i < count; i++) {
        int exp = nranks * (nranks - 1) / 2 + (offset + i) * nranks;
        if (ELEM(buf, i) != exp) {
            lerrs++;
            if (errs + lerrs < 10) {
                printf("%s: [%d] expected %d, got %d, %s\n", name, i, exp, ELEM(buf, i),
                       MTestGetIntracommName());
            }
        }
    }
    /* the gaps between elements must be untouched */
    for (int i = 0; i < STRIDE * count; i++) {
        if (i % STRIDE != 1 && base[i] != GUARD) {
            lerrs++;
            if (errs + lerrs < 10) {
                printf("%s: gap at %d overwritten with %d, %s\n", name, i, base[i],
                       MTestGetIntracommName());
            }
        }
    }
    return lerrs;
}

static void test_comm(MPI_Comm comm, MPI_Datatype type, MPI_Op op, int count)
{
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int *sbase, *rbase;
    void *sbuf = alloc_buf(count * size, &sbase);
    void *rbuf;

    /* reduce to the first and the last rank, so a non-root rank 0 needs a temporary buffer */
    init_buf(sbuf, count, rank, 0);
    if (want("reduce")) {
        int roots[2] = { 0, size - 1 };
        for (int k = 0; k < 2; k++) {
            rbuf = alloc_buf(count, &rbase);
            MPI_Reduce(sbuf, rbuf, count, type, op, roots[k], comm);
            if (rank == roots[k]) {
                errs += check_buf("Reduce", rbuf, rbase, count, size, 0);
            }
            free(rbase);
        }
    }

    if (want("allreduce")) {
        rbuf = alloc_buf(count, &rbase);
        MPI_Allreduce(sbuf, rbuf, count, type, op, comm);
        errs += check_buf("Allreduce", rbuf, rbase, count, size, 0);
        free(rbase);
    }

    if (want("scan")) {
        rbuf = alloc_buf(count, &rbase);
        MPI_Scan(sbuf, rbuf, count, type, op, comm);
        errs += check_buf("Scan", rbuf, rbase, count, rank + 1, 0);
        free(rbase);
    }

    if (want("exscan")) {
        rbuf = alloc_buf(count, &rbase);
        MPI_Exscan(sbuf, rbuf, count, type, op, comm);
        if (rank > 0) {
            errs += check_buf("Exscan", rbuf, rbase, count, rank, 0);
        }
        free(rbase);
    }

    /* rank r receives the elements [r * count, (r + 1) * count) */
    init_buf(sbuf, count * size, rank, 0);
    if (want("reduce_scatter_block")) {
        rbuf = alloc_buf(count, &rbase);
        MPI_Reduce_scatter_block(sbuf, rbuf, count, type, op, comm);
        errs += check_buf("Reduce_scatter_block", rbuf, rbase, count, size, rank * count);
        free(rbase);
    }

    /* rank q receives count - q % 2 elements */
    if (want("reduce_scatter")) {
        int *recvcounts = malloc(size * sizeof(int));
        int disp = 0;
        for (int q = 0; q < size; q++) {
            recvcounts[q] = count - q % 2;
            if (q < rank) {
                disp += recvcounts[q];
            }
        }
        rbuf = alloc_buf(recvcounts[rank], &rbase);
        MPI_Reduce_scatter(sbuf, rbuf, recvcounts, type, op, comm);
        errs += check_buf("Reduce_scatter", rbuf, rbase, recvcounts[rank], size, disp);
        free(rbase);
        free(recvcounts);
    }

    free(sbase);
}

int run(const char *arg)
{
    MPI_Comm comm;
    MPI_Datatype base_type, type;
    MPI_Op op;

    errs = 0;
    MTestArgList *head = MTestArgListCreate_arg(arg);
    g_coll = MTestArgListGetString_with_default(head, "coll", "all");

    MPI_Aint disp = sizeof(int);
    MPI_Type_create_hindexed_block(1, 1, &disp, MPI_INT, &base_type);
    MPI_Type_create_resized(base_type, 0, -STRIDE * (MPI_Aint) sizeof(int), &type);
    MPI_Type_commit(&type);
    MPI_Type_free(&base_type);

    MPI_Op_create(uop, 1, &op);

    while (MTestGetIntracommGeneral(&comm, 1, 1)) {
        if (comm == MPI_COMM_NULL) {
            continue;
        }
        for (int count = 1; count <= 1024; count *= 4) {
            test_comm(comm, type, op, count);
        }
        MTestFreeComm(&comm);
    }

    MPI_Op_free(&op);
    MPI_Type_free(&type);
    MTestArgListDestroy(head);

    return errs;
}
