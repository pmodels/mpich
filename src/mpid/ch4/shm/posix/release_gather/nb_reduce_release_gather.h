/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef NB_REDUCE_RELEASE_GATHER_H_INCLUDED
#define NB_REDUCE_RELEASE_GATHER_H_INCLUDED

#include "gentran_types.h"

#define MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_GATHER_FLAG_ADDR(rank, segment, num_ranks)        \
    (((MPL_atomic_uint64_t *)nb_release_gather_info_ptr->ireduce_flags_addr) +                     \
     (((segment * num_ranks * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK) +                 \
       (rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK +                                 \
        MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_OFFSET))/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))

#define MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_RELEASE_FLAG_ADDR(rank, segment, num_ranks)       \
    (((MPL_atomic_uint64_t *)nb_release_gather_info_ptr->ireduce_flags_addr) +                     \
     (((segment * num_ranks * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK) +                 \
       (rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK +                                 \
        MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_OFFSET))/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))

#define MPIDI_POSIX_RELEASE_GATHER_NB_REDUCE_DATA_ADDR(rank, buf) \
    (((char *) nb_release_gather_info_ptr->reduce_buf_addr) +     \
    (rank * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE) +       \
    (buf * MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE))

/* The following functions with suffix issue, completion, or cb are used as function pointers in the
 * generic vertex types or callback type vertices created later */

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_rank0_hold_buf_issue(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 1 issue */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ireduce_info_t *) vtxp->u.generic.data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);

    if (rank != 0)
        *done = 1;
    else
        *done = 0;

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_rank0_hold_buf_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 1 completion */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ireduce_info_t *) vtxp->u.generic.data;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    int num_cells = MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
    int segment = per_call_data->seq_no % num_cells;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    int last_seq_no = nb_release_gather_info_ptr->ireduce_last_seq_no_completed[segment];

    MPL_atomic_uint64_t *my_release_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_RELEASE_FLAG_ADDR(rank, segment, num_ranks);
    MPL_atomic_uint64_t *my_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_GATHER_FLAG_ADDR(rank, segment, num_ranks);

    /* Rank 0 keeps checking if the a particular cell is available */
    if (rank == 0 && (MPL_atomic_acquire_load_uint64(my_gather_flag_addr) == -1) &&
        (last_seq_no == per_call_data->seq_no - num_cells || last_seq_no < num_cells)) {
        /* Buffer can be overwritten */
        if (rank == 0) {
            /* Mark the buffer as occupied */
            MPL_atomic_release_store_uint64(my_gather_flag_addr, -2);
            /* Update the release_flag, so that the children ranks can move on */
            MPL_atomic_release_store_uint64(my_release_flag_addr, per_call_data->seq_no);
            *done = 1;
        }
    } else {
        /* Buffer was not ready */
        *done = 0;
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_update_release_flags_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 2 completion */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data = vtxp->u.generic.data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    MPL_atomic_uint64_t *parent_release_flag_addr, *my_release_flag_addr;

    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int segment = per_call_data->seq_no % MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;

    my_release_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_RELEASE_FLAG_ADDR(rank, segment, num_ranks);

    /* Everyone except rank 0 waits for its parent to arrive */
    if (rank != 0) {
        parent_release_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_RELEASE_FLAG_ADDR
            (nb_release_gather_info_ptr->reduce_tree.parent, segment, num_ranks);

        if (MPL_atomic_acquire_load_uint64(parent_release_flag_addr) == per_call_data->seq_no) {
            /* Update my release_flag if the parent has arrived */
            MPL_atomic_release_store_uint64(my_release_flag_addr, per_call_data->seq_no);
        } else {
            *done = 0;
            return MPI_SUCCESS;
        }
    }

    *done = 1;

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_all_datacopy_cb(MPIR_Comm * comm, int tag,
                                                               void *data)
{
    /* Step 3 callback */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data = data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);
    int root = per_call_data->root;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    int segment = per_call_data->seq_no % MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);

    /* If root is rank 0, final data is reduced into the recv_buf directly. It is to avoid extra
     * data copy between shm_buf and recv_buf */
    if (per_call_data->send_buf != per_call_data->recv_buf && rank == 0 && root == 0) {
        /* If MPI_IN_PLACE is not used and root is rank 0, copy data from send_buf to recv_buf */
        MPIR_Localcopy(per_call_data->send_buf, per_call_data->count, per_call_data->datatype,
                       per_call_data->recv_buf, per_call_data->count, per_call_data->datatype);
    } else {
        /* ranks copy data from sendbuf to shmbuf */
        MPIR_Localcopy(per_call_data->send_buf, per_call_data->count, per_call_data->datatype,
                       MPIDI_POSIX_RELEASE_GATHER_NB_REDUCE_DATA_ADDR(rank, segment),
                       per_call_data->count, per_call_data->datatype);
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_gather_step_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 4 completion */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data = vtxp->u.generic.data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int num_ranks = MPIR_Comm_size(comm_ptr);
    UT_array *children;
    int i;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    children = nb_release_gather_info_ptr->reduce_tree.children;
    int num_children = nb_release_gather_info_ptr->reduce_tree.num_children;
    int segment = per_call_data->seq_no % MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
    MPL_atomic_uint64_t *child_gather_flag_addr;

    /* Gather phase begins */
    /* Wait for all my children to arrive */
    for (i = 0; i < num_children; i++) {
        int child_rank = *(int *) utarray_eltptr(children, i);
        child_gather_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_GATHER_FLAG_ADDR(child_rank, segment, num_ranks);
        if (MPL_atomic_acquire_load_uint64(child_gather_flag_addr) != per_call_data->seq_no) {
            /* Set *done to 0 even if a single child has not arrived */
            *done = 0;
            return MPI_SUCCESS;
        }
    }

    *done = 1;

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_reduce_data_cb(MPIR_Comm * comm, int tag, void *data)
{
    /* Step 5 callback */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data = data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int num_ranks = MPIR_Comm_size(comm_ptr);
    int rank = MPIR_Comm_rank(comm_ptr);
    int i;
    int root = per_call_data->root;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int num_children = nb_release_gather_info_ptr->reduce_tree.num_children;
    int segment = per_call_data->seq_no % MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
    MPL_atomic_uint64_t *my_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_GATHER_FLAG_ADDR(rank, segment, num_ranks);
    void *child_data_addr;

    /* All my children have arrived (step 4 completion guarantees that). Reduce the data */
    for (i = 0; i < num_children; i++) {
        child_data_addr =
            (char *) nb_release_gather_info_ptr->child_reduce_buf_addr[i] +
            segment * MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE;
        if (root == 0 && rank == 0) {
            /* If rank 0 is root, reduce directly in recv_buf */
            MPIR_Reduce_local((void *) child_data_addr, per_call_data->recv_buf,
                              per_call_data->count, per_call_data->datatype, per_call_data->op);

        } else {
            /* Reduce in a specific cell in shm_buf */
            MPIR_Reduce_local((void *) child_data_addr,
                              (void *) MPIDI_POSIX_RELEASE_GATHER_NB_REDUCE_DATA_ADDR(rank,
                                                                                      segment),
                              per_call_data->count, per_call_data->datatype, per_call_data->op);
        }
    }

    if (rank == 0 && root == 0) {
        /* Mark the buffer as available, if root was rank 0 */
        MPL_atomic_release_store_uint64(my_gather_flag_addr, -1);
    } else {
        /* Update my gather flag to seq_no (so that my parent can be unblocked) */
        MPL_atomic_release_store_uint64(my_gather_flag_addr, per_call_data->seq_no);
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_reduce_start_sendrecv_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 6 completion */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ireduce_info_t *) vtxp->u.generic.data;
    int root = per_call_data->root;
    int segment = per_call_data->seq_no % MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int rank = MPIR_Comm_rank(comm_ptr);
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    /* If root is not rank 0, rank 0 sends data to root */
    if (root != 0) {
        if (rank == root) {
            MPIC_Irecv(per_call_data->recv_buf, per_call_data->count, per_call_data->datatype,
                       0, per_call_data->tag, comm_ptr, &(per_call_data->rreq));
        } else if (rank == 0) {
            MPIC_Isend(MPIDI_POSIX_RELEASE_GATHER_NB_REDUCE_DATA_ADDR(rank, segment),
                       per_call_data->count, per_call_data->datatype, per_call_data->root,
                       per_call_data->tag, comm_ptr, &(per_call_data->sreq), &errflag);
        }
    }

    *done = 1;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_finish_sendrecv_issue(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 6,7 issue */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ireduce_info_t *) vtxp->u.generic.data;
    int root = per_call_data->root;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);

    if ((root != 0) && (rank == root || rank == 0)) {
        /* Root and rank 0 will have to wait on completion of recv, send respectively in case of
         * non-zero root */
        *done = 0;
        return MPI_SUCCESS;
    }

    *done = 1;

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_reduce_finish_sendrecv_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 7 completion */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ireduce_info_t *) vtxp->u.generic.data;
    int root = per_call_data->root;
    int segment = per_call_data->seq_no % MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    MPL_atomic_uint64_t *my_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_GATHER_FLAG_ADDR(rank, segment, num_ranks);

    if (root != 0) {
        if (rank == root) {
            if (MPIR_Request_is_complete(per_call_data->rreq)) {
                MPIR_Request_free(per_call_data->rreq);
                per_call_data->rreq = NULL;
                *done = 1;
            } else {
                /* Recv is not complete */
                *done = 0;
            }
        } else if (rank == 0) {
            if (MPIR_Request_is_complete(per_call_data->sreq)) {
                MPIR_Request_free(per_call_data->sreq);
                per_call_data->sreq = NULL;
                /* Mark the buffer as available for next use */
                MPL_atomic_release_store_uint64(my_gather_flag_addr, -1);
                /* Update the last seq no completed */
                nb_release_gather_info_ptr->ireduce_last_seq_no_completed[segment] =
                    per_call_data->seq_no;
                *done = 1;
            } else {
                /* Send is not complete */
                *done = 0;
            }
        }
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_ref_count_decrement_free(void *v)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    /* Step 7 free */
    MPIDI_POSIX_per_call_ireduce_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ireduce_info_t *) vtxp->u.generic.data;
    MPIR_Op_release_if_not_builtin(per_call_data->op);
    MPIR_Datatype_release_if_not_builtin(per_call_data->datatype);

    return MPI_SUCCESS;
}

