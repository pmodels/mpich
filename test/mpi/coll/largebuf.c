#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
/*
 * Test case for collective operations with buffers with count 2^0 to 2^30.
 *
 */
int main(int argc, char *argv[])
{
    int rank;
    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Status status;
    unsigned int errs = 0;
    int datatypes[17] = { MPI_CHAR, MPI_SIGNED_CHAR, MPI_UNSIGNED_CHAR,
        MPI_BYTE, MPI_WCHAR, MPI_SHORT, MPI_UNSIGNED_SHORT, MPI_INT,
        MPI_UNSIGNED, MPI_LONG, MPI_UNSIGNED_LONG, MPI_FLOAT,
        MPI_DOUBLE, MPI_LONG_DOUBLE, MPI_LONG_LONG_INT,
        MPI_UNSIGNED_LONG_LONG, MPI_LONG_LONG
    };

    for (int type = 0; type < 17; type++) {
        for (int j = 1; j < 31; j++) {
            int size = 1 << j;
            MPI_Request request;
            int rc;
            int *number = (int *) malloc(sizeof(int) * size);
            int *sol = (int *) malloc(sizeof(int) * size);

            if (rank == 0) {
                for (int i = 0; i < size; i++) {
                    number[i] = i;
                }
            }
            for (int i = 0; i < size; i++) {
                sol[i] = i;
            }

            rc = MPI_Bcast(number, size, datatypes[type], 0, MPI_COMM_WORLD);
            if (rc) {
                errs++;
            }
            else {
                for (int i = 0; i < size; i++) {
                    if (number[i] != sol[i]) {
                        errs++;
                    }
                }
            }
            free(number);
        }
    }

    for (int type = 0; type < 17; type++) {
        for (int j = 1; j < 31; j++) {
            int size = 1 << j;
            MPI_Request request;
            int rc, wc;
            int *number = (int *) malloc(sizeof(int) * size);
            int *sol = (int *) malloc(sizeof(int) * size);
            if (rank == 0) {
                for (int i = 0; i < size; i++) {
                    number[i] = i;
                }
            }
            for (int i = 0; i < size; i++) {
                sol[i] = i;
            }
            rc = MPI_Ibcast(number, size, datatypes[type], 0, MPI_COMM_WORLD, &request);
            wc = MPI_Wait(&request, &status);
            if (rc || wc) {
                errs++;
            }
            else {
                for (int i = 0; i < size; i++) {
                    if (number[i] != sol[i]) {
                        errs++;
                    }
                }
            }
            free(number);
        }
    }

    for (int type = 0; type < 17; type++) {
        for (int j = 1; j < 31; j++) {
            int sendbufsize = 1 << j;
            int rc;
            MPI_Request request;
            int *sendbuf = (int *) malloc(sizeof(int) * sendbufsize);
            int *recvbuf = (int *) malloc(sizeof(int) * sendbufsize);
            int *sol = (int *) malloc(sizeof(int) * sendbufsize);
            int sum = 0;
            for (int i = 0; i < world_size; i++) {
                sum += i;
            }
            for (int i = 0; i < sendbufsize; i++) {
                sendbuf[i] = rank;
                sol[i] = sum;
            }
            rc = MPI_Allreduce(sendbuf, recvbuf, sendbufsize, datatypes[type],
                               MPI_SUM, MPI_COMM_WORLD);

            if (rc) {
                errs++;
            }
            else {
                for (int i = 0; i < sendbufsize; i++) {
                    if (recvbuf[i] != sol[i]) {
                        errs++;
                    }
                }
            }
            free(sendbuf);
            free(recvbuf);
            free(sol);
        }
    }

    for (int type = 0; type < 17; type++) {
        for (int j = 1; j < 31; j++) {
            int sendbufsize = 1 << j;
            int recvbufsize = sendbufsize / world_size;
            int rc;
            MPI_Request request;
            int *sendbuf = (int *) malloc(sizeof(int) * sendbufsize);
            int *recvbuf = (int *) malloc(sizeof(int) * recvbufsize);
            int *sol = (int *) malloc(sizeof(int) * sendbufsize);

            for (int i = 0; i < sendbufsize; i++) {
                sendbuf[i] = i;
                sol[i] = 2 * i;
            }

            rc = MPI_Reduce_scatter_block(sendbuf, recvbuf, recvbufsize,
                                          datatypes[type], MPI_SUM, MPI_COMM_WORLD);
            if (rc) {
                errs++;
            }
            else {
                for (int i = 0; i < recvbufsize; i++) {
                    if (recvbuf[i] != sol[i + rank * recvbufsize]) {
                        errs++;
                    }
                }
            }
            free(sendbuf);
            free(recvbuf);
            free(sol);
        }
    }
    MTest_Finalize(errs);
    return 0;
}
