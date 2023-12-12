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

static void test_sendrecv(int offset)
{
    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            buf[i * elems + j] = rank * 100.0 + j;
        }
    }

    for (int i = 0; i < nproc; i++) {
        int j = (i + offset) % nproc;
        MPI_Isendrecv_replace(&buf[elems * j], elems, MPI_FLOAT, i, 0, j, 0, comm, &reqs[i]);
    }

    MPI_Waitall(nproc, reqs, MPI_STATUSES_IGNORE);

    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            if (buf[i * elems + j] != i * 100.0 + j) {
                if (errs < 10) {
                    fprintf(stderr, "buf(%d, %d) mismatch, got %f, expect %f\n", i, j,
                            buf[i * elems + j], (float) i);
                }
                errs++;
            }
        }
    }
}

/* sendrecv may use the same or different source and destination,
 * offset defines the offset between them */

int run(const char *arg)
{
    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    reqs = malloc(nproc * sizeof(MPI_Request));
    buf = malloc(elems * nproc * sizeof(float));

    for (int offset = 0; offset < nproc - 1; offset++) {
        test_sendrecv(offset);
    }

    free(reqs);
    free(buf);

    return errs;
}
