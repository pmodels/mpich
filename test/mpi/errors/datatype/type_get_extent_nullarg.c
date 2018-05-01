#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Datatype type;
    MPI_Aint lb;
    int errs = 0, errclass, mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    type = MPI_INT;
    MPI_Type_size(type, &size);
    /* Checking type_get_extent for NULL variable */
    mpi_errno = MPI_Type_get_extent(type, &lb, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Type_free(&type);
    MTest_Finalize(errs);
    return 0;
}
