/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "group.h"

/* Global world list.
 * world_idx, part of MPIR_Lpid, points to this array */
#define MPIR_MAX_WORLDS 1024
static int num_worlds = 0;
struct MPIR_World MPIR_Worlds[MPIR_MAX_WORLDS];

int MPIR_add_world(const char *namespace, int num_procs)
{
    int world_idx = num_worlds++;

    MPL_strncpy(MPIR_Worlds[world_idx].namespace, namespace, MPIR_NAMESPACE_MAX);
    MPIR_Worlds[world_idx].num_procs = num_procs;

    return world_idx;
}

int MPIR_find_world(const char *namespace)
{
    for (int i = 0; i < num_worlds; i++) {
        if (strncmp(MPIR_Worlds[i].namespace, namespace, MPIR_NAMESPACE_MAX) == 0) {
            return i;
        }
    }
    return -1;
}

/* Preallocated group objects */
MPIR_Group MPIR_Group_builtin[MPIR_GROUP_N_BUILTIN];
MPIR_Group MPIR_Group_direct[MPIR_GROUP_PREALLOC];

MPIR_Object_alloc_t MPIR_Group_mem = { 0, 0, 0, 0, 0, 0, 0, MPIR_GROUP,
    sizeof(MPIR_Group), MPIR_Group_direct,
    MPIR_GROUP_PREALLOC,
    {0}
};

MPIR_Group *const MPIR_Group_empty = &MPIR_Group_builtin[0];

int MPIR_Group_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(MPIR_GROUP_N_BUILTIN == 3);     /* update this func if this ever triggers */

    struct MPIR_Pmap *pmap;

    MPIR_Group_builtin[0].handle = MPI_GROUP_EMPTY;
    MPIR_Object_set_ref(&MPIR_Group_builtin[0], 1);
    MPIR_Group_builtin[0].size = 0;
    MPIR_Group_builtin[0].rank = MPI_UNDEFINED;
    MPIR_Group_builtin[0].session_ptr = NULL;

    MPIR_Group_builtin[0].pmap.use_map = false;
    MPIR_Group_builtin[0].pmap.u.stride.offset = 0;
    MPIR_Group_builtin[0].pmap.u.stride.stride = 1;

    MPIR_Group_builtin[1].handle = MPIR_GROUP_WORLD;
    MPIR_Object_set_ref(&MPIR_Group_builtin[1], 1);
    MPIR_Group_builtin[1].size = MPIR_Process.size;
    MPIR_Group_builtin[1].rank = MPIR_Process.rank;
    MPIR_Group_builtin[1].session_ptr = NULL;
    pmap = &MPIR_Group_builtin[1].pmap;
    pmap->use_map = false;
    pmap->u.stride.offset = 0;
    pmap->u.stride.stride = 1;

    MPIR_Group_builtin[2].handle = MPIR_GROUP_SELF;
    MPIR_Object_set_ref(&MPIR_Group_builtin[2], 1);
    MPIR_Group_builtin[2].size = 1;
    MPIR_Group_builtin[2].rank = 0;
    MPIR_Group_builtin[2].session_ptr = NULL;
    pmap = &MPIR_Group_builtin[2].pmap;
    pmap->use_map = false;
    pmap->u.stride.offset = MPIR_Process.rank;
    pmap->u.stride.stride = 1;

    return mpi_errno;
}

void MPIR_Group_finalize(void)
{
    num_worlds = 0;
}

int MPIR_Group_release(MPIR_Group * group_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    /* MPIR_Group_empty was not properly reference counted - FIXME */
    if (group_ptr == MPIR_Group_empty) {
        goto fn_exit;
    }

    int inuse;
    MPIR_Group_release_ref(group_ptr, &inuse);
    if (!inuse) {
        MPIR_Assert(!HANDLE_IS_BUILTIN(group_ptr->handle));
        /* Only if refcount is 0 do we actually free. */
        if (group_ptr->pmap.use_map) {
            MPL_free(group_ptr->pmap.u.map);
        }
        if (group_ptr->session_ptr != NULL) {
            /* Release session */
            MPIR_Session_release(group_ptr->session_ptr);
        }
#ifdef MPID_DEV_GROUP_DECL
        mpi_errno = MPID_Group_free_hook(group_ptr);
#endif
        MPIR_Handle_obj_free(&MPIR_Group_mem, group_ptr);
    }

  fn_exit:
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
#ifdef MPID_DEV_GROUP_DECL
    mpi_errno = MPID_Group_init_hook(*new_group_ptr);
#endif

    return mpi_errno;
}

/* Internally the only reason to duplicate a group is to copy from NULL session to a new session.
 * Otherwise, we can just use the same group and increment the reference count.
 */
