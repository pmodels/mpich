/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "group.h"

#ifndef MPID_GROUP_PREALLOC
#define MPID_GROUP_PREALLOC 8
#endif

/* Preallocated group objects */
MPIR_Group MPIR_Group_builtin[MPIR_GROUP_N_BUILTIN];
MPIR_Group MPIR_Group_direct[MPID_GROUP_PREALLOC];

MPIR_Object_alloc_t MPIR_Group_mem = { 0, 0, 0, 0, 0, 0, MPIR_GROUP,
    sizeof(MPIR_Group), MPIR_Group_direct,
    MPID_GROUP_PREALLOC,
    NULL, {0}
};

MPIR_Group *const MPIR_Group_empty = &MPIR_Group_builtin[0];

int MPIR_Group_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_GROUP_N_BUILTIN == 1);     /* update this func if this ever triggers */

    MPIR_Group_builtin[0].handle = MPI_GROUP_EMPTY;
    MPIR_Object_set_ref(&MPIR_Group_builtin[0], 1);
    MPIR_Group_builtin[0].size = 0;
    MPIR_Group_builtin[0].rank = MPI_UNDEFINED;
    MPIR_Group_builtin[0].idx_of_first_lpid = -1;
    MPIR_Group_builtin[0].lrank_to_lpid = NULL;

    /* TODO hook for device here? */
    return mpi_errno;
}


int MPIR_Group_release(MPIR_Group * group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int inuse;

    MPIR_Group_release_ref(group_ptr, &inuse);
    if (!inuse) {
        /* Only if refcount is 0 do we actually free. */
        MPL_free(group_ptr->lrank_to_lpid);
        MPIR_Handle_obj_free(&MPIR_Group_mem, group_ptr);
    }
    return mpi_errno;
}


/*
 * Allocate a new group and the group lrank to lpid array.  Does *not*
 * initialize any arrays, but does set the reference count.
 */
