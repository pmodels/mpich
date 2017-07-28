#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

#define BLKLEN (1024)
#define NUM_BLKS (64)

double large_buf[(BLKLEN + 1) * NUM_BLKS];
double large_buf1[(BLKLEN + 1) * NUM_BLKS];

int main(int argc, char *argv[])
{
    int errs = 0, nranks, rank, i, j, r = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    if (nranks != 3)
        MPI_Abort(MPI_COMM_WORLD, 1);

    MPI_Request reqs[nranks-1];

    if (rank == 0) {
        MPI_Datatype type;
        MPI_Type_create_hvector(NUM_BLKS, BLKLEN, (BLKLEN + 1) * sizeof(double), MPI_DOUBLE, &type);
        MPI_Type_commit(&type);

        MPI_Irecv(large_buf, 1, type, r + 1, 111, MPI_COMM_WORLD, &reqs[r]);
        MPI_Irecv(large_buf1, 1, type, r + 2, 111, MPI_COMM_WORLD, &reqs[r+1]);

        MPI_Type_free(&type);

        MPI_Waitall(nranks - 1, reqs, MPI_STATUSES_IGNORE);

        for (i = 0; i < (BLKLEN * NUM_BLKS); i+=1025) {
             for (j = i; j < i + 1024; j++) {
                  if (large_buf[j] != rank + 1)
		      ++errs;
                  if (large_buf1[j] != rank + 2)
                      ++errs;
             }
        }
    }
    else {
        for (i=0; i < (BLKLEN * NUM_BLKS); i++)
             large_buf[i] = rank;
        MPI_Send(large_buf, BLKLEN * NUM_BLKS, MPI_DOUBLE, 0, 111, MPI_COMM_WORLD);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
