#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    MPI_Group basegroup;
    MPI_Group g1;
    MPI_Comm comm, newcomm;
    int errs = 0, mpi_errno, rank, size;
    int i, errclass, nranks, *ranks;
    int worldrank;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldrank);
    comm = MPI_COMM_WORLD;
    MPI_Comm_group(comm, &basegroup);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    MPI_Comm_split(comm, 0, size - rank, &newcomm);
    MPI_Comm_group(newcomm, &g1);
    ranks = (int *) malloc(size * sizeof(int));
    for (i = 0; i < size; i++)
        ranks[i] = i;
    nranks = size;

    /*Checking group_ranslate_ranks for NULL variable */
    mpi_errno = MPI_Group_translate_ranks(g1, nranks, ranks, basegroup, NULL);
    MPI_Error_class(mpi_errno, &errclass);
    if (errclass != MPI_ERR_ARG)
        ++errs;

    MPI_Comm_free(&comm);
    MPI_Comm_free(&newcomm);
    MPI_Group_free(&basegroup);
    MPI_Group_free(&g1);
    MTest_Finalize(errs);
    return 0;
}
