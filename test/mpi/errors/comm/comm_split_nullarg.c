#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Comm comm;
    int rank, size, color;
    int errs = 0, mpi_errno, errclass;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    color = rank % 2;
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /*test comm_split for NULL variable */
    mpi_errno = MPI_Comm_split(comm, color, rank, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&comm);
    MTest_Finalize(errs);
    return 0;
}
