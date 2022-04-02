/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "utarray.h"

/* TODO: extend ULFM to support dynamic processes */

/* maintain a list of failed process in comm_world */
/* NOTE: we need maintain the order of failed_procs as the show up. We do it here because
 *       it isn't fair to require PMI to do it.
 */
static UT_array *failed_procs;

static void add_failed_proc(int rank)
{
    if (!failed_procs) {
        utarray_new(failed_procs, &ut_int_icd, MPL_MEM_OTHER);
    }

    /* NOTE: if failed procs gets too large, we may consider add a hash for O(1) search;
     *       but currently it is limited by PMI_MAXVALLEN anyway */
    int found = 0;
    for (int i = 0; i < utarray_len(failed_procs); i++) {
        int *p = (int *) utarray_eltptr(failed_procs, i);
        if (*p == rank) {
            found = 1;
            break;
        }
    }

    if (!found) {
        utarray_push_back(failed_procs, &rank, MPL_MEM_OTHER);
    }
}

static void parse_failed_procs_string(char *failed_procs_string)
{
    /* failed_procs_string is a comma separated list
     * of ranks or ranges of ranks (e.g., "1, 3-5, 11") */
    const char *delim = ",";
    char *token;

    token = strtok(failed_procs_string, delim);
    while (token != NULL) {
        char *p = strchr(token, '-');
        if (p) {
            /* a-b */
            int a = atoi(token);
            int b = atoi(p + 1);
            MPIR_Assertp(a <= b);
            for (int i = a; i <= b; i++) {
                add_failed_proc(i);
            }
        } else {
            int a = atoi(token);
            add_failed_proc(a);
        }
        token = strtok(NULL, delim);
    }
}

int MPIR_Comm_get_failed_impl(MPIR_Comm * comm_ptr, MPIR_Group ** failed_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    char *failed_procs_string = MPIR_pmi_get_failed_procs();

    if (!failed_procs_string) {
        *failed_group_ptr = MPIR_Group_empty;
    } else if (failed_procs_string[0] == '\0') {
        *failed_group_ptr = MPIR_Group_empty;
        MPL_free(failed_procs_string);
    } else {
        parse_failed_procs_string(failed_procs_string);
        MPL_free(failed_procs_string);

        /* create failed_group */
        int n = utarray_len(failed_procs);

        MPIR_Group *new_group;
        mpi_errno = MPIR_Group_create(n, &new_group);
        MPIR_ERR_CHECK(mpi_errno);

        new_group->rank = MPI_UNDEFINED;
        for (int i = 0; i < utarray_len(failed_procs); i++) {
            int *p = (int *) utarray_eltptr(failed_procs, i);
            new_group->lrank_to_lpid[i].lpid = *p;
            /* if calling process is part of the group, set the rank */
            if (*p == MPIR_Process.rank) {
                new_group->rank = i;
            }
        }
        new_group->size = n;
        new_group->idx_of_first_lpid = -1;

        *failed_group_ptr = new_group;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
