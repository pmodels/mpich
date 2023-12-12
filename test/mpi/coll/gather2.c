/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_gather2
int run(const char *arg);
#endif

/* Gather data from a vector to contiguous.  Use IN_PLACE */

int run(const char *arg)
{
    MPI_Datatype vec;
    double *vecin, *vecout;
    MPI_Comm comm;
    int count, minsize = 2;
    int root, i, n, stride, errs = 0;
    int rank, size;

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

                if (rank == root) {
                    for (i = 0; i < n; i++) {
                        vecout[rank * n + i] = rank * n + i;
                    }
                    MPI_Gather(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL,
                               vecout, n, MPI_DOUBLE, root, comm);
                } else {
                    MPI_Gather(vecin, 1, vec, NULL, -1, MPI_DATATYPE_NULL, root, comm);
                }
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
                MPI_Type_free(&vec);
                free(vecin);
                free(vecout);
            }
        }
        MTestFreeComm(&comm);
    }

    /* do a zero length gather */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Gather(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, NULL, 0, MPI_BYTE, 0, MPI_COMM_WORLD);
    } else {
        MPI_Gather(NULL, 0, MPI_BYTE, NULL, 0, MPI_BYTE, 0, MPI_COMM_WORLD);
    }

    return errs;
}
