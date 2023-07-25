/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "utarray.h"

/* MPIR_Comm_get_failed_impl -
 *     use PMI to get list of dead process (PMI_dead_processes) in MPI_COMM_WORLD
 */
/* TODO: extend ULFM to support dynamic processes */
/* NOTE: src/mpid/ch3/src/mpid_comm_get_all_failed_procs.c contains a non-local
 *       implementation. Since ULFM require local discovery, we should remove that
 */

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

    char *failed_procs_string = MPIR_pmi_get_jobattr("PMI_dead_processes");

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

        MPIR_Group *comm_group;
        MPIR_Comm_group_impl(comm_ptr, &comm_group);

        mpi_errno = MPIR_Group_intersection_impl(comm_group, new_group, failed_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        MPIR_Group_release(comm_group);
        MPIR_Group_release(new_group);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* comm shrink impl; assumes that standard error checking has already taken
 * place in the calling function */
int MPIR_Comm_shrink_impl(MPIR_Comm * comm_ptr, MPIR_Comm ** newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *global_failed = NULL, *comm_grp = NULL, *new_group_ptr = NULL;
    int attempts = 0;

    MPIR_FUNC_ENTER;

    /* TODO - Implement this function for intercommunicators */
    MPIR_Comm_group_impl(comm_ptr, &comm_grp);

    do {
        int errflag = MPIR_ERR_NONE;

        MPID_Comm_get_all_failed_procs(comm_ptr, &global_failed, MPIR_SHRINK_TAG);
        /* Ignore the mpi_errno value here as it will definitely communicate
         * with failed procs */

        mpi_errno = MPIR_Group_difference_impl(comm_grp, global_failed, &new_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
        if (MPIR_Group_empty != global_failed)
            MPIR_Group_release(global_failed);

        mpi_errno = MPIR_Comm_create_group_impl(comm_ptr, new_group_ptr, MPIR_SHRINK_TAG,
                                                newcomm_ptr);
        if (*newcomm_ptr == NULL) {
            errflag = MPIR_ERR_PROC_FAILED;
        } else if (mpi_errno) {
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_Comm_release(*newcomm_ptr);
        }

        mpi_errno = MPII_Allreduce_group(MPI_IN_PLACE, &errflag, 1, MPI_INT, MPI_MAX, comm_ptr,
                                         new_group_ptr, MPIR_SHRINK_TAG, MPIR_ERR_NONE);
        MPIR_Group_release(new_group_ptr);

        if (errflag) {
            if (*newcomm_ptr != NULL && MPIR_Object_get_ref(*newcomm_ptr) > 0) {
                MPIR_Object_set_ref(*newcomm_ptr, 1);
                MPIR_Comm_release(*newcomm_ptr);
            }
            if (MPIR_Object_get_ref(new_group_ptr) > 0) {
                MPIR_Object_set_ref(new_group_ptr, 1);
                MPIR_Group_release(new_group_ptr);
            }
        } else {
            mpi_errno = MPI_SUCCESS;
            goto fn_exit;
        }
    } while (++attempts < 5);

    goto fn_fail;

  fn_exit:
    MPIR_Group_release(comm_grp);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (*newcomm_ptr)
        MPIR_Object_set_ref(*newcomm_ptr, 0);
    MPIR_Object_set_ref(global_failed, 0);
    MPIR_Object_set_ref(new_group_ptr, 0);
    goto fn_exit;
}

int MPIR_Comm_agree_impl(MPIR_Comm * comm_ptr, int *flag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_tmp = MPI_SUCCESS;
    MPIR_Group *comm_grp = NULL, *failed_grp = NULL, *new_group_ptr = NULL, *global_failed = NULL;
    int result, success = 1;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int values[2];

    MPIR_FUNC_ENTER;

    MPIR_Comm_group_impl(comm_ptr, &comm_grp);

    /* Get the locally known (not acknowledged) group of failed procs */
    mpi_errno = MPID_Comm_failure_get_acked(comm_ptr, &failed_grp);
    MPIR_ERR_CHECK(mpi_errno);

    /* First decide on the group of failed procs. */
    mpi_errno = MPID_Comm_get_all_failed_procs(comm_ptr, &global_failed, MPIR_AGREE_TAG);
    if (mpi_errno)
        errflag = MPIR_ERR_PROC_FAILED;

    mpi_errno = MPIR_Group_compare_impl(failed_grp, global_failed, &result);
    MPIR_ERR_CHECK(mpi_errno);

    /* Create a subgroup without the failed procs */
    mpi_errno = MPIR_Group_difference_impl(comm_grp, global_failed, &new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* If that group isn't the same as what we think is failed locally, then
     * mark it as such. */
    if (result == MPI_UNEQUAL || errflag)
        success = 0;

    /* Do an allreduce to decide whether or not anyone thinks the group
     * has changed */
    mpi_errno_tmp = MPII_Allreduce_group(MPI_IN_PLACE, &success, 1, MPI_INT, MPI_MIN, comm_ptr,
                                         new_group_ptr, MPIR_AGREE_TAG, errflag);
    if (!success || errflag || mpi_errno_tmp)
        success = 0;

    values[0] = success;
    values[1] = *flag;

    /* Determine both the result of this function (mpi_errno) and the result
     * of flag that will be returned to the user. */
    MPII_Allreduce_group(MPI_IN_PLACE, values, 2, MPI_INT, MPI_BAND, comm_ptr,
                         new_group_ptr, MPIR_AGREE_TAG, errflag);
    /* Ignore the result of the operation this time. Everyone will either
     * return a failure because of !success earlier or they will return
     * something useful for flag because of this operation. If there was a new
     * failure in between the first allreduce and the second one, it's ignored
     * here. */

    if (failed_grp != MPIR_Group_empty)
        MPIR_Group_release(failed_grp);
    MPIR_Group_release(new_group_ptr);
    MPIR_Group_release(comm_grp);
    if (global_failed != MPIR_Group_empty)
        MPIR_Group_release(global_failed);

    success = values[0];
    *flag = values[1];

    if (!success) {
        MPIR_ERR_SET(mpi_errno_tmp, MPIX_ERR_PROC_FAILED, "**mpix_comm_agree");
        MPIR_ERR_ADD(mpi_errno, mpi_errno_tmp);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
