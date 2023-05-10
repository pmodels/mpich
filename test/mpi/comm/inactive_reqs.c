#include <stdlib.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Comm comm;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int dst = (rank + 1) % size;
    int src = rank - 1 < 0 ? size - 1 : rank - 1;

    /* started and freed request on MPI_COMM_WORLD */
    MPI_Request request;
    MPI_Send_init(NULL, 0, MPI_INT, dst, 0, MPI_COMM_WORLD, &request);
    MPI_Start(&request);
    MPI_Request_free(&request);

    /* inactive request on MPI_COMM_SELF */
    MPI_Send_init(NULL, 0, MPI_INT, 0, 0, MPI_COMM_SELF, &request);

    /* freed request on user comm */
    MPI_Send_init(NULL, 0, MPI_INT, dst, 0, comm, &request);
    MPI_Request_free(&request);

    /* started and completed request on MPI_COMM_WORLD */
    MPI_Recv_init(NULL, 0, MPI_INT, src, 0, MPI_COMM_WORLD, &request);
    MPI_Start(&request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);

    /* inactive request on MPI_COMM_SELF */
    MPI_Recv_init(NULL, 0, MPI_INT, 0, 0, MPI_COMM_SELF, &request);

    /* inactive request on user comm */
    MPI_Recv_init(NULL, 0, MPI_INT, src, 0, comm, &request);

    /* inactive collective request on user comm */
    MPI_Barrier_init(comm, MPI_INFO_NULL, &request);

    MPI_Comm_disconnect(&comm);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