/* Creates the generic type vertices as well as dependencies to implement release step followed
 * by gather step to implement Ireduce */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_nb_release_gather_ireduce_impl(void *send_buf,
                                                                        void *recv_buf,
                                                                        MPI_Aint count,
                                                                        MPI_Datatype datatype,
                                                                        MPI_Op op, const int root,
                                                                        MPIR_Comm * comm_ptr,
                                                                        MPIR_TSP_sched_t sched)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    int tag;
    int first_vtx_id = -1, second_vtx_id, third_vtx_id, fourth_vtx_id, fifth_vtx_id, sixth_vtx_id,
        seventh_vtx_id, prev_vtx_id;
    int reserve_buf_type_id, propagate_release_flag_type_id, gather_type_id, start_sendrecv_type_id,
        finish_sendrecv_type_id;
    MPIDI_POSIX_per_call_ireduce_info_t *data;
    int i;
    MPI_Aint num_chunks, chunk_count_floor, chunk_count_ceil;
    MPI_Aint true_extent, type_size, lb, extent;
    int offset = 0, is_contig;

    /* Register the vertices */
    reserve_buf_type_id = MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_rank0_hold_buf_issue,
                                                  MPIDI_POSIX_NB_RG_rank0_hold_buf_completion,
                                                  NULL);
    propagate_release_flag_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_empty_issue,
                                MPIDI_POSIX_NB_RG_update_release_flags_completion, NULL);
    gather_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_empty_issue,
                                MPIDI_POSIX_NB_RG_gather_step_completion, NULL);
    start_sendrecv_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_finish_sendrecv_issue,
                                MPIDI_POSIX_NB_RG_reduce_start_sendrecv_completion, NULL);
    finish_sendrecv_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_finish_sendrecv_issue,
                                MPIDI_POSIX_NB_RG_reduce_finish_sendrecv_completion,
                                MPIDI_POSIX_NB_RG_ref_count_decrement_free);

    MPIR_Type_get_extent_impl(datatype, &lb, &extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    MPIR_Datatype_is_contig(datatype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        MPIR_Pack_size_impl(1, datatype, comm_ptr, &type_size);
    }

    if (send_buf == MPI_IN_PLACE) {
        send_buf = recv_buf;
    }
    /* Calculate chunking information, using extent handles contiguous and non-contiguous datatypes
     * both */
    MPIR_Algo_calculate_pipeline_chunk_info(MPIDI_POSIX_RELEASE_GATHER_REDUCE_CELLSIZE,
                                            extent, count, &num_chunks,
                                            &chunk_count_floor, &chunk_count_ceil);

    /* Do pipelined release-gather */
    /* A schedule gets created in the form of a forest, where each tree has 7 vertices (to perform
     * release_gather) and number of trees is same as number of chunks the message is divided
     * into. Then, there is a dependence from first vertex of previous chunk to first vertex of next
     * chunk. */
    for (i = 0; i < num_chunks; i++) {
        MPI_Aint chunk_count = (i == 0) ? chunk_count_floor : chunk_count_ceil;
        int n_incoming = 0;
        data = (MPIDI_POSIX_per_call_ireduce_info_t *)
            MPIR_TSP_sched_malloc(sizeof(MPIDI_POSIX_per_call_ireduce_info_t), sched);
        MPIR_ERR_CHKANDJUMP(!data, mpi_errno, MPI_ERR_OTHER, "**nomem");

        data->seq_no = MPIDI_POSIX_COMM(comm_ptr, nb_reduce_seq_no);

        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        MPIR_Datatype_add_ref_if_not_builtin(datatype);
        MPIR_Op_add_ref_if_not_builtin(op);

        /* Every iteration gets a unique sequence number. This is needed to determine which cell in
         * shm_buffer should be used for which iteration */
        MPIDI_POSIX_COMM(comm_ptr, nb_reduce_seq_no)++;

        /* Fill in the per_call data structure */
        data->seq_no = MPIDI_POSIX_COMM(comm_ptr, nb_reduce_seq_no);
        data->send_buf = (char *) send_buf + offset * extent;
        data->recv_buf = (char *) recv_buf + offset * extent;
        data->count = chunk_count;
        data->datatype = datatype;
        data->root = root;
        data->op = op;
        data->comm_ptr = comm_ptr;
        data->tag = tag;
        data->sreq = NULL;
        data->rreq = NULL;

        /* First vertex of next chunk depends on first vertex of previous chunk */
        if (i > 0) {
            n_incoming = 1;
            prev_vtx_id = first_vtx_id;
        }
        mpi_errno_ret =
            MPIR_TSP_sched_generic(reserve_buf_type_id, data, sched, n_incoming, &prev_vtx_id,
                                   &first_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_generic(propagate_release_flag_type_id, data, sched, 1, &first_vtx_id,
                                   &second_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_cb(&MPIDI_POSIX_NB_RG_all_datacopy_cb, data, sched, 1, &second_vtx_id,
                              &third_vtx_id);

        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_generic(gather_type_id, data, sched, 1, &third_vtx_id, &fourth_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_cb(&MPIDI_POSIX_NB_RG_reduce_data_cb, data, sched, 1, &fourth_vtx_id,
                              &fifth_vtx_id);

        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_generic(start_sendrecv_type_id, data, sched, 1, &fifth_vtx_id,
                                   &sixth_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_generic(finish_sendrecv_type_id, data, sched, 1, &sixth_vtx_id,
                                   &seventh_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        offset += chunk_count;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
