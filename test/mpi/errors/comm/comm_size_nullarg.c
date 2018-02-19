#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int rank, size;
    int errclass, errs = 0, mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /*test comm_size for NULL variable */
    mpi_errno = MPI_Comm_size(MPI_COMM_WORLD, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MTest_Finalize(errs);
    return 0;
}
