#include "mpiimpl.h"
#include "cdesc.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_spawn_multiple_c
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_spawn_multiple_c(int count, char* array_of_commands_f,
             char* array_of_argv_f, const int* array_of_maxprocs,
             const MPI_Info* array_of_info, int root, MPI_Comm comm,
             MPI_Comm* intercomm, int* array_of_errcodes,
             int commands_elem_len, int argv_elem_len)
{
    int mpi_errno = MPI_SUCCESS;
    char** array_of_commands_c = NULL;
    char*** array_of_argv_c = NULL;
    int i, j, offset, len, terminate;
    char *buf, *newbuf;

    /* array_of_commands_f in Fortran has type CHARACTER(LEN=*), INTENT(IN) :: array_of_commands(*).
      It contains commands array_of_commands(1), ..., array_of_commands(count). Each is a Fortran
      string of length commands_elem_len, which equals to len(array_of_commands).

      We need to convert array_of_commands_f into array_of_commands_c, which in C has type
      char* array_of_commands_c[count], in other words, each element is a pointer to string.
     */
    mpi_errno = MPIR_Fortran_array_of_string_f2c(array_of_commands_f, &array_of_commands_c,
        commands_elem_len, 1 /* size of array_of_commands_f is known */, count);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* array_of_argv_f in Fortran has type CHARACTER(LEN=*), INTENT(IN) :: array_of_argv(count, *).
      For a particular command number K (in the range 1..count), array_of_argv (K, 1) is the first
      argument, array_of_argv(K,2) is the second argument, ... etc., until you get to array_of_argv(K,J)
      being a string of all blank characters. That indicates that command K has J-1 arguments.
      The value of J might be different from each command, but the size of the second dimension of
      array_of_argv is the largest value of J for all the commands.  The actual memory layout of
      the array is (arg1 for command 1) (arg1 for command 2) ... (arg1 for command COUNT)
      (arg2 for command 1) ...

      We need to convert array_of_argv_f into array_of_argv_c, which in C has type
      char** array_of_argv_c[count], with each element pointing to an array of pointers.
      For example, array_of_argv_c[0] points to an array of pointers to string.
      array_of_argv_c[0][0] points to 1st arg of command 0. array_of_argv_c[0][0] points
      to 2nd arg of command 0, etc. If array_of_argv_c[0][J] is NULL, then command 0
      has J args.
    */

    if ((char***)array_of_argv_f == MPI_ARGVS_NULL) {
        array_of_argv_c = MPI_ARGVS_NULL;
    } else {
        array_of_argv_c = (char***) MPIU_Malloc(sizeof(char**)*count);
        if (!array_of_argv_c) MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        /* Allocate a temp buf to put args of a command */
        len = 256; /* length of buf. Initialized with an arbitrary value */
        buf = (char*)MPIU_Malloc(sizeof(char)*len);
        if (!buf) MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        for (i = 0; i < count; i++) {
            /* Extract args of command i, and put them in buf */
            char *arg;
            offset = 0; /* offset in bytes in buf to put next arg */
            arg = array_of_argv_f + argv_elem_len * i; /* Point to 1st arg of command i */
            do {
                if (offset + argv_elem_len > len) { /* Make sure buf is big enough */
                    len = offset + argv_elem_len;
                    newbuf = (char*)MPIU_Realloc(buf, len);
                    if (!newbuf) {
                        MPIU_Free(buf);
                        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
                    }
                    buf = newbuf;
                }

                /* Check if arg is a terminating blank string */
                j = 0;
                while (arg[j] == ' ' && j < argv_elem_len) j++;
                terminate = (j == argv_elem_len);

                strncpy(buf + offset, arg, argv_elem_len); /* Copy it even it is all blank */
                arg += argv_elem_len * count; /* Move to next arg of command i */
                offset += argv_elem_len;
            } while(!terminate);

            /* Convert the args into C style. We indicate we don't know the count of strings so
               that a NULL pointer will be appended at the end.
             */
            mpi_errno = MPIR_Fortran_array_of_string_f2c(buf, &(array_of_argv_c[i]), argv_elem_len, 0, 0);
            if (mpi_errno != MPI_SUCCESS) {
                for (j = i - 1; j >= 0; j--)
                    MPIU_Free(array_of_argv_c[j]);
                MPIU_Free(buf);
                goto fn_fail;
            }
        }

        MPIU_Free(buf);
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