int MPIR_Group_dup(MPIR_Group * old_group, MPIR_Session * session_ptr, MPIR_Group ** new_group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *new_group;

    new_group = (MPIR_Group *) MPIR_Handle_obj_alloc(&MPIR_Group_mem);
    if (!new_group) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, "MPIR_Group_dup",
                                         __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_fail;
    }
    MPIR_Object_set_ref(new_group, 1);

    /* initialize fields */
    new_group->size = old_group->size;
    new_group->rank = old_group->rank;
    MPIR_Group_set_session_ptr(new_group, session_ptr);
    memcpy(&new_group->pmap, &old_group->pmap, sizeof(struct MPIR_Pmap));
    if (old_group->pmap.use_map) {
        new_group->pmap.u.map = MPL_malloc(old_group->size * sizeof(MPIR_Lpid), MPL_MEM_GROUP);
        MPIR_ERR_CHKANDJUMP(!new_group->pmap.u.map, mpi_errno, MPI_ERR_OTHER, "**nomem");
        memcpy(new_group->pmap.u.map, old_group->pmap.u.map, old_group->size * sizeof(MPIR_Lpid));
    }

    *new_group_ptr = new_group;
#ifdef MPID_DEV_GROUP_DECL
    mpi_errno = MPID_Group_init_hook(*new_group_ptr);
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static bool check_map_is_strided(int size, MPIR_Lpid * map,
                                 MPIR_Lpid * offset_out, MPIR_Lpid * stride_out);

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
        MPIR_Group_add_ref(*new_group_ptr);
        goto fn_exit;
    } else {
        MPIR_Group *newgrp;
        mpi_errno = MPIR_Group_create(size, &newgrp);
        MPIR_ERR_CHECK(mpi_errno);

        newgrp->rank = rank;
        MPIR_Group_set_session_ptr(newgrp, session_ptr);

        if (check_map_is_strided(size, map, &newgrp->pmap.u.stride.offset,
                                 &newgrp->pmap.u.stride.stride)) {
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
                             MPIR_Lpid offset, MPIR_Lpid stride, MPIR_Group ** new_group_ptr)
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

        MPIR_Group *newgrp;
        mpi_errno = MPIR_Group_create(size, &newgrp);
        MPIR_ERR_CHECK(mpi_errno);

        newgrp->rank = rank;
        MPIR_Group_set_session_ptr(newgrp, session_ptr);

        newgrp->pmap.use_map = false;
        newgrp->pmap.u.stride.offset = offset;
        newgrp->pmap.u.stride.stride = stride;

        *new_group_ptr = newgrp;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int pmap_lpid_to_rank(struct MPIR_Pmap *pmap, int size, MPIR_Lpid lpid);

int MPIR_Group_lpid_to_rank(MPIR_Group * group, MPIR_Lpid lpid)
{
    return pmap_lpid_to_rank(&group->pmap, group->size, lpid);
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
    MPIR_Assert(group_ptr != NULL);

    for (int rank = 0; rank < group_ptr->size; rank++) {
        MPIR_Lpid lpid = MPIR_Group_rank_to_lpid(group_ptr, rank);
        int r = MPIR_Group_lpid_to_rank(comm_ptr->local_group, lpid);
        if (r == MPI_UNDEFINED) {
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_GROUP, "**groupnotincomm",
                                 "**groupnotincomm %d", rank);
        }
    }

  fn_exit:
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
                                 MPIR_Lpid * offset_out, MPIR_Lpid * stride_out)
{
    MPIR_Assert(size > 0);
    for (int i = 0; i < size; i++) {
        MPIR_Assert(map[i] != MPI_UNDEFINED);
    }
    if (size == 1) {
        *offset_out = map[0];
        *stride_out = 1;
        return true;
    } else {
        MPIR_Lpid offset, stride;
        offset = map[0];
        stride = map[1] - map[0];
        for (int i = 1; i < size; i++) {
            if (map[i] - map[i - 1] != stride) {
                return false;
            }
        }
        *offset_out = offset;
        *stride_out = stride;
        return true;
    }
}

static int pmap_lpid_to_rank(struct MPIR_Pmap *pmap, int size, MPIR_Lpid lpid)
{
    if (pmap->use_map) {
        /* Use linear search for now.
         * Optimization: build hash map in MPIR_Group_create_map and do O(1) hash lookup
         */
        for (int rank = 0; rank < size; rank++) {
            if (pmap->u.map[rank] == lpid) {
                return rank;
            }
        }
        return MPI_UNDEFINED;
    } else {
        /* NOTE: stride could be negative, in which case, make sure r_blk >= 0 */
        int rank = (lpid - pmap->u.stride.offset) / pmap->u.stride.stride;
        if (rank < 0 || rank >= size ||
            lpid != rank * pmap->u.stride.stride + pmap->u.stride.offset) {
            return MPI_UNDEFINED;
        }
        return rank;
    }
}