int MPIR_Group_create(int nproc, MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    *new_group_ptr = (MPIR_Group *) MPIR_Handle_obj_alloc(&MPIR_Group_mem);
    /* --BEGIN ERROR HANDLING-- */
    if (!*new_group_ptr) {
        mpi_errno =
            MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIR_Group_create", __LINE__,
                                 MPI_ERR_OTHER, "**nomem", 0);
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    MPIR_Object_set_ref(*new_group_ptr, 1);
    (*new_group_ptr)->lrank_to_lpid =
        (MPII_Group_pmap_t *) MPL_calloc(nproc, sizeof(MPII_Group_pmap_t), MPL_MEM_GROUP);
    /* --BEGIN ERROR HANDLING-- */
    if (!(*new_group_ptr)->lrank_to_lpid) {
        MPIR_Handle_obj_free(&MPIR_Group_mem, *new_group_ptr);
        *new_group_ptr = NULL;
        MPIR_CHKMEM_SETERR(mpi_errno, nproc * sizeof(MPII_Group_pmap_t), "newgroup->lrank_to_lpid");
        return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    (*new_group_ptr)->size = nproc;
    /* Make sure that there is no question that the list of ranks sorted
     * by pids is marked as uninitialized */
    (*new_group_ptr)->idx_of_first_lpid = -1;

    (*new_group_ptr)->is_local_dense_monotonic = FALSE;
    (*new_group_ptr)->pset_name = NULL;
    return mpi_errno;
}

/*
 * return value is the first index in the list
 *
 * This "sorts" an lpid array by lpid value, using a simple merge sort
 * algorithm.
 *
 * In actuality, it does not reorder the elements of maparray (these must remain
 * in group rank order).  Instead it builds the traversal order (in increasing
 * lpid order) through the maparray given by the "next_lpid" fields.
 */
static int mergesort_lpidarray(MPII_Group_pmap_t maparray[], int n)
{
    int idx1, idx2, first_idx, cur_idx, next_lpid, idx2_offset;

    if (n == 2) {
        if (maparray[0].lpid > maparray[1].lpid) {
            first_idx = 1;
            maparray[0].next_lpid = -1;
            maparray[1].next_lpid = 0;
        } else {
            first_idx = 0;
            maparray[0].next_lpid = 1;
            maparray[1].next_lpid = -1;
        }
        return first_idx;
    }
    if (n == 1) {
        maparray[0].next_lpid = -1;
        return 0;
    }
    if (n == 0)
        return -1;

    /* Sort each half */
    idx2_offset = n / 2;
    idx1 = mergesort_lpidarray(maparray, n / 2);
    idx2 = mergesort_lpidarray(maparray + idx2_offset, n - n / 2) + idx2_offset;
    /* merge the results */
    /* There are three lists:
     * first_idx - points to the HEAD of the sorted, merged list
     * cur_idx - points to the LAST element of the sorted, merged list
     * idx1    - points to the HEAD of one sorted list
     * idx2    - points to the HEAD of the other sorted list
     *
     * We first identify the head element of the sorted list.  We then
     * take elements from the remaining lists.  When one list is empty,
     * we add the other list to the end of sorted list.
     *
     * The last wrinkle is that the next_lpid fields in maparray[idx2]
     * are relative to n/2, not 0 (that is, a next_lpid of 1 is
     * really 1 + n/2, relative to the beginning of maparray).
     */
    /* Find the head element */
    if (maparray[idx1].lpid > maparray[idx2].lpid) {
        first_idx = idx2;
        idx2 = maparray[idx2].next_lpid + idx2_offset;
    } else {
        first_idx = idx1;
        idx1 = maparray[idx1].next_lpid;
    }

    /* Merge the lists until one is empty */
    cur_idx = first_idx;
    while (idx1 >= 0 && idx2 >= 0) {
        if (maparray[idx1].lpid > maparray[idx2].lpid) {
            next_lpid = maparray[idx2].next_lpid;
            if (next_lpid >= 0)
                next_lpid += idx2_offset;
            maparray[cur_idx].next_lpid = idx2;
            cur_idx = idx2;
            idx2 = next_lpid;
        } else {
            next_lpid = maparray[idx1].next_lpid;
            maparray[cur_idx].next_lpid = idx1;
            cur_idx = idx1;
            idx1 = next_lpid;
        }
    }
    /* Add whichever list remains */
    if (idx1 >= 0) {
        maparray[cur_idx].next_lpid = idx1;
    } else {
        maparray[cur_idx].next_lpid = idx2;
        /* Convert the rest of these next_lpid values to be
         * relative to the beginning of maparray */
        while (idx2 >= 0) {
            next_lpid = maparray[idx2].next_lpid;
            if (next_lpid >= 0) {
                next_lpid += idx2_offset;
                maparray[idx2].next_lpid = next_lpid;
            }
            idx2 = next_lpid;
        }
    }

    return first_idx;
}

/*
 * Create a list of the lpids, in lpid order.
 *
 * Called by group_compare, group_translate_ranks, group_union
 *
 * In the case of a single main thread lock, the lock must
 * be held on entry to this routine.  This forces some of the routines
 * noted above to hold the SINGLE_CS; which would otherwise not be required.
 */
void MPII_Group_setup_lpid_list(MPIR_Group * group_ptr)
{
    if (group_ptr->idx_of_first_lpid == -1) {
        group_ptr->idx_of_first_lpid =
            mergesort_lpidarray(group_ptr->lrank_to_lpid, group_ptr->size);
    }
}

void MPIR_Group_setup_lpid_pairs(MPIR_Group * group_ptr1, MPIR_Group * group_ptr2)
{
    /* If the lpid list hasn't been created, do it now */
    if (group_ptr1->idx_of_first_lpid < 0) {
        MPII_Group_setup_lpid_list(group_ptr1);
    }
    if (group_ptr2->idx_of_first_lpid < 0) {
        MPII_Group_setup_lpid_list(group_ptr2);
    }
}

#ifdef HAVE_ERROR_CHECKING
/*
 * The following routines are needed only for error checking
 */

/*
 * This routine is for error checking for a valid ranks array, used
 * by Group_incl and Group_excl.
 */
int MPIR_Group_check_valid_ranks(MPIR_Group * group_ptr, const int ranks[], int n)
{
    int mpi_errno = MPI_SUCCESS, i;

    int *flags = MPL_calloc(group_ptr->size, sizeof(int), MPL_MEM_OTHER);
    for (i = 0; i < n; i++) {
        if (ranks[i] < 0 || ranks[i] >= group_ptr->size) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_RANK, "**rankarray", "**rankarray %d %d %d", i,
                                     ranks[i], group_ptr->size - 1);
            break;
        }
        if (flags[i]) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_RANK, "**rankdup", "**rankdup %d %d %d", i, ranks[i],
                                     flags[i] - 1);
            break;
        }
        flags[i] = i + 1;
    }
    MPL_free(flags);

    return mpi_errno;
}

