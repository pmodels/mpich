/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "group.h"

int MPIR_Group_rank_impl(MPIR_Group * group_ptr, int *rank)
{
    *rank = group_ptr->rank;
    return MPI_SUCCESS;
}

int MPIR_Group_size_impl(MPIR_Group * group_ptr, int *size)
{
    *size = group_ptr->size;
    return MPI_SUCCESS;
}

int MPIR_Group_compare_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2, int *result)
{
    int mpi_errno = MPI_SUCCESS;
    int g1_idx, g2_idx, size, i;

    /* See if their sizes are equal */
    if (group_ptr1->size != group_ptr2->size) {
        *result = MPI_UNEQUAL;
        goto fn_exit;
    }

    /* Run through the lrank to lpid lists of each group in lpid order
     * to see if the same processes are involved */
    g1_idx = group_ptr1->idx_of_first_lpid;
    g2_idx = group_ptr2->idx_of_first_lpid;
    /* If the lpid list hasn't been created, do it now */
    if (g1_idx < 0) {
        MPII_Group_setup_lpid_list(group_ptr1);
        g1_idx = group_ptr1->idx_of_first_lpid;
    }
    if (g2_idx < 0) {
        MPII_Group_setup_lpid_list(group_ptr2);
        g2_idx = group_ptr2->idx_of_first_lpid;
    }
    while (g1_idx >= 0 && g2_idx >= 0) {
        if (group_ptr1->lrank_to_lpid[g1_idx].lpid != group_ptr2->lrank_to_lpid[g2_idx].lpid) {
            *result = MPI_UNEQUAL;
            goto fn_exit;
        }
        g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
        g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
    }

    /* See if the processes are in the same order by rank */
    size = group_ptr1->size;
    for (i = 0; i < size; i++) {
        if (group_ptr1->lrank_to_lpid[i].lpid != group_ptr2->lrank_to_lpid[i].lpid) {
            *result = MPI_SIMILAR;
            goto fn_exit;
        }
    }

    /* If we reach here, the groups are identical */
    *result = MPI_IDENT;

  fn_exit:
    return mpi_errno;
}

