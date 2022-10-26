/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef RELEASE_GATHER_H_INCLUDED
#define RELEASE_GATHER_H_INCLUDED

extern MPL_atomic_uint64_t *MPIDI_POSIX_shm_limit_counter;
extern MPL_shm_hnd_t shm_limit_handle;
extern MPIDI_POSIX_release_gather_tree_type_t MPIDI_POSIX_Bcast_tree_type,
    MPIDI_POSIX_Reduce_tree_type;

#define RELEASE_GATHER_FIELD(comm, field)                   \
    MPIDI_POSIX_COMM(comm, release_gather).field

#define NB_RELEASE_GATHER_FIELD(comm, field)                   \
    MPIDI_POSIX_COMM(comm, nb_release_gather).field

/* Blocking wait implementation */
/* "acquire" makes sure no writes/reads are reordered before this load */
#define MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN(ptr, value)                           \
    do {                                                           \
        int spin_count = 0;                                        \
        while (MPL_atomic_acquire_load_uint64(ptr) < (value))    { \
            if (++spin_count >= MPIR_CVAR_POSIX_POLL_FREQUENCY) {    \
                /* Call progress only after waiting for a while */ \
                MPID_Progress_test(NULL);                              \
                spin_count = 0;                                    \
            }                                                      \
        }                                                          \
    }                                                              \
    while (0)
#define MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE (sizeof(MPL_atomic_uint64_t))
/* 1 cache_line each for gather and release flag */
#define MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK (MPIDU_SHM_CACHE_LINE_LEN * 2)
#define MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_OFFSET (0)
#define MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_OFFSET (MPIDU_SHM_CACHE_LINE_LEN)
#define MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_ADDR(rank)                                    \
    (((MPL_atomic_uint64_t *)release_gather_info_ptr->flags_addr) +  \
    ((rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK + MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_OFFSET)/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))
#define MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(rank)                                   \
    (((MPL_atomic_uint64_t *)release_gather_info_ptr->flags_addr) +  \
    ((rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK + MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_OFFSET)/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))
#define MPIDI_POSIX_RELEASE_GATHER_BCAST_DATA_ADDR(buf)                           \
    (char *) release_gather_info_ptr->bcast_buf_addr + \
    (buf * MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE)
#define MPIDI_POSIX_RELEASE_GATHER_REDUCE_DATA_ADDR(rank, buf)                         \
    (((char *) release_gather_info_ptr->reduce_buf_addr) +  \
    (rank * RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size)) + \
    (buf * MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE))
#define MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE \
    (RELEASE_GATHER_FIELD(comm_ptr, bcast_shm_size) / RELEASE_GATHER_FIELD(comm_ptr, bcast_num_cells))
#define MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE \
    (RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size) / RELEASE_GATHER_FIELD(comm_ptr, reduce_num_cells))

int MPIDI_POSIX_mpi_release_gather_comm_init_null(MPIR_Comm * comm_ptr);
int MPIDI_POSIX_mpi_release_gather_comm_init(MPIR_Comm * comm_ptr,
                                             const MPIDI_POSIX_release_gather_opcode_t operation);
int MPIDI_POSIX_mpi_release_gather_comm_free(MPIR_Comm * comm_ptr);

