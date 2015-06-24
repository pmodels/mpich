#include <mpi.h>
#include <stdio.h>

/* Derived from mpi4py test case.  This test that tries to follow what the MPI
 * standard says about spawning processes with arguments that should be
 * relevant only at the root process.  See
 * https://bitbucket.org/mpi4py/mpi4py/issues/19/unit-tests-fail-with-mpich-master#comment-19261971
 * and
 * https://trac.mpich.org/projects/mpich/ticket/2282
 */

int main(int argc, char *argv[])
{
    char *args[] = { "a", "b", "c", (char *) 0 };
    int rank;
    MPI_Comm master, worker;

    MPI_Init(&argc, &argv);
    MPI_Comm_get_parent(&master);

    if (master == MPI_COMM_NULL) {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_spawn("./spawn-nullargs", args,        /*MPI_ARGV_NULL, */
                       5, MPI_INFO_NULL, 0, MPI_COMM_SELF, &worker, MPI_ERRCODES_IGNORE);
        MPI_Barrier(worker);
        MPI_Comm_disconnect(&worker);
        if (!rank)
            printf(" No Errors\n");
    }
    else {
        MPI_Barrier(master);
        MPI_Comm_disconnect(&master);
    }

    MPI_Finalize();

    return 0;
}
