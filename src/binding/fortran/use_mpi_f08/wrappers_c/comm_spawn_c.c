#include "mpiimpl.h"
#include "cdesc.h"

int MPIR_Comm_spawn_c(const char *command, char *argv_f, int maxprocs, MPI_Info info, int root,
        MPI_Comm comm, MPI_Comm *intercomm, int* array_of_errcodes, int argv_elem_len)
{
    int mpi_errno = MPI_SUCCESS;
    char** argv_c = NULL;

    if ((char**)argv_f == MPI_ARGV_NULL) {
        argv_c = MPI_ARGV_NULL;
    } else {
        mpi_errno = MPIR_Fortran_array_of_string_f2c(argv_f, &argv_c, argv_elem_len,
            0 /* Don't know size of argv_f */, 0);
        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    }

    mpi_errno = PMPI_Comm_spawn(command, argv_c, maxprocs, info, root, comm, intercomm, array_of_errcodes);

    if (argv_c != MPI_ARGV_NULL) {
        MPIU_Free(argv_c);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
