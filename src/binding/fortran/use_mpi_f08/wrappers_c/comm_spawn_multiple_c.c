#include "mpiimpl.h"
#include "cdesc.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_spawn_multiple_c
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Comm_spawn_multiple_c(int count, char *array_of_commands_f,
             char **array_of_argv_f, const int* array_of_maxprocs,
             const MPI_Info *array_of_info, int root, MPI_Comm comm,
             MPI_Comm *intercomm, int* array_of_errcodes,
             int commands_elem_len, int argv_elem_len)
{
    int mpi_errno = MPI_SUCCESS;
    char*** array_of_argv_c = NULL;
    char** array_of_commands_c = NULL;
    int i, j;

    mpi_errno = MPIR_Fortran_array_of_string_f2c(array_of_commands_f, &array_of_commands_c,
                    commands_elem_len, 1, count); /* array_of_commands_f size is known */
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if ((char***)array_of_argv_f == MPI_ARGVS_NULL) {
        array_of_argv_c = MPI_ARGVS_NULL;
    } else {
        array_of_argv_c = (char***) MPIU_Malloc(sizeof(char**)*count);
        if (array_of_argv_c == NULL) {
            MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
        }

        for (i = 0; i < count; i++) {
            mpi_errno = MPIR_Fortran_array_of_string_f2c(array_of_argv_f[i],
                 &(array_of_argv_c[i]), argv_elem_len, 0, 0); /* array_of_argv_f[i] size is unknown */
            if (mpi_errno != MPI_SUCCESS) {
                for (j = i - 1; j >= 0; j--)
                    MPIU_Free(array_of_argv_c[j]);
                goto fn_fail;
            }
        }
    }

    mpi_errno = PMPI_Comm_spawn_multiple(count, array_of_commands_c, array_of_argv_c,
            array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes);

    MPIU_Free(array_of_commands_c);

    if (array_of_argv_c != MPI_ARGVS_NULL) {
        for (i = 0; i < count; i++)
            MPIU_Free(array_of_argv_c[i]);
        MPIU_Free(array_of_argv_c);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