int MPIR_Group_difference_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                               MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int size1, i, k, g1_idx, g2_idx, l1_pid, l2_pid, nnew;
    int *flags = NULL;

    MPIR_FUNC_ENTER;
    /* Return a group consisting of the members of group1 that are *not*
     * in group2 */
    size1 = group_ptr1->size;
    /* Insure that the lpid lists are setup */
    MPIR_Group_setup_lpid_pairs(group_ptr1, group_ptr2);

    flags = MPL_calloc(size1, sizeof(int), MPL_MEM_OTHER);

    g1_idx = group_ptr1->idx_of_first_lpid;
    g2_idx = group_ptr2->idx_of_first_lpid;

    nnew = size1;
    while (g1_idx >= 0 && g2_idx >= 0) {
        l1_pid = group_ptr1->lrank_to_lpid[g1_idx].lpid;
        l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
        if (l1_pid < l2_pid) {
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
        } else if (l1_pid > l2_pid) {
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
        } else {
            /* Equal */
            flags[g1_idx] = 1;
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
            nnew--;
        }
    }
    /* Create the group */
    if (nnew == 0) {
        /* See 5.3.2, Group Constructors.  For many group routines,
         * the standard explicitly says to return MPI_GROUP_EMPTY;
         * for others it is implied */
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    } else {
        mpi_errno = MPIR_Group_create(nnew, new_group_ptr);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno) {
            goto fn_fail;
        }
        /* --END ERROR HANDLING-- */
        (*new_group_ptr)->rank = MPI_UNDEFINED;
        k = 0;
        for (i = 0; i < size1; i++) {
            if (!flags[i]) {
                (*new_group_ptr)->lrank_to_lpid[k].lpid = group_ptr1->lrank_to_lpid[i].lpid;
                if (i == group_ptr1->rank)
                    (*new_group_ptr)->rank = k;
                k++;
            }
        }
        /* TODO calculate is_local_dense_monotonic */
    }

  fn_exit:
    MPL_free(flags);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_excl_impl(MPIR_Group * group_ptr, int n, const int ranks[],
                         MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int size, i, newi;
    int *flags = NULL;

    MPIR_FUNC_ENTER;

    size = group_ptr->size;

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create(size - n, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    (*new_group_ptr)->rank = MPI_UNDEFINED;
    /* Use flag fields to mark the members to *exclude* . */

    flags = MPL_calloc(size, sizeof(int), MPL_MEM_OTHER);

    for (i = 0; i < n; i++) {
        flags[ranks[i]] = 1;
    }

    newi = 0;
    for (i = 0; i < size; i++) {
        if (flags[i] == 0) {
            (*new_group_ptr)->lrank_to_lpid[newi].lpid = group_ptr->lrank_to_lpid[i].lpid;
            if (group_ptr->rank == i)
                (*new_group_ptr)->rank = newi;
            newi++;
        }
    }

    (*new_group_ptr)->size = size - n;
    (*new_group_ptr)->idx_of_first_lpid = -1;
    /* TODO calculate is_local_dense_monotonic */

  fn_exit:
    MPL_free(flags);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_free_impl(MPIR_Group * group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Do not free MPI_GROUP_EMPTY */
    if (group_ptr->handle != MPI_GROUP_EMPTY) {
        mpi_errno = MPIR_Group_release(group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_incl_impl(MPIR_Group * group_ptr, int n, const int ranks[],
                         MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_ENTER;

    if (n == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create(n, new_group_ptr);
    if (mpi_errno)
        goto fn_fail;

    (*new_group_ptr)->rank = MPI_UNDEFINED;
    for (i = 0; i < n; i++) {
        (*new_group_ptr)->lrank_to_lpid[i].lpid = group_ptr->lrank_to_lpid[ranks[i]].lpid;
        if (ranks[i] == group_ptr->rank)
            (*new_group_ptr)->rank = i;
    }
    (*new_group_ptr)->size = n;
    (*new_group_ptr)->idx_of_first_lpid = -1;
    /* TODO calculate is_local_dense_monotonic */


  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_intersection_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                                 MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int size1, i, k, g1_idx, g2_idx, l1_pid, l2_pid, nnew;
    int *flags = NULL;

    MPIR_FUNC_ENTER;
    /* Return a group consisting of the members of group1 that are
     * in group2 */
    size1 = group_ptr1->size;
    /* Insure that the lpid lists are setup */
    MPIR_Group_setup_lpid_pairs(group_ptr1, group_ptr2);

    flags = MPL_calloc(size1, sizeof(int), MPL_MEM_OTHER);

    g1_idx = group_ptr1->idx_of_first_lpid;
    g2_idx = group_ptr2->idx_of_first_lpid;

    nnew = 0;
    while (g1_idx >= 0 && g2_idx >= 0) {
        l1_pid = group_ptr1->lrank_to_lpid[g1_idx].lpid;
        l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
        if (l1_pid < l2_pid) {
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
        } else if (l1_pid > l2_pid) {
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
        } else {
            /* Equal */
            flags[g1_idx] = 1;
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
            nnew++;
        }
    }
    /* Create the group.  Handle the trivial case first */
    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    mpi_errno = MPIR_Group_create(nnew, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    (*new_group_ptr)->rank = MPI_UNDEFINED;
    (*new_group_ptr)->is_local_dense_monotonic = TRUE;
    k = 0;
    for (i = 0; i < size1; i++) {
        if (flags[i]) {
            int lpid = group_ptr1->lrank_to_lpid[i].lpid;
            (*new_group_ptr)->lrank_to_lpid[k].lpid = lpid;
            if (i == group_ptr1->rank)
                (*new_group_ptr)->rank = k;
            if (lpid > MPIR_Process.size ||
                (k > 0 && (*new_group_ptr)->lrank_to_lpid[k - 1].lpid != (lpid - 1))) {
                (*new_group_ptr)->is_local_dense_monotonic = FALSE;
            }

            k++;
        }
    }

  fn_exit:
    MPL_free(flags);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_range_excl_impl(MPIR_Group * group_ptr, int n, int ranges[][3],
                               MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int size, i, j, k, nnew, first, last, stride;
    int *flags = NULL;

    MPIR_FUNC_ENTER;
    /* Compute size, assuming that included ranks are valid (and distinct) */
    size = group_ptr->size;
    nnew = 0;
    for (i = 0; i < n; i++) {
        first = ranges[i][0];
        last = ranges[i][1];
        stride = ranges[i][2];
        /* works for stride of either sign.  Error checking above
         * has already guaranteed stride != 0 */
        nnew += 1 + (last - first) / stride;
    }
    nnew = size - nnew;

    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create(nnew, new_group_ptr);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno) {
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    (*new_group_ptr)->rank = MPI_UNDEFINED;

    /* Group members are taken in rank order from the original group,
     * with the specified members removed. Use the flag array for that
     * purpose.  If this was a critical routine, we could use the
     * flag values set in the error checking part, if the error checking
     * was enabled *and* we are not MPI_THREAD_MULTIPLE, but since this
     * is a low-usage routine, we haven't taken that optimization.  */

    flags = MPL_calloc(size, sizeof(int), MPL_MEM_OTHER);

    for (i = 0; i < n; i++) {
        first = ranges[i][0];
        last = ranges[i][1];
        stride = ranges[i][2];
        if (stride > 0) {
            for (j = first; j <= last; j += stride) {
                flags[j] = 1;
            }
        } else {
            for (j = first; j >= last; j += stride) {
                flags[j] = 1;
            }
        }
    }
    /* Now, run through the group and pick up the members that were
     * not excluded */
    k = 0;
    for (i = 0; i < size; i++) {
        if (!flags[i]) {
            (*new_group_ptr)->lrank_to_lpid[k].lpid = group_ptr->lrank_to_lpid[i].lpid;
            if (group_ptr->rank == i) {
                (*new_group_ptr)->rank = k;
            }
            k++;
        }
    }

    /* TODO calculate is_local_dense_monotonic */

  fn_exit:
    MPL_free(flags);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_range_incl_impl(MPIR_Group * group_ptr, int n, int ranges[][3],
                               MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int first, last, stride, nnew, i, j, k;

    MPIR_FUNC_ENTER;

    /* Compute size, assuming that included ranks are valid (and distinct) */
    nnew = 0;
    for (i = 0; i < n; i++) {
        first = ranges[i][0];
        last = ranges[i][1];
        stride = ranges[i][2];
        /* works for stride of either sign.  Error checking above
         * has already guaranteed stride != 0 */
        nnew += 1 + (last - first) / stride;
    }

    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create(nnew, new_group_ptr);
    if (mpi_errno)
        goto fn_fail;
    (*new_group_ptr)->rank = MPI_UNDEFINED;

    /* Group members taken in order specified by the range array */
    /* This could be integrated with the error checking, but since this
     * is a low-usage routine, we haven't taken that optimization */
    k = 0;
    for (i = 0; i < n; i++) {
        first = ranges[i][0];
        last = ranges[i][1];
        stride = ranges[i][2];
        if (stride > 0) {
            for (j = first; j <= last; j += stride) {
                (*new_group_ptr)->lrank_to_lpid[k].lpid = group_ptr->lrank_to_lpid[j].lpid;
                if (j == group_ptr->rank)
                    (*new_group_ptr)->rank = k;
                k++;
            }
        } else {
            for (j = first; j >= last; j += stride) {
                (*new_group_ptr)->lrank_to_lpid[k].lpid = group_ptr->lrank_to_lpid[j].lpid;
                if (j == group_ptr->rank)
                    (*new_group_ptr)->rank = k;
                k++;
            }
        }
    }

    /* TODO calculate is_local_dense_monotonic */

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_translate_ranks_impl(MPIR_Group * gp1, int n, const int ranks1[],
                                    MPIR_Group * gp2, int ranks2[])
{
    int mpi_errno = MPI_SUCCESS;
    int i, g2_idx, l1_pid, l2_pid;

    MPL_DBG_MSG_S(MPIR_DBG_OTHER, VERBOSE, "gp2->is_local_dense_monotonic=%s",
                  (gp2->is_local_dense_monotonic ? "TRUE" : "FALSE"));

    /* Initialize the output ranks */
    for (i = 0; i < n; i++)
        ranks2[i] = MPI_UNDEFINED;

    if (gp2->size > 0 && gp2->is_local_dense_monotonic) {
        /* g2 probably == group_of(MPI_COMM_WORLD); use fast, constant-time lookup */
        int lpid_offset = gp2->lrank_to_lpid[0].lpid;

        MPIR_Assert(lpid_offset >= 0);
        for (i = 0; i < n; ++i) {
            int g1_lpid;

            if (ranks1[i] == MPI_PROC_NULL) {
                ranks2[i] = MPI_PROC_NULL;
                continue;
            }
            /* "adjusted" lpid from g1 */
            g1_lpid = gp1->lrank_to_lpid[ranks1[i]].lpid - lpid_offset;
            if ((g1_lpid >= 0) && (g1_lpid < gp2->size)) {
                ranks2[i] = g1_lpid;
            }
            /* else leave UNDEFINED */
        }
    } else {
        /* general, slow path; lookup time is dependent on the user-provided rank values! */
        g2_idx = gp2->idx_of_first_lpid;
        if (g2_idx < 0) {
            MPII_Group_setup_lpid_list(gp2);
            g2_idx = gp2->idx_of_first_lpid;
        }
        if (g2_idx >= 0) {
            /* g2_idx can be < 0 if the g2 group is empty */
            l2_pid = gp2->lrank_to_lpid[g2_idx].lpid;
            for (i = 0; i < n; i++) {
                if (ranks1[i] == MPI_PROC_NULL) {
                    ranks2[i] = MPI_PROC_NULL;
                    continue;
                }
                l1_pid = gp1->lrank_to_lpid[ranks1[i]].lpid;
                /* Search for this l1_pid in group2.  Use the following
                 * optimization: start from the last position in the lpid list
                 * if possible.  A more sophisticated version could use a
                 * tree based or even hashed search to speed the translation. */
                if (l1_pid < l2_pid || g2_idx < 0) {
                    /* Start over from the beginning */
                    g2_idx = gp2->idx_of_first_lpid;
                    l2_pid = gp2->lrank_to_lpid[g2_idx].lpid;
                }
                while (g2_idx >= 0 && l1_pid > l2_pid) {
                    g2_idx = gp2->lrank_to_lpid[g2_idx].next_lpid;
                    if (g2_idx >= 0)
                        l2_pid = gp2->lrank_to_lpid[g2_idx].lpid;
                    else
                        l2_pid = -1;
                }
                if (l1_pid == l2_pid)
                    ranks2[i] = g2_idx;
            }
        }
    }
    return mpi_errno;
}

int MPIR_Group_union_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                          MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int g1_idx, g2_idx, nnew, i, k, size1, size2, mylpid;
    int *flags = NULL;

    MPIR_FUNC_ENTER;

    /* Determine the size of the new group.  The new group consists of all
     * members of group1 plus the members of group2 that are not in group1.
     */
    g1_idx = group_ptr1->idx_of_first_lpid;
    g2_idx = group_ptr2->idx_of_first_lpid;

    /* If the lpid list hasn't been created, do it now */
    if (g1_idx < 0) {
        MPII_Group_setup_lpid_list(group_ptr1);
        g1_idx = group_ptr1->idx_of_first_lpid;
    }
    if (g2_idx < 0) {
        MPII_Group_setup_lpid_list(group_ptr2);
        g2_idx = group_ptr2->idx_of_first_lpid;
    }
    nnew = group_ptr1->size;

    /* Clear the flag bits on the second group.  The flag is set if
     * a member of the second group belongs to the union */
    size2 = group_ptr2->size;
    flags = MPL_calloc(size2, sizeof(int), MPL_MEM_OTHER);

    /* Loop through the lists that are ordered by lpid (local process
     * id) to detect which processes in group 2 are not in group 1
     */
    while (g1_idx >= 0 && g2_idx >= 0) {
        int l1_pid, l2_pid;
        l1_pid = group_ptr1->lrank_to_lpid[g1_idx].lpid;
        l2_pid = group_ptr2->lrank_to_lpid[g2_idx].lpid;
        if (l1_pid > l2_pid) {
            nnew++;
            flags[g2_idx] = 1;
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
        } else if (l1_pid == l2_pid) {
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
            g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
        } else {
            /* l1 < l2 */
            g1_idx = group_ptr1->lrank_to_lpid[g1_idx].next_lpid;
        }
    }
    /* If we hit the end of group1, add the remaining members of group 2 */
    while (g2_idx >= 0) {
        nnew++;
        flags[g2_idx] = 1;
        g2_idx = group_ptr2->lrank_to_lpid[g2_idx].next_lpid;
    }

    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    /* Allocate a new group and lrank_to_lpid array */
    mpi_errno = MPIR_Group_create(nnew, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* If this process is in group1, then we can set the rank now.
     * If we are not in this group, this assignment will set the
     * current rank to MPI_UNDEFINED */
    (*new_group_ptr)->rank = group_ptr1->rank;

    /* Add group1 */
    size1 = group_ptr1->size;
    for (i = 0; i < size1; i++) {
        (*new_group_ptr)->lrank_to_lpid[i].lpid = group_ptr1->lrank_to_lpid[i].lpid;
    }

    /* Add members of group2 that are not in group 1 */

    if (group_ptr1->rank == MPI_UNDEFINED && group_ptr2->rank >= 0) {
        mylpid = group_ptr2->lrank_to_lpid[group_ptr2->rank].lpid;
    } else {
        mylpid = -2;
    }
    k = size1;
    for (i = 0; i < size2; i++) {
        if (flags[i]) {
            (*new_group_ptr)->lrank_to_lpid[k].lpid = group_ptr2->lrank_to_lpid[i].lpid;
            if ((*new_group_ptr)->rank == MPI_UNDEFINED &&
                group_ptr2->lrank_to_lpid[i].lpid == mylpid)
                (*new_group_ptr)->rank = k;
            k++;
        }
    }

    /* TODO calculate is_local_dense_monotonic */

  fn_exit:
    MPL_free(flags);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_from_session_pset_impl(MPIR_Session * session_ptr, const char *pset_name,
                                      MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr;

    if (MPL_stricmp(pset_name, "mpi://WORLD") == 0) {
        mpi_errno = MPIR_Group_create(MPIR_Process.size, &group_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        group_ptr->size = MPIR_Process.size;
        group_ptr->rank = MPIR_Process.rank;
        group_ptr->is_local_dense_monotonic = TRUE;
        for (int i = 0; i < group_ptr->size; i++) {
            group_ptr->lrank_to_lpid[i].lpid = i;
            group_ptr->lrank_to_lpid[i].next_lpid = i + 1;
        }
        group_ptr->lrank_to_lpid[group_ptr->size - 1].next_lpid = -1;
        group_ptr->idx_of_first_lpid = 0;
    } else if (MPL_stricmp(pset_name, "mpi://SELF") == 0) {
        mpi_errno = MPIR_Group_create(1, &group_ptr);
        MPIR_ERR_CHECK(mpi_errno);

        group_ptr->size = 1;
        group_ptr->rank = 0;
        group_ptr->is_local_dense_monotonic = TRUE;
        group_ptr->lrank_to_lpid[0].lpid = MPIR_Process.rank;
        group_ptr->lrank_to_lpid[0].next_lpid = -1;
        group_ptr->idx_of_first_lpid = 0;
    } else {
        /* TODO: Implement pset struct, locate pset struct ptr */
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**psetinvalidname");
    }

    *new_group_ptr = group_ptr;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
