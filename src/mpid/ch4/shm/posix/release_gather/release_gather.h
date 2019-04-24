/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef RELEASE_GATHER_H_INCLUDED
#define RELEASE_GATHER_H_INCLUDED

extern zm_atomic_uint_t *MPIDI_POSIX_shm_limit_counter;
extern MPL_shm_hnd_t shm_limit_handle;
extern MPIDI_POSIX_release_gather_tree_type_t MPIDI_POSIX_Bcast_tree_type,
    MPIDI_POSIX_Reduce_tree_type;

/*Blocking wait implementation*/
/* zm_memord_acquire makes sure no writes/reads are reordered before this load */
#define MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN(ptr, value)                           \
    do {                                                           \
        int spin_count = 0;                                        \
        while (zm_atomic_load(ptr, zm_memord_acquire) < (value)) { \
            if (++spin_count >= 10000) {                           \
                /* Call progress only after waiting for a while */ \
                MPID_Progress_test();                              \
                spin_count = 0;                                    \
            }                                                      \
        }                                                          \
    }                                                              \
    while (0)
#define MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE (sizeof(zm_atomic_uint_t))
/* 1 cache_line each for gather and release flag */
#define MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK (MPIDU_SHM_CACHE_LINE_LEN * 2)
#define MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_OFFSET (0)
#define MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_OFFSET (MPIDU_SHM_CACHE_LINE_LEN)
#define MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_ADDR(rank)                                    \
    (((zm_atomic_uint_t *)release_gather_info_ptr->flags_addr) +  \
    ((rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK + MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_OFFSET)/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))
#define MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(rank)                                   \
    (((zm_atomic_uint_t *)release_gather_info_ptr->flags_addr) +  \
    ((rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK + MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_OFFSET)/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))
#define MPIDI_POSIX_RELEASE_GATHER_BCAST_DATA_ADDR(buf)                           \
    (char *) release_gather_info_ptr->bcast_buf_addr + \
    (buf * MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE)
#define MPIDI_POSIX_RELEASE_GATHER_REDUCE_DATA_ADDR(rank, buf)                         \
    (((char *) release_gather_info_ptr->reduce_buf_addr) +  \
    (rank * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE) + \
    (buf * MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE))
#define MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE \
    (MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE / MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS)
#define MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE \
    (MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE / MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS)

int MPIDI_POSIX_mpi_release_gather_comm_init_null(MPIR_Comm * comm_ptr);
int MPIDI_POSIX_mpi_release_gather_comm_init(MPIR_Comm * comm_ptr,
                                             const MPIDI_POSIX_release_gather_opcode_t operation);
int MPIDI_POSIX_mpi_release_gather_comm_free(MPIR_Comm * comm_ptr);

