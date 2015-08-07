
#include "mpiimpl.h"
#include "cdesc.h"

/*
  Convert an array of strings in Fortran Format to an array of strings in C format (i.e., char* a[]).

  IN strs_f:  An array of strings in Fortran
  OUT strs_c: An array of char* in C
  IN str_len: The length of a string, i.e., an element in strs_f[].
               By Fortran convention, all strings have the same length
  IN know_size: Indicate if size of strs_f[] is already known.
     If it is false, then the last element of strs_f[] is represented by a blank line.
     Accordingly, we put a NULL to strs_c[] as the last element of strs_c
     This case mimics argv in MPI_Comm_spawn

     If it is true, then 'size' gives the size of strs_f[]. No terminating NULL is needed in strs_c[]
     This case mimics array_of_commands in MPI_Comm_spawn_multiple

  IN size: Used only when know_size is true.

  Note: The caller needs to free memory of strs_c
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Fortran_array_of_string_f2c
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
extern int MPIR_Fortran_array_of_string_f2c(const char* strs_f, char*** strs_c, int str_len, int know_size, int size)
{
    int mpi_errno = MPI_SUCCESS;
    char* buf;
    char* cur_start;
    int num_chars = 0; /* To count the number of chars to allocate */
    int num_strs = 0; /* To count the number of strings in strs_f[] */
    int reached_the_end = 0; /* Indicate whether we reached the last element in strs_f[] */
    int index;

    while (!reached_the_end) {
        for (index = str_len - 1; index >= 0; index--) { /* Trim the trailing blanks */
            if (strs_f[str_len * num_strs + index] != ' ') break;
        }

        num_chars += index + 1;
        num_strs++;

        if (know_size) {
            if (num_strs == size) reached_the_end = 1; /* Reached the last element */
        } else if (index < 0) {
            reached_the_end = 1; /* Found the terminating blank line */
        }
    }

    /* Allocate memory for pointers to strings and the strings themself */
    buf = (char*) MPIU_Malloc(sizeof(char*) * num_strs + sizeof(char) * (num_chars + num_strs)); /* Add \0 for each string */
    if (buf == NULL) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    *strs_c = (char**)buf;
    cur_start = (char*)(buf + sizeof(char*) * num_strs);

    /* Scan strs_f[] again to set *strs_c[] */
    reached_the_end = 0;
    num_strs = 0;
    while (!reached_the_end) {
        for (index = str_len - 1; index >= 0; index--) {
            if (strs_f[str_len * num_strs + index] != ' ') break;
        }

        strncpy(cur_start, &strs_f[str_len*num_strs], index + 1); /* index may be -1 */
        cur_start[index + 1] = '\0';
        (*strs_c)[num_strs] = cur_start;
        cur_start += index + 2; /* Move to start of next string */

        num_strs++;

        if (know_size) {
            if (num_strs == size) reached_the_end = 1; /* Reached the last element */
        } else if (index < 0) {
            reached_the_end = 1; /* Find the terminating blank line */
            (*strs_c)[num_strs - 1] = NULL; /* Rewrite the last pointer as NULL */
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
