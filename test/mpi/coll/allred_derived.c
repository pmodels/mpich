/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include <assert.h>

/*
static char MTEST_Descrip[] = "Test MPI_Allreduce with commutative user-defined operations";
*/

#define MAX_BLOCKLEN 10000
#define IS_COMMUTATIVE 1

/* We make the error count global so that we can easily control the output
   of error information (in particular, limiting it after the first 10
   errors */
int errs = 0;

/* parameter for a vector type of MPI_INT */
int g_blocklen = 1;
int g_stride = 1;

void uop(void *, void *, int *, MPI_Datatype *);
void uop(void *cinPtr, void *coutPtr, int *count, MPI_Datatype * dtype)
{
    const int *cin = (const int *) cinPtr;
    int *cout = (int *) coutPtr;

    int k = 0;
    for (int i = 0; i < *count; i++) {
        for (int j = 0; j < g_blocklen; j++) {
            cout[k] += cin[k];
            k += g_stride;
        }
    }
}

static void init_buf(void *buf, int count, int rank)
{
    int *p = buf;

    int k = 0;
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < g_blocklen; j++) {
            p[k] = rank + i + j;
            k += g_stride;
        }
    }
}

static int check_result(void *buf, int count, int size)
{
    int lerrs = 0;
    int *p = buf;

    int k = 0;
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < g_blocklen; j++) {
            int exp = size * (size - 1) / 2 + (i + j) * size;
            if (p[k] != exp) {
                lerrs++;
                if (errs + lerrs < 10) {
                    printf("[%d - %d] expected %d, got %d, %s\n",
                           i, j, exp, p[k], MTestGetIntracommName());
                }
            }
            k += g_stride;
        }
    }
    return lerrs;
}

int main(int argc, char *argv[])
{
    MPI_Comm comm;
    void *buf, *bufout;
    MPI_Op op;
    MPI_Datatype datatype;

    MTest_Init(&argc, &argv);

    MPI_Op_create(uop, IS_COMMUTATIVE, &op);

    while (MTestGetIntracommGeneral(&comm, 2, 1)) {
        if (comm == MPI_COMM_NULL) {
            continue;
        }

        int rank, size;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        int count = 10;
        for (int n = 1; n < MAX_BLOCKLEN; n *= 2) {
            g_blocklen = n;
            int extent = g_blocklen * g_stride * sizeof(int);
            MPI_Type_vector(g_blocklen, 1, g_stride, MPI_INT, &datatype);
            MPI_Type_commit(&datatype);

            buf = (int *) malloc(extent * count);
            if (!buf) {
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
            bufout = (int *) malloc(extent * count);
            if (!bufout) {
                MPI_Abort(MPI_COMM_WORLD, 1);
            }

            init_buf(buf, count, rank);
            MPI_Allreduce(buf, bufout, count, datatype, op, comm);
            errs += check_result(bufout, count, size);

            /* do it again using MPI_IN_PLACE */
            init_buf(bufout, count, rank);
            MPI_Allreduce(MPI_IN_PLACE, bufout, count, datatype, op, comm);
            errs += check_result(bufout, count, size);

            free(buf);
            free(bufout);
            MPI_Type_free(&datatype);
        }

        MTestFreeComm(&comm);
    }

    MPI_Op_free(&op);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
