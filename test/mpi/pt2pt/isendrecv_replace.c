/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_isendrecv_replace
int run(const char *arg);
#endif

/* Similar test as isendirecv.c, but use MPI_Isendrecv_replace */

static int errs = 0;
static int elems = 20;
static int rank, nproc, dest;
static float *buf;
static MPI_Comm comm;
static MPI_Request *reqs;
/* stride > 1 sends each block as a non-contiguous vector type */
static int stride = 1;
static int count;
static MPI_Datatype dtype;

#define IDX(i, j) (((i) * elems + (j)) * stride)

static void test_sendrecv(int offset)
{
    /* the gaps between the elements must stay untouched */
    for (int i = 0; i < nproc * elems * stride; i++) {
        buf[i] = -1.0;
    }
    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            buf[IDX(i, j)] = rank * 100.0 + j;
        }
    }

    for (int i = 0; i < nproc; i++) {
        int j = (i + offset) % nproc;
        MPI_Isendrecv_replace(&buf[IDX(j, 0)], count, dtype, i, 0, j, 0, comm, &reqs[i]);
    }

    MPI_Waitall(nproc, reqs, MPI_STATUSES_IGNORE);

    for (int i = 0; i < nproc * elems * stride; i++) {
        float expect = -1.0;
        if (i % stride == 0) {
            expect = (i / stride / elems) * 100.0 + (i / stride) % elems;
        }
        if (buf[i] != expect) {
            if (errs < 10) {
                fprintf(stderr, "buf[%d] mismatch, got %f, expect %f\n", i, buf[i], expect);
            }
            errs++;
        }
    }
}

/* sendrecv may use the same or different source and destination,
 * offset defines the offset between them */

int run(const char *arg)
{
    MTestArgList *head = MTestArgListCreate_arg(arg);
    stride = MTestArgListGetInt_with_default(head, "stride", 1);
    MTestArgListDestroy(head);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    reqs = malloc(nproc * sizeof(MPI_Request));
    buf = malloc(elems * nproc * stride * sizeof(float));

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
    free(buf);

    return errs;
}