/* Release step of the release_gather framework. This is top-down step in the release_tree.
 * Parent notifies the children to go, once it arrives. In case of Bcast, root places the data in
 * shm bcast buffer before notifying the children. Children copy the data out of shm buffer when
 * notified by the parent */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_release_gather_release(void *local_buf,
                                                                    const int count,
                                                                    MPI_Datatype datatype,
                                                                    const int root,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const
                                                                    MPIDI_POSIX_release_gather_opcode_t
                                                                    operation)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_RELEASE_GATHER_RELEASE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_RELEASE_GATHER_RELEASE);

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIDI_POSIX_release_gather_comm_t *release_gather_info_ptr;
    int segment, rank;
    void *bcast_data_addr = NULL;
    volatile zm_atomic_uint_t *parent_flag_addr;
    /* Set the relaxation to 0 because in Bcast, gather step is "relaxed" to make sure multiple
     * buffers can be used to pipeline the copying in and out of shared memory, and data is not
     * overwritten */
    const int relaxation =
        (operation ==
         MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) ? 0 : MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS - 1;

    rank = MPIR_Comm_rank(comm_ptr);
    release_gather_info_ptr = MPIDI_POSIX_COMM(comm_ptr)->release_gather;

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) {
        segment = release_gather_info_ptr->release_state % MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
        bcast_data_addr = MPIDI_POSIX_RELEASE_GATHER_BCAST_DATA_ADDR(segment);

        if (root != 0) {
            /* Root sends data to rank 0 */
            if (rank == root) {
                mpi_errno =
                    MPIC_Send(local_buf, count, datatype, 0, MPIR_BCAST_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            } else if (rank == 0) {
                mpi_errno =
                    MPIC_Recv(bcast_data_addr, count, datatype, root, MPIR_BCAST_TAG, comm_ptr,
                              MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        } else if (rank == 0) {
            mpi_errno = MPIR_Localcopy(local_buf, count, datatype,
                                       bcast_data_addr, count, datatype);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

    release_gather_info_ptr->release_state++;

    if (rank == 0) {
        /* Rank 0 updates its flag when it arrives and data is ready in shm buffer (if bcast) */
        /* zm_memord_release makes sure that the write of data does not get reordered after this
         * store */
        zm_atomic_store((release_gather_info_ptr->release_flag_addr),
                        release_gather_info_ptr->release_state, zm_memord_release);
    } else {
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) {
            parent_flag_addr =
                MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(release_gather_info_ptr->
                                                             bcast_tree.parent);
        } else {
            parent_flag_addr =
                MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(release_gather_info_ptr->
                                                             reduce_tree.parent);
        }

        /* Wait until the parent has updated its flag */
        MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN(parent_flag_addr,
                                                        release_gather_info_ptr->release_state -
                                                        relaxation);
        /* Update its own flag */
        /* zm_memord_release makes sure that the read of parent's flag does not get reordered after
         * this store */
        zm_atomic_store((release_gather_info_ptr->release_flag_addr),
                        release_gather_info_ptr->release_state, zm_memord_release);
    }

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) {
        /* Non-root ranks copy data from shm buffer to user buffer */
        if (rank != root) {
            MPIR_ERR_CHKANDJUMP(!bcast_data_addr, mpi_errno, MPI_ERR_OTHER, "**nomem");
            mpi_errno = MPIR_Localcopy(bcast_data_addr, count, datatype,
                                       local_buf, count, datatype);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_MPI_RELEASE_GATHER_RELEASE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Gather step of the release_gather framework. This is bottom-up step in the gather_tree.
 * Children notify the parent when it arrives. In case of Reduce, each rank places its data in shm
 * reduce buffer. A parent reduces all its children data with its own before notifying its parent. */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_release_gather_gather(const void *inbuf, void *outbuf,
                                                                   const int count,
                                                                   MPI_Datatype datatype, MPI_Op op,
                                                                   const int root,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_POSIX_release_gather_opcode_t
                                                                   operation)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_RELEASE_GATHER_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_RELEASE_GATHER_GATHER);

    MPIDI_POSIX_release_gather_comm_t *release_gather_info_ptr;
    int segment, rank, num_children;
    volatile void *child_data_addr;
    volatile zm_atomic_uint_t child_gather_flag, *child_flag_addr;
    volatile void *reduce_data_addr = NULL;
    int i, mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    bool skip_checking = false;
    /* Set the relaxation to 0 because in Reduce, release step is "relaxed" to make sure multiple
     * buffers can be used to pipeline the copying in and out of shared memory, and data is not
     * overwritten */
    const int relaxation =
        (operation ==
         MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) ? 0 : MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS - 1;
    zm_atomic_uint_t min_gather;
    UT_array *children;

    release_gather_info_ptr = MPIDI_POSIX_COMM(comm_ptr)->release_gather;
    segment = release_gather_info_ptr->gather_state % MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
    children = release_gather_info_ptr->bcast_tree.children;
    num_children = release_gather_info_ptr->bcast_tree.num_children;
    rank = MPIR_Comm_rank(comm_ptr);

    release_gather_info_ptr->gather_state++;
    min_gather = release_gather_info_ptr->gather_state;

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
        if (rank == 0) {
            /* Rank 0 reduces the data directly in its outbuf. Copy the data from inbuf to outbuf
             * if needed */
            if (inbuf != outbuf) {
                mpi_errno = MPIR_Localcopy(inbuf, count, datatype, outbuf, count, datatype);
            }
            reduce_data_addr = outbuf;
        } else {
            reduce_data_addr = MPIDI_POSIX_RELEASE_GATHER_REDUCE_DATA_ADDR(rank, segment);
            /* Copy data from user buffer to shared buffer */
            mpi_errno =
                MPIR_Localcopy(inbuf, count, datatype, (void *) reduce_data_addr, count, datatype);
        }
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        num_children = release_gather_info_ptr->reduce_tree.num_children;
        children = release_gather_info_ptr->reduce_tree.children;
    }

    /* Avoid checking for availabilty of next buffer if it is guaranteed to be available */
    /* zm_memord_acquire makes sure no writes/reads are reordered before this load */
    if ((operation != MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) &&
        (zm_atomic_load(release_gather_info_ptr->gather_flag_addr, zm_memord_acquire)) >=
        (release_gather_info_ptr->gather_state - relaxation)) {
        skip_checking = true;
    }

    /* Leaf nodes never skip checking */
    if (num_children == 0 || !skip_checking) {
        for (i = 0; i < num_children; i++) {
            MPIR_ERR_CHKANDJUMP(!utarray_eltptr(children, i), mpi_errno, MPI_ERR_OTHER, "**nomem");
            child_flag_addr =
                MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_ADDR(*utarray_eltptr(children, i));
            /* Wait until the child has arrived */
            MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN(child_flag_addr,
                                                            release_gather_info_ptr->gather_state -
                                                            relaxation);

            if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
                child_data_addr =
                    (char *) release_gather_info_ptr->child_reduce_buf_addr[i] +
                    segment * MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE;
                /* zm_memord_acquire in MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN makes sure
                 * that the reduce_local call does not get reordered before read of children's flag
                 * in MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN */
                mpi_errno =
                    MPIR_Reduce_local((void *) child_data_addr, (void *) reduce_data_addr,
                                      count, datatype, op);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            /* Read child_flag_addr which 'may' be larger than the strongest waiting condition
             * so, it is safe */
            child_gather_flag = *child_flag_addr;
            min_gather = MPL_MIN(child_gather_flag, min_gather);
        }
        /* zm_memord_release makes sure that the write of data (reduce_local) does not get
         * reordered after this store */
        zm_atomic_store((release_gather_info_ptr->gather_flag_addr), min_gather, zm_memord_release);
    }

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
        if (root != 0) {
            /* send-recv between root and rank 0 */
            if (rank == root) {
                mpi_errno =
                    MPIC_Recv(outbuf, count, datatype, 0, MPIR_REDUCE_TAG, comm_ptr,
                              MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            } else if (rank == 0) {
                MPIR_ERR_CHKANDJUMP(!reduce_data_addr, mpi_errno, MPI_ERR_OTHER, "**nomem");
                mpi_errno =
                    MPIC_Send((void *) reduce_data_addr, count, datatype, root, MPIR_REDUCE_TAG,
                              comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
        /* No data copy is required if root was rank 0, because it reduced the data directly in its
         * outbuf */
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_MPI_RELEASE_GATHER_GATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* RELEASE_GATHER_H_INCLUDED */
