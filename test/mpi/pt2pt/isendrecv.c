/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_isendrecv
int run(const char *arg);
#endif

/* Similar test as isendirecv.c, but use MPI_Isendrecv */

static int errs = 0;
static int elems = 20;
static int rank, nproc, dest;
static MPI_Comm comm;
static float *in_buf, *out_buf;
static MPI_Request *reqs;
/* stride > 1 sends each block as a non-contiguous vector type */
static int stride = 1;
static int count;
static MPI_Datatype dtype;

#define IDX(i, j) (((i) * elems + (j)) * stride)

/* sendrecv may use the same or different source and destination,
 * offset defines the offset between them */
static void test_sendrecv(int offset)
{
    /* the gaps between the elements must stay untouched */
    for (int i = 0; i < nproc * elems * stride; i++) {
        in_buf[i] = -1.0;
        out_buf[i] = -1.0;
    }
    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            out_buf[IDX(i, j)] = rank * 10000.0 + i * 100.0 + j;
        }
    }

    for (int i = 0; i < nproc; i++) {
        int j = (i + offset) % nproc;
        MPI_Isendrecv(&out_buf[IDX(i, 0)], count, dtype, i, 0,
                      &in_buf[IDX(j, 0)], count, dtype, j, 0, comm, &reqs[i]);
    }

    MPI_Waitall(nproc, reqs, MPI_STATUSES_IGNORE);

    for (int i = 0; i < nproc * elems * stride; i++) {
        float expect = -1.0;
        if (i % stride == 0) {
            int blk = i / stride / elems;
            expect = blk * 10000.0 + rank * 100.0 + (i / stride) % elems;
        }
        if (in_buf[i] != expect) {
            errs++;
        }
    }
}

int run(const char *arg)
{
    MTestArgList *head = MTestArgListCreate_arg(arg);
    stride = MTestArgListGetInt_with_default(head, "stride", 1);
    MTestArgListDestroy(head);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    reqs = malloc(nproc * sizeof(MPI_Request));
    in_buf = malloc(elems * nproc * stride * sizeof(float));
    out_buf = malloc(elems * nproc * stride * sizeof(float));

    if (stride > 1) {
        count = 1;
        MPI_Type_vector(elems, 1, stride, MPI_FLOAT, &dtype);
        MPI_Type_commit(&dtype);
    } else {
        count = elems;
        dtype = MPI_FLOAT;
    }

    for (int offset = 0; offset < nproc - 1; offset++) {
        test_sendrecv(offset);
    }

    if (stride > 1) {
        MPI_Type_free(&dtype);
    }
    free(reqs);
    free(in_buf);
    free(out_buf);

    return errs;
}
