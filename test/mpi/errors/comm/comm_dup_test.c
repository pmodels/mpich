#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char* argv[])
{
    int err = 0, errclass, mpi_errno;
    int rank, size;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /*test comm_dup for NULL variable */
    mpi_errno = MPI_Comm_dup(comm, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
 }