/* Release step of the release_gather framework. This is top-down step in the release_tree.
 * Parent notifies the children to go, once it arrives. In case of Bcast, root places the data in
 * shm bcast buffer before notifying the children. Children copy the data out of shm buffer when
 * notified by the parent */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_release_gather_release(void *local_buf,
                                                                    MPI_Aint count,
                                                                    MPI_Datatype datatype,
                                                                    const int root,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const
                                                                    MPIDI_POSIX_release_gather_opcode_t
                                                                    operation)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIDI_POSIX_release_gather_comm_t *release_gather_info_ptr;
    int segment, rank;
    void *bcast_data_addr = NULL;
    MPL_atomic_uint64_t *parent_flag_addr;
    /* Set the relaxation to 0 because in Bcast, gather step is "relaxed" to make sure multiple
     * buffers can be used to pipeline the copying in and out of shared memory, and data is not
     * overwritten */
    const int relaxation =
        (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) ?
        RELEASE_GATHER_FIELD(comm_ptr, reduce_num_cells) - 1 : 0;

    rank = MPIR_Comm_rank(comm_ptr);
    release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, release_gather);
    release_gather_info_ptr->release_state++;

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) {
        segment = release_gather_info_ptr->release_state %
            RELEASE_GATHER_FIELD(comm_ptr, bcast_num_cells);
        bcast_data_addr = MPIDI_POSIX_RELEASE_GATHER_BCAST_DATA_ADDR(segment);

        if (root != 0) {
            /* Root sends data to rank 0 */
            if (rank == root) {
                mpi_errno =
                    MPIC_Send(local_buf, count, datatype, 0, MPIR_BCAST_TAG, comm_ptr, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
            } else if (rank == 0) {
#ifdef HAVE_ERROR_CHECKING
                /* when error checking is enabled, the amount of data sender sent is retrieved from
                 * status. If it does not match the expected datasize, mpi_errno is set. The received
                 * size is placed on shm_buffer, followed by the errflag, followed by actual data
                 * with an offset of (2*cacheline_size) bytes from the starting address */
                MPI_Status status;
                MPI_Aint recv_bytes;
                mpi_errno =
                    MPIC_Recv((char *) bcast_data_addr + 2 * MPIDU_SHM_CACHE_LINE_LEN, count,
                              datatype, root, MPIR_BCAST_TAG, comm_ptr, &status, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
                MPIR_Get_count_impl(&status, MPI_BYTE, &recv_bytes);
                MPIR_Typerep_copy(bcast_data_addr, &recv_bytes, sizeof(int),
                                  MPIR_TYPEREP_FLAG_NONE);
                /* It is necessary to copy the errflag as well to handle the case when non-root
                 * becomes temporary root as part of compositions (or smp aware colls). These temp
                 * roots might expect same data as other ranks but different from the actual root.
                 * So only datasize mismatch handling is not sufficient */
                MPIR_Typerep_copy((char *) bcast_data_addr + MPIDU_SHM_CACHE_LINE_LEN, errflag,
                                  sizeof(MPIR_Errflag_t), MPIR_TYPEREP_FLAG_NONE);
                if ((int) recv_bytes != count) {
                    /* It is OK to compare with count because datatype is always MPI_BYTE for Bcast */
                    *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
#else
                /* When error checking is disabled, MPI_STATUS_IGNORE is used */
                mpi_errno =
                    MPIC_Recv(bcast_data_addr, count, datatype, root, MPIR_BCAST_TAG, comm_ptr,
                              MPI_STATUS_IGNORE, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
#endif
            }
        } else if (rank == 0) {
#ifdef HAVE_ERROR_CHECKING
            /* When error checking is enabled, place the datasize in shm_buf first, followed by the
             * errflag, followed by the actual data with an offset of (2*cacheline_size) bytes from
             * the starting address */
            MPIR_Typerep_copy(bcast_data_addr, &count, sizeof(int), MPIR_TYPEREP_FLAG_NONE);
            /* It is necessary to copy the errflag as well to handle the case when non-root
             * becomes root as part of compositions (or smp aware colls). These roots might
             * expect same data as other ranks but different from the actual root. So only
             * datasize mismatch handling is not sufficient */
            MPIR_Typerep_copy((char *) bcast_data_addr + MPIDU_SHM_CACHE_LINE_LEN, errflag,
                              sizeof(MPIR_Errflag_t), MPIR_TYPEREP_FLAG_NONE);
            mpi_errno =
                MPIR_Localcopy(local_buf, count, datatype,
                               (char *) bcast_data_addr + 2 * MPIDU_SHM_CACHE_LINE_LEN, count,
                               datatype);
#else
            mpi_errno = MPIR_Localcopy(local_buf, count, datatype,
                                       bcast_data_addr, count, datatype);
#endif
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        }
    }

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_ALLREDUCE) {
        /* For allreduce, ranks directly copy the data from the reduce buffer of rank 0 */
        segment = release_gather_info_ptr->release_state %
            RELEASE_GATHER_FIELD(comm_ptr, reduce_num_cells);
        bcast_data_addr = MPIDI_POSIX_RELEASE_GATHER_REDUCE_DATA_ADDR(0, segment);
    }

    if (rank == 0) {
        /* Rank 0 updates its flag when it arrives and data is ready in shm buffer (if bcast) */
        /* "release" makes sure that the write of data does not get reordered after this
         * store */
        MPL_atomic_release_store_uint64(release_gather_info_ptr->release_flag_addr,
                                        release_gather_info_ptr->release_state);
    } else {
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
            parent_flag_addr =
                MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(release_gather_info_ptr->
                                                             reduce_tree.parent);
        } else {
            parent_flag_addr =
                MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(release_gather_info_ptr->
                                                             bcast_tree.parent);
        }

        /* Wait until the parent has updated its flag */
        MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN(parent_flag_addr,
                                                        release_gather_info_ptr->release_state -
                                                        relaxation);
        /* Update its own flag */
        /* "release" makes sure that the read of parent's flag does not get reordered after
         * this store */
        MPL_atomic_release_store_uint64(release_gather_info_ptr->release_flag_addr,
                                        release_gather_info_ptr->release_state);
    }

    if (((operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) && (rank != root)) ||
        (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_ALLREDUCE)) {
        /* For bcast only non-root ranks copy data from shm buffer to user buffer and in case of
         * allreduce all ranks copy data from shm buffer to their user buffer */
        MPIR_ERR_CHKANDJUMP(!bcast_data_addr, mpi_errno, MPI_ERR_OTHER, "**nomem");
#ifdef HAVE_ERROR_CHECKING
        if ((operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) && (rank != root)) {
            /* When error checking is enabled, collective is Bcast, and rank is not root,
             * datasize is copied out from shm_buffer and compared against the count a rank was
             * expecting. Also, the errflag is copied out. In case of mismatch mpi_errno is set.
             * Actual data starts after (2*cacheline_size) bytes */
            int recv_bytes, recv_errflag;
            MPIR_Typerep_copy(&recv_bytes, bcast_data_addr, sizeof(int), MPIR_TYPEREP_FLAG_NONE);
            MPIR_Typerep_copy(&recv_errflag, (char *) bcast_data_addr + MPIDU_SHM_CACHE_LINE_LEN,
                              sizeof(int), MPIR_TYPEREP_FLAG_NONE);
            if (recv_bytes != count || recv_errflag != MPI_SUCCESS) {
                /* It is OK to compare with count because datatype is always MPI_BYTE for Bcast */
                *errflag = MPIR_ERR_OTHER;
                MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**collective_size_mismatch",
                              "**collective_size_mismatch %d %d", recv_bytes, count);
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            mpi_errno =
                MPIR_Localcopy((char *) bcast_data_addr + 2 * MPIDU_SHM_CACHE_LINE_LEN, count,
                               datatype, local_buf, count, datatype);
        } else {
            mpi_errno =
                MPIR_Localcopy(bcast_data_addr, count, datatype, local_buf, count, datatype);
        }
#else
        mpi_errno = MPIR_Localcopy(bcast_data_addr, count, datatype, local_buf, count, datatype);
#endif
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno_ret;
  fn_fail:
    goto fn_exit;
}

/* Gather step of the release_gather framework. This is bottom-up step in the gather_tree.
 * Children notify the parent when it arrives. In case of Reduce, each rank places its data in shm
 * reduce buffer. A parent reduces all its children data with its own before notifying its parent. */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_release_gather_gather(const void *inbuf, void *outbuf,
                                                                   MPI_Aint count,
                                                                   MPI_Datatype datatype, MPI_Op op,
                                                                   const int root,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_POSIX_release_gather_opcode_t
                                                                   operation)
{
    MPIR_FUNC_ENTER;

    MPIDI_POSIX_release_gather_comm_t *release_gather_info_ptr;
    int segment, rank, num_children;
    void *child_data_addr;
    MPL_atomic_uint64_t *child_flag_addr;
    void *reduce_data_addr = NULL;
    int i, mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    bool skip_checking = false;
    /* Set the relaxation to 0 because in Reduce, release step is "relaxed" to make sure multiple
     * buffers can be used to pipeline the copying in and out of shared memory, and data is not
     * overwritten */
    const int relaxation =
        (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) ?
        RELEASE_GATHER_FIELD(comm_ptr, bcast_num_cells) - 1 : 0;
    uint64_t min_gather, child_gather_flag;
    UT_array *children;
    void *temp_recvbuf = NULL;

    release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, release_gather);
    children = release_gather_info_ptr->bcast_tree.children;
    num_children = release_gather_info_ptr->bcast_tree.num_children;
    rank = MPIR_Comm_rank(comm_ptr);

    release_gather_info_ptr->gather_state++;
    min_gather = release_gather_info_ptr->gather_state;
    segment = release_gather_info_ptr->gather_state %
        RELEASE_GATHER_FIELD(comm_ptr, reduce_num_cells);

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE ||
        operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_ALLREDUCE) {
        if (rank == 0 && operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
            /* non-root need a temporary recv buf */
            if (rank != root) {
                temp_recvbuf = MPIR_alloc_buffer(count, datatype);
                outbuf = temp_recvbuf;
            }
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
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        num_children = release_gather_info_ptr->reduce_tree.num_children;
        children = release_gather_info_ptr->reduce_tree.children;
    }

    /* Avoid checking for availability of next buffer if it is guaranteed to be available */
    /* "acquire" makes sure no writes/reads are reordered before this load */
    if ((operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) &&
        (MPL_atomic_acquire_load_uint64(release_gather_info_ptr->gather_flag_addr)) >=
        (release_gather_info_ptr->gather_state - relaxation)) {
        skip_checking = true;
    }

    /* Leaf nodes never skip checking */
    if (num_children == 0 || !skip_checking) {
        for (i = 0; i < num_children; i++) {
            int child_rank = *(int *) utarray_eltptr(children, i);
            child_flag_addr = MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_ADDR(child_rank);
            /* Wait until the child has arrived */
            MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN(child_flag_addr,
                                                            release_gather_info_ptr->gather_state -
                                                            relaxation);

            if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE ||
                operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_ALLREDUCE) {
                child_data_addr =
                    (char *) release_gather_info_ptr->child_reduce_buf_addr[i] +
                    segment * MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE;
                /* zm_memord_acquire in MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN makes sure
                 * that the reduce_local call does not get reordered before read of children's flag
                 * in MPIDI_POSIX_RELEASE_GATHER_WAIT_WHILE_LESS_THAN */
                mpi_errno =
                    MPIR_Reduce_local((void *) child_data_addr, (void *) reduce_data_addr,
                                      count, datatype, op);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
            }
            /* Read child_flag_addr which 'may' be larger than the strongest waiting condition
             * so, it is safe */
            child_gather_flag = MPL_atomic_acquire_load_uint64(child_flag_addr);
            min_gather = MPL_MIN(child_gather_flag, min_gather);
        }
        /* "release" makes sure that the write of data (reduce_local) does not get
         * reordered after this store */
        MPL_atomic_release_store_uint64((release_gather_info_ptr->gather_flag_addr), min_gather);
    }

    if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
        if (root != 0) {
            /* send-recv between root and rank 0 */
            if (rank == root) {
                mpi_errno =
                    MPIC_Recv(outbuf, count, datatype, 0, MPIR_REDUCE_TAG, comm_ptr,
                              MPI_STATUS_IGNORE, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
            } else if (rank == 0) {
                MPIR_ERR_CHKANDJUMP(!reduce_data_addr, mpi_errno, MPI_ERR_OTHER, "**nomem");
                mpi_errno =
                    MPIC_Send((void *) reduce_data_addr, count, datatype, root, MPIR_REDUCE_TAG,
                              comm_ptr, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
            }
        }
        /* No data copy is required if root was rank 0, because it reduced the data directly in its
         * outbuf */
    }

  fn_exit:
    if (temp_recvbuf) {
        MPL_free(temp_recvbuf);
    }
    MPIR_FUNC_EXIT;
    return mpi_errno_ret;
  fn_fail:
    goto fn_exit;
}

#endif /* RELEASE_GATHER_H_INCLUDED */