/* Service routine to check for valid range arguments. */
int MPIR_Group_check_valid_ranges(MPIR_Group * group_ptr, int ranges[][3], int n)
{
    int i, j, size, first, last, stride, mpi_errno = MPI_SUCCESS;

    if (n < 0) {
        MPIR_ERR_SETANDSTMT2(mpi_errno, MPI_ERR_ARG,;, "**argneg", "**argneg %s %d", "n", n);
        return mpi_errno;
    }

    size = group_ptr->size;

    int *flags = MPL_calloc(group_ptr->size, sizeof(int), MPL_MEM_OTHER);
    for (i = 0; i < n; i++) {
        int act_last;

        first = ranges[i][0];
        last = ranges[i][1];
        stride = ranges[i][2];
        if (first < 0 || first >= size) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_ARG, "**rangestartinvalid",
                                     "**rangestartinvalid %d %d %d", i, first, size);
            break;
        }
        if (stride == 0) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_ARG, "**stridezero", 0);
            break;
        }

        /* We must compute the actual last value, taking into account
         * the stride value.  At this point, we know that the stride is
         * non-zero
         */
        act_last = first + stride * ((last - first) / stride);

        if (last < 0 || act_last >= size) {
            /* Use last instead of act_last in the error message since
             * the last value is the one that the user provided */
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_ARG, "**rangeendinvalid", "**rangeendinvalid %d %d %d",
                                     i, last, size);
            break;
        }
        if ((stride > 0 && first > last) || (stride < 0 && first < last)) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_ARG, "**stride", "**stride %d %d %d", first, last,
                                     stride);
            break;
        }

        /* range is valid.  Mark flags */
        if (stride > 0) {
            for (j = first; j <= last; j += stride) {
                if (flags[j]) {
                    mpi_errno =
                        MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                             MPI_ERR_ARG, "**rangedup", "**rangedup %d %d %d", j, i,
                                             flags[j] - 1);
                    break;
                } else
                    flags[j] = 1;
            }
        } else {
            for (j = first; j >= last; j += stride) {
                if (flags[j]) {
                    mpi_errno =
                        MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                             MPI_ERR_ARG, "**rangedup", "**rangedup %d %d %d", j, i,
                                             flags[j] - 1);
                    break;
                } else
                    /* Set to i + 1 so that we can remember where it was
                     * first set */
                    flags[j] = i + 1;
            }
        }
        if (mpi_errno)
            break;
    }
    MPL_free(flags);

    return mpi_errno;
}

/* Given a group and a comm, check that the group is a subset of the processes
   defined by the comm.

   We sort the lpids for the group and the comm.  If the group has an
   lpid that is not in the comm, then report an error.
*/
int MPIR_Group_check_subset(MPIR_Group * group_ptr, MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int g1_idx, g2_idx, l1_pid, l2_pid, i;
    MPII_Group_pmap_t *vmap = 0;
    int vsize = comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM ? comm_ptr->local_size :
        comm_ptr->remote_size;
    MPIR_CHKLMEM_DECL(1);

    MPIR_Assert(group_ptr != NULL);

    MPIR_CHKLMEM_MALLOC(vmap, MPII_Group_pmap_t *,
                        vsize * sizeof(MPII_Group_pmap_t), mpi_errno, "", MPL_MEM_GROUP);
    /* Initialize the vmap */
    for (i = 0; i < vsize; i++) {
        MPID_Comm_get_lpid(comm_ptr, i, &vmap[i].lpid, FALSE);
        vmap[i].next_lpid = 0;
    }

    MPII_Group_setup_lpid_list(group_ptr);
    g1_idx = group_ptr->idx_of_first_lpid;
    g2_idx = mergesort_lpidarray(vmap, vsize);
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST,
                                             "initial indices: %d %d\n", g1_idx, g2_idx));
    while (g1_idx >= 0 && g2_idx >= 0) {
        l1_pid = group_ptr->lrank_to_lpid[g1_idx].lpid;
        l2_pid = vmap[g2_idx].lpid;
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST,
                                                 "Lpids are %d, %d\n", l1_pid, l2_pid));
        if (l1_pid < l2_pid) {
            /* If we have to advance g1, we didn't find a match, so
             * that's an error. */
            break;
        } else if (l1_pid > l2_pid) {
            g2_idx = vmap[g2_idx].next_lpid;
        } else {
            /* Equal */
            g1_idx = group_ptr->lrank_to_lpid[g1_idx].next_lpid;
            g2_idx = vmap[g2_idx].next_lpid;
        }
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST,
                                                 "g1 = %d, g2 = %d\n", g1_idx, g2_idx));
    }

    if (g1_idx >= 0) {
        MPIR_ERR_SET1(mpi_errno, MPI_ERR_GROUP, "**groupnotincomm", "**groupnotincomm %d", g1_idx);
    }

  fn_fail:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
}

#endif /* HAVE_ERROR_CHECKING */
