/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "cdesc.h"
#include <string.h>

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
extern int MPIR_Fortran_array_of_string_f2c(const char *strs_f, char ***strs_c, int str_len,
                                            int know_size, int size)
{
    int mpi_errno = MPI_SUCCESS;
    char *buf;
    char *cur_start;
    int num_chars = 0;          /* To count the number of chars to allocate */
    int num_strs = 0;           /* To count the number of strings in strs_f[] */
    int reached_the_end = 0;    /* Indicate whether we reached the last element in strs_f[] */
    int index;

    while (!reached_the_end) {
        for (index = str_len - 1; index >= 0; index--) {        /* Trim the trailing blanks */
            if (strs_f[str_len * num_strs + index] != ' ')
                break;
        }

        num_chars += index + 1;
        num_strs++;

        if (know_size) {
            if (num_strs == size)
                reached_the_end = 1;    /* Reached the last element */
        } else if (index < 0) {
            reached_the_end = 1;        /* Found the terminating blank line */
        }
    }

    /* Allocate memory for pointers to strings and the strings themself */
    buf = (char *) malloc(sizeof(char *) * num_strs + sizeof(char) * (num_chars + num_strs));   /* Add \0 for each string */
    if (buf == NULL) {
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }

    *strs_c = (char **) buf;
    cur_start = (char *) (buf + sizeof(char *) * num_strs);

    /* Scan strs_f[] again to set *strs_c[] */
    reached_the_end = 0;
    num_strs = 0;
    while (!reached_the_end) {
        for (index = str_len - 1; index >= 0; index--) {
            if (strs_f[str_len * num_strs + index] != ' ')
                break;
        }

        size_t maxlen = (index >= 0 ? index + 1 : 0);   /* index may be -1 */
        strncpy(cur_start, &strs_f[str_len * num_strs], maxlen);
        cur_start[maxlen] = '\0';
        (*strs_c)[num_strs] = cur_start;
        cur_start += maxlen + 1;        /* Move to start of next string */

        num_strs++;

        if (know_size) {
            if (num_strs == size)
                reached_the_end = 1;    /* Reached the last element */
        } else if (maxlen == 0) {
            reached_the_end = 1;        /* Find the terminating blank line */
            (*strs_c)[num_strs - 1] = NULL;     /* Rewrite the last pointer as NULL */
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

void *MPIR_F08_get_MPI_STATUS_IGNORE(void)
{
    return (void *) MPI_STATUS_IGNORE;
}

void *MPIR_F08_get_MPI_STATUSES_IGNORE(void)
{
    return (void *) MPI_STATUSES_IGNORE;
}

void *MPIR_F08_get_MPI_ARGV_NULL(void)
{
    return (void *) MPI_ARGV_NULL;
}

void *MPIR_F08_get_MPI_ARGVS_NULL(void)
{
    return (void *) MPI_ARGVS_NULL;
}

void *MPIR_F08_get_MPI_ERRCODES_IGNORE(void)
{
    return (void *) MPI_ERRCODES_IGNORE;
}

void *MPIR_F08_get_MPI_UNWEIGHTED(void)
{
    return (void *) MPI_UNWEIGHTED;
}

void *MPIR_F08_get_MPI_WEIGHTS_EMPTY(void)
{
    return (void *) MPI_WEIGHTS_EMPTY;
}

void *MPIR_F08_get_MPI_BUFFER_AUTOMATIC(void)
{
    return (void *) MPI_BUFFER_AUTOMATIC;
}

/* ---- user op ----------------- */
/* The user's mpi_f08 op function is never called from C: a Fortran trampoline makes the
 * last call (see mpi_c_interface_glue.f90).
 *
 * Known gap (#8143): without HAS_MPIX_CALLBACKS (the standalone ABI binding) there is no place
 * to keep the function pointer for the proxy, so the function is registered directly and the
 * VALUE problem remains on that path. Not addressed yet. */
extern void MPII_f08_op_trampoline(void *fn, void *invec, void *inoutvec, int len, int datatype);
extern void MPII_f08_op_trampoline_c(void *fn, void *invec, void *inoutvec, MPI_Count len,
                                     int datatype);

int MPII_op_create_f08(void *user_fn, int commute, int *op);
int MPII_op_create_c_f08(void *user_fn, int commute, MPI_Op * op);

#ifdef HAS_MPIX_CALLBACKS
struct F08_op_state {
    void *fn;
};

static void F08_op_proxy(void *invec, void *inoutvec, MPI_Count len, MPI_Datatype datatype,
                         void *extra_state)
{
    struct F08_op_state *p = extra_state;
    MPII_f08_op_trampoline(p->fn, invec, inoutvec, (int) len, MPI_Type_toint(datatype));
}

static void F08_op_proxy_c(void *invec, void *inoutvec, MPI_Count len, MPI_Datatype datatype,
                           void *extra_state)
{
    struct F08_op_state *p = extra_state;
    MPII_f08_op_trampoline_c(p->fn, invec, inoutvec, len, MPI_Type_toint(datatype));
}

static void F08_op_free(void *extra_state)
{
    free(extra_state);
}

static int op_create_proxy(void *user_fn, MPIX_User_function_x * proxy, int commute, MPI_Op * op)
{
    struct F08_op_state *p = malloc(sizeof(struct F08_op_state));
    p->fn = user_fn;

    int ret = MPIX_Op_create_x(proxy, F08_op_free, commute, p, op);
    if (ret != MPI_SUCCESS) {
        free(p);
    }
    return ret;
}
#endif

int MPII_op_create_f08(void *user_fn, int commute, int *op)
{
    MPI_Op op_i;
#ifdef HAS_MPIX_CALLBACKS
    int ret = op_create_proxy(user_fn, F08_op_proxy, commute, &op_i);
#else
    /* known gap, see the comment above: no trampoline on this path */
    int ret = MPI_Op_create((MPI_User_function *) user_fn, commute, &op_i);
#endif
    if (ret == MPI_SUCCESS) {
        *op = MPI_Op_toint(op_i);
    }
    return ret;
}

int MPII_op_create_c_f08(void *user_fn, int commute, MPI_Op * op)
{
#ifdef HAS_MPIX_CALLBACKS
    return op_create_proxy(user_fn, F08_op_proxy_c, commute, op);
#else
    /* known gap, see the comment above: no trampoline on this path */
    return MPI_Op_create_c((MPI_User_function_c *) user_fn, commute, op);
#endif
}
