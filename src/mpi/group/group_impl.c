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

int MPIR_Group_compare_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2, int *result)
{
    int mpi_errno = MPI_SUCCESS;

    /* See if their sizes are equal */
    if (group_ptr1->size != group_ptr2->size) {
        *result = MPI_UNEQUAL;
        goto fn_exit;
    }

    int size;
    size = group_ptr1->size;

    /* See if they are identical */
    bool is_ident = true;
    for (int i = 0; i < size; i++) {
        if (MPIR_Group_rank_to_lpid(group_ptr1, i) != MPIR_Group_rank_to_lpid(group_ptr2, i)) {
            is_ident = false;
            break;
        }
    }

    if (is_ident) {
        *result = MPI_IDENT;
        goto fn_exit;
    }

    /* See if they are similar */
    bool is_similar = true;
    for (int i = 0; i < size; i++) {
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group_ptr1, i);
        if (MPI_UNDEFINED == MPIR_Group_lpid_to_rank(group_ptr2, lpid)) {
            /* not found */
            is_similar = false;
            break;
        }
    }

    if (is_similar) {
        *result = MPI_SIMILAR;
    } else {
        *result = MPI_UNEQUAL;
    }

  fn_exit:
    return mpi_errno;
}

int MPIR_Group_translate_ranks_impl(MPIR_Group * gp1, int n, const int ranks1[],
                                    MPIR_Group * gp2, int ranks2[])
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < n; i++) {
        if (ranks1[i] == MPI_PROC_NULL) {
            ranks2[i] = MPI_PROC_NULL;
            continue;
        }
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(gp1, ranks1[i]);
        ranks2[i] = MPIR_Group_lpid_to_rank(gp2, lpid);
    }

    return mpi_errno;
}

