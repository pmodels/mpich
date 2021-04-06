/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdio.h>

/* Derived from mpi4py test case.  This test that tries to follow what the MPI
 * standard says about spawning processes with arguments that should be
 * relevant only at the root process.  See
 * https://bitbucket.org/mpi4py/mpi4py/issues/19
 * and
 * https://trac.mpich.org/projects/mpich/ticket/2282
 */

int main(int argc, char *argv[])
{
    char *args[] = { (char *) "a", (char *) "b", (char *) "c", (char *) 0 };
    int rank;
    MPI_Comm parent, child;

    MPI_Init(&argc, &argv);
    MPI_Comm_get_parent(&parent);

    if (parent == MPI_COMM_NULL) {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_spawn("./spawn_rootargs", args,        /*MPI_ARGV_NULL, */
                       5, MPI_INFO_NULL, 0, MPI_COMM_SELF, &child, MPI_ERRCODES_IGNORE);
        MPI_Barrier(child);
        MPI_Comm_disconnect(&child);
        if (!rank)
            printf(" No Errors\n");
    } else {
        MPI_Barrier(parent);
        MPI_Comm_disconnect(&parent);
    }

    MPI_Finalize();

    return 0;
}
