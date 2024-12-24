/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "group.h"

/* Preallocated group objects */
MPIR_Group MPIR_Group_builtin[MPIR_GROUP_N_BUILTIN];
MPIR_Group MPIR_Group_direct[MPIR_GROUP_PREALLOC];

MPIR_Object_alloc_t MPIR_Group_mem = { 0, 0, 0, 0, 0, 0, 0, MPIR_GROUP,
    sizeof(MPIR_Group), MPIR_Group_direct,
    MPIR_GROUP_PREALLOC,
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
    MPIR_Group_builtin[0].session_ptr = NULL;
    memset(&MPIR_Group_builtin[0].pmap, 0, sizeof(struct MPIR_Pmap));

    return mpi_errno;
}


int MPIR_Group_release(MPIR_Group * group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int inuse;

    MPIR_Group_release_ref(group_ptr, &inuse);
    if (!inuse) {
        /* Only if refcount is 0 do we actually free. */
        if (group_ptr->pmap.use_map) {
            MPL_free(group_ptr->pmap.u.map);
        }
        if (group_ptr->session_ptr != NULL) {
            /* Release session */
            MPIR_Session_release(group_ptr->session_ptr);
        }
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

    /* initialize fields */
    (*new_group_ptr)->size = nproc;
    (*new_group_ptr)->rank = MPI_UNDEFINED;
    (*new_group_ptr)->session_ptr = NULL;
    memset(&(*new_group_ptr)->pmap, 0, sizeof(struct MPIR_Pmap));
    (*new_group_ptr)->pmap.size = nproc;

    return mpi_errno;
}

static bool check_map_is_strided(int size, MPIR_Lpid * map,
                                 MPIR_Lpid * offset_out, MPIR_Lpid * stride_out,
                                 MPIR_Lpid * blocksize_out);

int MPIR_Group_create_map(int size, int rank, MPIR_Session * session_ptr, MPIR_Lpid * map,
                          MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (size == 0) {
        /* See 5.3.2, Group Constructors.  For many group routines,
         * the standard explicitly says to return MPI_GROUP_EMPTY;
         * for others it is implied */
        MPL_free(map);
        *new_group_ptr = MPIR_Group_empty;
    } else {
        MPIR_Group *newgrp;
        mpi_errno = MPIR_Group_create(size, &newgrp);
        MPIR_ERR_CHECK(mpi_errno);

        newgrp->rank = rank;
        MPIR_Group_set_session_ptr(newgrp, session_ptr);

        if (check_map_is_strided(size, map, &newgrp->pmap.u.stride.offset,
                                 &newgrp->pmap.u.stride.stride, &newgrp->pmap.u.stride.blocksize)) {
            newgrp->pmap.use_map = false;
            MPL_free(map);
        } else {
            newgrp->pmap.use_map = true;
            newgrp->pmap.u.map = map;
            /* TODO: build hash to accelerate MPIR_Group_lpid_to_rank */
        }

        *new_group_ptr = newgrp;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Group_create_stride(int size, int rank, MPIR_Session * session_ptr,
                             MPIR_Lpid offset, MPIR_Lpid stride, MPIR_Lpid blocksize,
                             MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (size == 0) {
        /* See 5.3.2, Group Constructors.  For many group routines,
         * the standard explicitly says to return MPI_GROUP_EMPTY;
         * for others it is implied */
        *new_group_ptr = MPIR_Group_empty;
    } else {
        /* NOTE: stride may be negative */
        MPIR_Assert(offset >= 0);
        MPIR_Assert(stride != 0);
        MPIR_Assert(blocksize > 0);

        MPIR_Group *newgrp;
        mpi_errno = MPIR_Group_create(size, &newgrp);
        MPIR_ERR_CHECK(mpi_errno);

        newgrp->rank = rank;
        MPIR_Group_set_session_ptr(newgrp, session_ptr);

        newgrp->pmap.use_map = false;
        newgrp->pmap.u.stride.offset = offset;
        newgrp->pmap.u.stride.stride = stride;
        newgrp->pmap.u.stride.blocksize = blocksize;

        *new_group_ptr = newgrp;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmap_lpid_to_rank(struct MPIR_Pmap *pmap, MPIR_Lpid lpid);
static MPIR_Lpid pmap_rank_to_lpid(struct MPIR_Pmap *pmap, int rank);

int MPIR_Group_lpid_to_rank(MPIR_Group * group, MPIR_Lpid lpid)
{
    return pmap_lpid_to_rank(&group->pmap, lpid);
}

MPIR_Lpid MPIR_Group_rank_to_lpid(MPIR_Group * group, int rank)
{
    return pmap_rank_to_lpid(&group->pmap, rank);
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
    MPIR_CHKLMEM_DECL();

    MPIR_Assert(group_ptr != NULL);

    int vsize = comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM ? comm_ptr->local_size :
        comm_ptr->remote_size;

    /* Initialize the vmap */
    MPIR_Lpid *vmap;
    MPIR_CHKLMEM_MALLOC(vmap, vsize * sizeof(MPIR_Lpid));
    for (int i = 0; i < vsize; i++) {
        /* FIXME: MPID_Comm_get_lpid to be removed */
        uint64_t dev_lpid;
        MPID_Comm_get_lpid(comm_ptr, i, &dev_lpid, FALSE);
        MPIR_Assert((dev_lpid >> 32) == 0);
        vmap[i] = dev_lpid;
    }

    for (int rank = 0; rank < group_ptr->size; rank++) {
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group_ptr, rank);
        bool found = false;
        for (int i = 0; i < vsize; i++) {
            if (vmap[i] == lpid) {
                found = true;
                break;
            }
        }
        if (!found) {
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_GROUP, "**groupnotincomm",
                          "**groupnotincomm %d", rank);
            goto fn_fail;
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* HAVE_ERROR_CHECKING */

void MPIR_Group_set_session_ptr(MPIR_Group * group_ptr, MPIR_Session * session_ptr)
{
    /* Set the session pointer of the group
     * and increase ref counter of the session */
    if (session_ptr != NULL) {
        group_ptr->session_ptr = session_ptr;
        MPIR_Session_add_ref(session_ptr);
    }
}

/* internal static routines */

static bool check_map_is_strided(int size, MPIR_Lpid * map,
                                 MPIR_Lpid * offset_out, MPIR_Lpid * stride_out,
                                 MPIR_Lpid * blocksize_out)
{
    MPIR_Assert(size > 0);
    if (size == 1) {
        *offset_out = map[0];
        *stride_out = 1;
        *blocksize_out = 1;
        return true;
    } else {
        MPIR_Lpid offset, stride, blocksize;
        offset = map[0];

        blocksize = 1;
        for (int i = 1; i < size; i++) {
            if (map[i] - map[i - 1] == 1) {
                blocksize++;
            } else {
                break;
            }
        }
        if (blocksize == size) {
            /* consecutive */
            *offset_out = offset;
            *stride_out = 1;
            *blocksize_out = 1;
            return true;
        } else {
            /* NOTE: stride may be negative */
            stride = map[blocksize] - map[0];
            int n_strides = (size + blocksize - 1) / blocksize;
            int k = 0;
            for (int i = 0; i < n_strides; i++) {
                for (int j = 0; j < blocksize; j++) {
                    if (map[k] != offset + i * stride + j) {
                        return false;
                    }
                    k++;
                    if (k == size) {
                        break;
                    }
                }
            }
            *offset_out = offset;
            *stride_out = stride;
            *blocksize_out = blocksize;
            return true;
        }
    }
}

static MPIR_Lpid pmap_rank_to_lpid(struct MPIR_Pmap *pmap, int rank)
{
    if (rank < 0 || rank >= pmap->size) {
        return MPI_UNDEFINED;
    }

    if (pmap->use_map) {
        return pmap->u.map[rank];
    } else {
        MPIR_Lpid i_blk = rank / pmap->u.stride.blocksize;
        MPIR_Lpid r_blk = rank % pmap->u.stride.blocksize;
        return pmap->u.stride.offset + i_blk * pmap->u.stride.stride + r_blk;
    }
}

static int pmap_lpid_to_rank(struct MPIR_Pmap *pmap, MPIR_Lpid lpid)
{
    if (pmap->use_map) {
        /* Use linear search for now.
         * Optimization: build hash map in MPIR_Group_create_map and do O(1) hash lookup
         */
        for (int rank = 0; rank < pmap->size; rank++) {
            if (pmap->u.map[rank] == lpid) {
                return rank;
            }
        }
        return MPI_UNDEFINED;
    } else {
        lpid -= pmap->u.stride.offset;
        MPIR_Lpid i_blk = lpid / pmap->u.stride.stride;
        MPIR_Lpid r_blk = lpid % pmap->u.stride.stride;
        /* NOTE: stride could be negative, in which case, make sure r_blk >= 0 */
        if (r_blk < 0) {
            MPIR_Assert(pmap->u.stride.stride < 0);
            r_blk -= pmap->u.stride.stride;
            i_blk += 1;
        }

        if (i_blk < 0) {
            return MPI_UNDEFINED;
        }

        if (r_blk >= pmap->u.stride.blocksize) {
            return MPI_UNDEFINED;
        }

        int rank = i_blk * pmap->u.stride.blocksize + r_blk;
        if (rank >= 0 && rank < pmap->size) {
            return rank;
        } else {
            return MPI_UNDEFINED;
        }
    }
}
