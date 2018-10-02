#include <mpi.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    MPI_Comm comm, newcomm, scomm;
    MPI_Group group;
    MPI_Info newinfo;
    int rank, size, color;
    int errs = 0, errclass, mpi_errno;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    MPI_Comm_group(comm, &group);

    MPI_Comm_create(comm, group, &newcomm);
    color = rank % 2;
    MPI_Comm_split(MPI_COMM_WORLD, color, rank, &scomm);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /*test comm_split_type for NULL variable */
    newinfo = MPI_INFO_NULL;
    mpi_errno = MPI_Comm_split_type(scomm, 2, 4, newinfo, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&comm);
    MPI_Comm_free(&newcomm);
    MPI_Comm_free(&scomm);
    MPI_Group_free(&group);
    MTest_Finalize(errs);
    return 0;
}
