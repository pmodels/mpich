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
#include <stdlib.h>
#include "mpitest.h"

#define NUM_SPAWNS 4

int main(int argc, char *argv[])
{
    int np = NUM_SPAWNS;
    int my_rank, size;
    int errcodes[NUM_SPAWNS];
    MPI_Comm allcomm;
    MPI_Comm intercomm;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    MPI_Comm_spawn((char *) "./spawntest_child", MPI_ARGV_NULL, np,
                   MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errcodes);

    if (intercomm == MPI_COMM_NULL) {
        fprintf(stdout, "intercomm is null\n");
    }

    MPI_Intercomm_merge(intercomm, 0, &allcomm);

    MPI_Comm_rank(allcomm, &my_rank);
    MPI_Comm_size(allcomm, &size);

    /* Without the Free of allcomm, the children *must not exit* until the
     * master calls MPI_Finalize. */
    MPI_Barrier(allcomm);
    /* According to 10.5.4, case 1b in MPI2.2, the children and master are
     * still connected unless MPI_Comm_disconnect is used with allcomm.
     * MPI_Comm_free is not sufficient */
    MPI_Comm_free(&allcomm);
    MPI_Comm_disconnect(&intercomm);

    fprintf(stdout, "%s:%d: MTestSleep starting; children should exit\n", __FILE__, __LINE__);
    fflush(stdout);
    MTestSleep(30);
    fprintf(stdout,
            "%s:%d: MTestSleep done; all children should have already exited\n",
            __FILE__, __LINE__);
    fflush(stdout);

    MPI_Finalize();
    return 0;
}
