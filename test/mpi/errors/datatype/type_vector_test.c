#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int rank, size;
    MPI_Datatype type2, type;
    int errs = 0, errclass, mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Type_contiguous(3, MPI_INT, &type2);
    MPI_Type_commit(&type2);
    /* Checking type_vector for NULL variable */
    mpi_errno = MPI_Type_vector(3, 2, 3, type2, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Type_free(&type);
    MPI_Type_free(&type2);
    MTest_Finlaize();
    MPI_Finalize();
    return 0;
}