int MPIR_Group_excl_impl(MPIR_Group * group_ptr, int n, const int ranks[],
                         MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int size = group_ptr->size;
    int nnew = size - n;

    /* Use flag fields to mark the members to *exclude* . */
    int *flags = MPL_calloc(size, sizeof(int), MPL_MEM_OTHER);
    for (int i = 0; i < n; i++) {
        flags[ranks[i]] = 1;
    }

    MPIR_Lpid *map = MPL_malloc(nnew * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int myrank = MPI_UNDEFINED;
    int newi = 0;
    for (int i = 0; i < size; i++) {
        if (flags[i] == 0) {
            map[newi] = MPIR_Group_rank_to_lpid(group_ptr, i);
            if (group_ptr->rank == i) {
                myrank = newi;
            }
            newi++;
        }
    }

    mpi_errno = MPIR_Group_create_map(nnew, myrank, group_ptr->session_ptr, map, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPL_free(flags);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_incl_impl(MPIR_Group * group_ptr, int n, const int ranks[],
                         MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (n == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    int nnew = n;
    MPIR_Lpid *map = MPL_malloc(nnew * sizeof(MPIR_Lpid), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int myrank = MPI_UNDEFINED;
    for (int i = 0; i < n; i++) {
        map[i] = MPIR_Group_rank_to_lpid(group_ptr, ranks[i]);
        if (ranks[i] == group_ptr->rank) {
            myrank = i;
        }
    }

    mpi_errno = MPIR_Group_create_map(nnew, myrank, group_ptr->session_ptr, map, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_range_excl_impl(MPIR_Group * group_ptr, int n, int ranges[][3],
                               MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Compute size, assuming that included ranks are valid (and distinct) */
    int size = group_ptr->size;
    int nnew = 0;
    for (int i = 0; i < n; i++) {
        int first = ranges[i][0];
        int last = ranges[i][1];
        int stride = ranges[i][2];
        /* works for stride of either sign.  Error checking above
         * has already guaranteed stride != 0 */
        nnew += 1 + (last - first) / stride;
    }
    nnew = size - nnew;

    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    /* Group members are taken in rank order from the original group,
     * with the specified members removed. Use the flag array for that
     * purpose.  If this was a critical routine, we could use the
     * flag values set in the error checking part, if the error checking
     * was enabled *and* we are not MPI_THREAD_MULTIPLE, but since this
     * is a low-usage routine, we haven't taken that optimization.  */

    int *flags = MPL_calloc(size, sizeof(int), MPL_MEM_OTHER);

    for (int i = 0; i < n; i++) {
        int first = ranges[i][0];
        int last = ranges[i][1];
        int stride = ranges[i][2];
        if (stride > 0) {
            for (int j = first; j <= last; j += stride) {
                flags[j] = 1;
            }
        } else {
            for (int j = first; j >= last; j += stride) {
                flags[j] = 1;
            }
        }
    }

    /* Now, run through the group and pick up the members that were
     * not excluded */
    MPIR_Lpid *map = MPL_malloc(nnew * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int myrank = MPI_UNDEFINED;
    int k = 0;
    for (int i = 0; i < size; i++) {
        if (!flags[i]) {
            map[k] = MPIR_Group_rank_to_lpid(group_ptr, i);
            if (group_ptr->rank == i) {
                myrank = k;
            }
            k++;
        }
    }

    MPL_free(flags);

    mpi_errno = MPIR_Group_create_map(nnew, myrank, group_ptr->session_ptr, map, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_range_incl_impl(MPIR_Group * group_ptr, int n, int ranges[][3],
                               MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* Compute size, assuming that included ranks are valid (and distinct) */
    int nnew = 0;
    for (int i = 0; i < n; i++) {
        int first = ranges[i][0];
        int last = ranges[i][1];
        int stride = ranges[i][2];
        /* works for stride of either sign.  Error checking above
         * has already guaranteed stride != 0 */
        nnew += 1 + (last - first) / stride;
    }

    if (nnew == 0) {
        *new_group_ptr = MPIR_Group_empty;
        goto fn_exit;
    }

    MPIR_Lpid *map = MPL_malloc(nnew * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* Group members taken in order specified by the range array */
    /* This could be integrated with the error checking, but since this
     * is a low-usage routine, we haven't taken that optimization */
    int myrank = MPI_UNDEFINED;
    int k = 0;
    for (int i = 0; i < n; i++) {
        int first = ranges[i][0];
        int last = ranges[i][1];
        int stride = ranges[i][2];
        if (stride > 0) {
            for (int j = first; j <= last; j += stride) {
                map[k] = MPIR_Group_rank_to_lpid(group_ptr, j);
                if (j == group_ptr->rank) {
                    myrank = k;
                }
                k++;
            }
        } else {
            for (int j = first; j >= last; j += stride) {
                map[k] = MPIR_Group_rank_to_lpid(group_ptr, j);
                if (j == group_ptr->rank) {
                    myrank = k;
                }
                k++;
            }
        }
    }

    mpi_errno = MPIR_Group_create_map(nnew, myrank, group_ptr->session_ptr, map, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_difference_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                               MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Assert(group_ptr1->session_ptr == group_ptr2->session_ptr);

    MPIR_Lpid *map = MPL_malloc(group_ptr1->size * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int nnew = 0;
    int myrank = MPI_UNDEFINED;
    /* For each rank in group1, search it in group2. */
    for (int i = 0; i < group_ptr1->size; i++) {
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group_ptr1, i);
        if (MPI_UNDEFINED == MPIR_Group_lpid_to_rank(group_ptr2, lpid)) {
            /* not found */
            if (i == group_ptr1->rank) {
                myrank = nnew;
            }
            map[nnew++] = lpid;
        }
    }

    /* Create the group */
    mpi_errno = MPIR_Group_create_map(nnew, myrank, group_ptr1->session_ptr, map, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

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
    MPIR_FUNC_ENTER;

    /* Similar to MPI_Group_difference, but take the ranks that are found in group2 */

    MPIR_Assert(group_ptr1->session_ptr == group_ptr2->session_ptr);

    MPIR_Lpid *map = MPL_malloc(group_ptr1->size * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    int nnew = 0;
    int myrank = MPI_UNDEFINED;
    /* For each rank in group1, search it in group2. */
    for (int i = 0; i < group_ptr1->size; i++) {
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group_ptr1, i);
        if (MPI_UNDEFINED != MPIR_Group_lpid_to_rank(group_ptr2, lpid)) {
            /* found */
            if (i == group_ptr1->rank) {
                myrank = nnew;
            }
            map[nnew++] = lpid;
        }
    }

    /* Create the group */
    mpi_errno = MPIR_Group_create_map(nnew, myrank, group_ptr1->session_ptr, map, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_union_impl(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2,
                          MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_Assert(group_ptr1->session_ptr == group_ptr2->session_ptr);

    MPIR_Lpid *map = MPL_malloc((group_ptr1->size + group_ptr2->size) * sizeof(MPIR_Lpid),
                                MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!map, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* If this process is in group1, then we can set the rank now.
     * If we are not in this group, this assignment will set the
     * current rank to MPI_UNDEFINED */
    int myrank = group_ptr1->rank;

    /* Add group1 */
    for (int i = 0; i < group_ptr1->size; i++) {
        map[i] = MPIR_Group_rank_to_lpid(group_ptr1, i);
    }

    /* Add members of group2 that are not in group 1 */
    int nnew = group_ptr1->size;
    for (int i = 0; i < group_ptr2->size; i++) {
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group_ptr2, i);
        if (MPI_UNDEFINED == MPIR_Group_lpid_to_rank(group_ptr1, lpid)) {
            /* not found */
            if (i == group_ptr2->rank) {
                myrank = nnew;
            }
            map[nnew++] = lpid;
        }
    }

    mpi_errno = MPIR_Group_create_map(nnew, myrank, group_ptr1->session_ptr, map, new_group_ptr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_from_session_pset_impl(MPIR_Session * session_ptr, const char *pset_name,
                                      MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPL_stricmp(pset_name, "mpi://WORLD") == 0) {
        mpi_errno = MPIR_Group_create_stride(MPIR_Process.size, MPIR_Process.rank, session_ptr,
                                             0, 1, 1, new_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (MPL_stricmp(pset_name, "mpi://SELF") == 0) {
        mpi_errno = MPIR_Group_create_stride(1, 0, session_ptr, 0, 1, 1, new_group_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* TODO: Implement pset struct, locate pset struct ptr */
        MPIR_ERR_SETANDSTMT(mpi_errno, MPI_ERR_ARG, goto fn_fail, "**psetinvalidname");
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
