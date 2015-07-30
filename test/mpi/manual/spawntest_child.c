/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Test code provided by Guruprasad H. Kora
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int my_rank;
    int size;
    MPI_Comm parentcomm;
    MPI_Comm allcomm;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Comm_get_parent(&parentcomm);

    if (parentcomm == MPI_COMM_NULL) {
        fprintf(stdout, "parentcomm is null\n");
    }

    MPI_Intercomm_merge(parentcomm, 1, &allcomm);

    /* Without the Free of allcomm, the children *must not exit* until the
     * master calls MPI_Finalize. */
    MPI_Barrier(allcomm);
    /* According to 10.5.4, case 1b in MPI2.2, the children and master are
     * still connected unless MPI_Comm_disconnect is used with allcomm.
     * MPI_Comm_free is not sufficient */
    MPI_Comm_free(&allcomm);
    MPI_Comm_disconnect(&parentcomm);

    fprintf(stdout, "%s:%d\n", __FILE__, __LINE__);
    fflush(stdout);
    MPI_Finalize();

    fprintf(stdout, "%d:child exiting.\n", my_rank);
    fflush(stdout);
    return 0;
}
