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

/* sendrecv may use the same or different source and destination,
 * offset defines the offset between them */
static void test_sendrecv(int offset)
{
    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            in_buf[i * elems + j] = -1.0;
            out_buf[i * elems + j] = rank * 10000.0 + i * 100.0 + j;
        }
    }

    for (int i = 0; i < nproc; i++) {
        int j = (i + offset) % nproc;
        MPI_Isendrecv(&out_buf[elems * i], elems, MPI_FLOAT, i, 0,
                      &in_buf[elems * j], elems, MPI_FLOAT, j, 0, comm, &reqs[i]);
    }

    MPI_Waitall(nproc, reqs, MPI_STATUSES_IGNORE);

    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            if (in_buf[i * elems + j] != i * 10000.0 + rank * 100.0 + j) {
                errs++;
            }
        }
    }
}

int run(const char *arg)
{
    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    reqs = malloc(nproc * sizeof(MPI_Request));
    in_buf = malloc(elems * nproc * sizeof(float));
    out_buf = malloc(elems * nproc * sizeof(float));

    for (int offset = 0; offset < nproc - 1; offset++) {
        test_sendrecv(offset);
    }

    free(reqs);
    free(in_buf);
    free(out_buf);

    return errs;
}
