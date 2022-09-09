/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpitest.h>
#include <mpi.h>

/*
static char MTEST_Descrip[] = "Isend to self with MPI_Request_free + Mprobe/Mrecv";
*/

int main(void)
{
    int errs = 0;
    MTest_Init(NULL, NULL);

    const int dummy_data = 42;

    MPI_Request req;
    MPI_Isend(&dummy_data, 1, MPI_INT, 0, 0, MPI_COMM_SELF, &req);

    MPI_Request_free(&req);

    MPI_Message mpi_msg;
    MPI_Mprobe(0, 0, MPI_COMM_SELF, &mpi_msg, MPI_STATUS_IGNORE);

    int recvd_data = 0;
    MPI_Mrecv(&recvd_data, 1, MPI_INT, &mpi_msg, MPI_STATUS_IGNORE);

    if (recvd_data != dummy_data) {
        errs++;
    }

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
