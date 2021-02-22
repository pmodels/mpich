/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdlib.h>
#include <stdio.h>

/* Gather data from a vector to contiguous */

int main(int argc, char **argv)
{
    MPI_Datatype vec;
    MPI_Comm comm;
    double *vecin, *vecout;
    int minsize = 2, count;
    int root, i, j, n, stride, errs = 0;
    int rank, size;

    MPI_Request req;
    MPI_Info info;

    MTest_Init(&argc, &argv);

    MPI_Info_create(&info);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        for (root = 0; root < size; root++) {
            for (count = 1; count < 65000; count = count * 2) {
                n = 12;
                stride = 10;
                vecin = (double *) malloc(n * stride * size * sizeof(double));
                vecout = (double *) malloc(size * n * sizeof(double));

                MPI_Type_vector(n, 1, stride, MPI_DOUBLE, &vec);
                MPI_Type_commit(&vec);

                for (i = 0; i < n * stride; i++)
                    vecin[i] = -2;
                for (i = 0; i < n; i++)
                    vecin[i * stride] = rank * n + i;

                MPI_Gather_init(vecin, 1, vec, vecout, n, MPI_DOUBLE, root, comm, info, &req);
                for (j = 0; j < 10; ++j) {
                    MPI_Start(&req);
                    MPI_Wait(&req, MPI_STATUS_IGNORE);
                    if (rank == root) {
                        for (i = 0; i < n * size; i++) {
                            if (vecout[i] != i) {
                                errs++;
                                if (errs < 10) {
                                    fprintf(stderr, "vecout[%d]=%d\n", i, (int) vecout[i]);
                                }
                            }
                        }
                    }
                }
                MPI_Request_free(&req);

                MPI_Type_free(&vec);
                free(vecin);
                free(vecout);
            }
        }
        MTestFreeComm(&comm);
    }

    /* do a zero length gather */
    MPI_Gather_init(NULL, 0, MPI_BYTE, NULL, 0, MPI_BYTE, 0, MPI_COMM_WORLD, info, &req);
    for (i = 0; i < 10; ++i) {
        MPI_Start(&req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    }
    MPI_Request_free(&req);

    MPI_Info_free(&info);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
