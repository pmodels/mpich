/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef NB_BCAST_RELEASE_GATHER_H_INCLUDED
#define NB_BCAST_RELEASE_GATHER_H_INCLUDED

#include "gentran_types.h"

/* Flags Layout - For segment 0, gather flag for each rank, followed by release flag for each rank.
 * Repeat same for segment 1 and so on.
 */
#define MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR(rank, segment, num_ranks)         \
    (((MPL_atomic_uint64_t *)nb_release_gather_info_ptr->ibcast_flags_addr) +                   \
     (((segment * num_ranks * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK) +                 \
       (rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK +                                 \
        MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_OFFSET))/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))

#define MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_RELEASE_FLAG_ADDR(rank, segment, num_ranks)        \
    (((MPL_atomic_uint64_t *)nb_release_gather_info_ptr->ibcast_flags_addr) +                   \
     (((segment * num_ranks * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK) +                 \
       (rank * MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK +                                 \
        MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_OFFSET))/(MPIDI_POSIX_RELEASE_GATHER_FLAG_SIZE)))

#define MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_DATA_ADDR(buf)\
    (char *) nb_release_gather_info_ptr->bcast_buf_addr + \
    (buf * MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE)

int MPIDI_POSIX_nb_release_gather_comm_init(MPIR_Comm * comm_ptr,
                                            const MPIDI_POSIX_release_gather_opcode_t operation);
int MPIDI_POSIX_nb_release_gather_comm_free(MPIR_Comm * comm_ptr);

/* The following functions with suffix issue, completion or cb are used as function pointers in the
 * generic vertex types or callback type vertices created later */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_empty_issue(void *v, int *done)
{
    *done = 0;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_root_datacopy_issue(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ibcast_info_t *) vtxp->u.generic.data;
    int root = per_call_data->root;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;

    int rank = MPIR_Comm_rank(comm_ptr);

    if (root != 0) {
        if (rank == root || rank == 0) {
            /* Root has to be send data to rank 0 when the shm_buffer is available */
            *done = 0;
            return MPI_SUCCESS;
        }
    } else if (rank == 0) {
        /* Rank 0 has to copy data to shm_buffer when it is available */
        *done = 0;
        return MPI_SUCCESS;
    }
    /* Rest of the ranks can proceed to next vertex */
    *done = 1;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_root_datacopy_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ibcast_info_t *) vtxp->u.generic.data;
    int root = per_call_data->root;
    int num_cells = MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    int segment = per_call_data->seq_no % num_cells;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int last_seq_no = nb_release_gather_info_ptr->ibcast_last_seq_no_completed[segment];

    MPL_atomic_uint64_t *my_release_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_RELEASE_FLAG_ADDR(rank, segment, num_ranks);
    MPL_atomic_uint64_t *my_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR(rank, segment, num_ranks);
    MPL_atomic_uint64_t *rank_0_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR(0, segment, num_ranks);

    if (MPL_atomic_acquire_load_uint64(rank_0_gather_flag_addr) == -1) {
        /* Buffer can be overwritten */
        if (root != 0) {
            /* Root sends data to rank 0 */
            if (rank == root) {
                MPIC_Isend(per_call_data->local_buf, per_call_data->count, per_call_data->datatype,
                           0, per_call_data->tag, comm_ptr, &(per_call_data->sreq), &errflag);
                *done = 1;
            } else if (rank == 0) {
                MPIC_Irecv(MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_DATA_ADDR(segment),
                           per_call_data->count, per_call_data->datatype, per_call_data->root,
                           per_call_data->tag, comm_ptr, &(per_call_data->rreq));
                *done = 1;
            }
        } else {
            /* Rank 0 makes sure that the seq_nos that executes on a cell are in order. It will
             * hold the buffer for seq_no 8 only when seq_no 4 is completed (Assuming num_cells
             * is 4). */
            if (rank == 0 &&
                (last_seq_no == per_call_data->seq_no - num_cells || last_seq_no < num_cells)) {
                /* Mark the buffer as occupied */
                MPL_atomic_release_store_uint64(my_gather_flag_addr, -2);
                MPIR_Localcopy(per_call_data->local_buf, per_call_data->count,
                               per_call_data->datatype,
                               MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_DATA_ADDR(segment),
                               per_call_data->count, per_call_data->datatype);
                MPL_atomic_release_store_uint64(my_release_flag_addr, per_call_data->seq_no);
                *done = 1;
            } else {
                *done = 0;
            }
        }
    } else {
        /* Buffer was not ready */
        *done = 0;
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_finish_send_recv_issue(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ibcast_info_t *) vtxp->u.generic.data;
    int root = per_call_data->root;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);

    if ((root != 0) && (rank == root || rank == 0)) {
        /* Root and rank 0 will have to wait on completion of send, recv respectively in case of
         * non-zero root*/
        *done = 0;
        return MPI_SUCCESS;
    }
    *done = 1;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_finish_send_recv_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data =
        (MPIDI_POSIX_per_call_ibcast_info_t *) vtxp->u.generic.data;
    int root = per_call_data->root;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    int segment = per_call_data->seq_no % MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    MPL_atomic_uint64_t *my_release_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_RELEASE_FLAG_ADDR(rank, segment, num_ranks);
    MPL_atomic_uint64_t *my_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR(rank, segment, num_ranks);

    if (root != 0) {
        if (rank == root) {
            if (MPIR_Request_is_complete(per_call_data->sreq)) {
                MPIR_Request_free(per_call_data->sreq);
                per_call_data->sreq = NULL;
                *done = 1;
                return MPI_SUCCESS;
            } else {
                /* Send is not complete */
                *done = 0;
                return MPI_SUCCESS;
            }
        } else if (rank == 0) {
            if (MPIR_Request_is_complete(per_call_data->rreq)) {
                /* Mark the buffer as occupied */
                MPL_atomic_release_store_uint64(my_gather_flag_addr, -2);
                MPIR_Request_free(per_call_data->rreq);
                per_call_data->rreq = NULL;
                /* Rank 0 updates its flag when it arrives and data is ready in shm buffer (if bcast) */
                /* zm_memord_release makes sure that the write of data does not get reordered after this
                 * store */
                MPL_atomic_release_store_uint64(my_release_flag_addr, per_call_data->seq_no);
                *done = 1;
                return MPI_SUCCESS;
            } else {
                /* Recv is not complete */
                *done = 0;
                return MPI_SUCCESS;
            }
        }
    }
    *done = 1;
    return MPI_SUCCESS;

}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_update_release_flag_completion(void *v, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data = vtxp->u.generic.data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    MPL_atomic_uint64_t *parent_release_flag_addr, *my_release_flag_addr;

    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int segment = per_call_data->seq_no % MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;

    my_release_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_RELEASE_FLAG_ADDR(rank, segment, num_ranks);

    if (rank != 0) {
        parent_release_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_RELEASE_FLAG_ADDR
            (nb_release_gather_info_ptr->bcast_tree.parent, segment, num_ranks);
    }

    if (rank != 0) {
        if (MPL_atomic_acquire_load_uint64(parent_release_flag_addr) == per_call_data->seq_no) {
            /* Update my release_flag if the parent has arrived */
            MPL_atomic_release_store_uint64(my_release_flag_addr, per_call_data->seq_no);
        } else {
            *done = 0;
            return MPI_SUCCESS;
        }
    }

    *done = 1;
    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_non_root_datacopy_cb(MPIR_Comm * comm, int tag,
                                                                    void *data)
{
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data = data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    int segment = per_call_data->seq_no % MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    int num_children = nb_release_gather_info_ptr->bcast_tree.num_children;
    MPL_atomic_uint64_t *my_gather_flag_addr;

    my_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR(rank, segment, num_ranks);

    if (rank != per_call_data->root) {
        MPIR_Localcopy(MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_DATA_ADDR(segment),
                       per_call_data->count, per_call_data->datatype, per_call_data->local_buf,
                       per_call_data->count, per_call_data->datatype);
    }
    if (num_children == 0) {
        /* Leaf ranks update their gather flags */
        MPL_atomic_release_store_uint64(my_gather_flag_addr, per_call_data->seq_no);
    }

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_gather_completion(void *v, int *done)
{
    MPII_Genutil_vtx_t *vtxp = (MPII_Genutil_vtx_t *) v;
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data = vtxp->u.generic.data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int num_ranks = MPIR_Comm_size(comm_ptr);
    UT_array *children;
    int i;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    children = nb_release_gather_info_ptr->bcast_tree.children;
    int num_children = nb_release_gather_info_ptr->bcast_tree.num_children;
    int segment = per_call_data->seq_no % MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
    MPL_atomic_uint64_t *child_gather_flag_addr;

    /* Gather phase begins */
    for (i = 0; i < num_children; i++) {
        int child_rank = *(int *) utarray_eltptr(children, i);
        child_gather_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR(child_rank, segment, num_ranks);
        if (MPL_atomic_acquire_load_uint64(child_gather_flag_addr) != per_call_data->seq_no) {
            *done = 0;
            return MPI_SUCCESS;
        }
    }

    *done = 1;
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_NB_RG_update_gather_flag_cb(MPIR_Comm * comm, int tag,
                                                                     void *data)
{
    MPIDI_POSIX_per_call_ibcast_info_t *per_call_data = data;
    MPIR_Comm *comm_ptr = per_call_data->comm_ptr;
    int rank = MPIR_Comm_rank(comm_ptr);
    int num_ranks = MPIR_Comm_size(comm_ptr);
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr;
    int segment = per_call_data->seq_no % MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
    nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
    MPL_atomic_uint64_t *my_gather_flag_addr;

    my_gather_flag_addr =
        MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR(rank, segment, num_ranks);
    /* Update my gather flag. Reaching here means all my children have arrived */
    MPL_atomic_release_store_uint64(my_gather_flag_addr, per_call_data->seq_no);

    if (rank == 0 && MPL_atomic_acquire_load_uint64(my_gather_flag_addr) == per_call_data->seq_no) {
        /* If I am rank 0 and all the ranks have arrived, set my_gather_flag to -1 */
        MPL_atomic_release_store_uint64(my_gather_flag_addr, -1);
        nb_release_gather_info_ptr->ibcast_last_seq_no_completed[segment] = per_call_data->seq_no;
    }
    return MPI_SUCCESS;
}

/* Add vertices to the schedule to perform a release followed by gather. Both the steps are broken
 * into a total of 6 vertices. It involves the following steps:
 *
 * 1. Rank 0 copies the data into a cell of shm_buffer and updates its release flag.
 * 2. Rest of the ranks are waiting on this flag.
 * 3. Then non-zero ranks copy data out of shm_buffer and update its gather_flag, depending on if
 * the children have arrived. This way rank 0 knows when the buffer can be overwritten for the next
 * release step */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_nb_release_gather_ibcast_impl(void *local_buf,
                                                                       MPI_Aint count,
                                                                       MPI_Datatype datatype,
                                                                       const int root,
                                                                       MPIR_Comm * comm_ptr,
                                                                       MPIR_TSP_sched_t sched)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    int tag, my_rank;
    int first_vtx_id = -1, second_vtx_id, third_vtx_id, fourth_vtx_id, fifth_vtx_id, *sixth_vtx_id;
    int prev_vtx_id, pack_vtx_id = -1;
    int root_datacopy_type_id, update_release_flag_type_id, gather_type_id,
        finish_send_recv_type_id;
    MPIDI_POSIX_per_call_ibcast_info_t *data;
    int i;
    MPI_Aint num_chunks, chunk_count_floor, chunk_count_ceil;
    int offset = 0, is_contig, vtx_id;
    MPI_Aint ori_count = count;
    MPI_Aint type_size, nbytes, true_lb, true_extent;
    void *ori_local_buf = local_buf;
    MPI_Datatype ori_datatype = datatype;

    MPIR_CHKLMEM_DECL(1);
    /* Register the vertices */
    root_datacopy_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_root_datacopy_issue,
                                MPIDI_POSIX_NB_RG_root_datacopy_completion, NULL);
    finish_send_recv_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_finish_send_recv_issue,
                                MPIDI_POSIX_NB_RG_finish_send_recv_completion, NULL);
    update_release_flag_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_empty_issue,
                                MPIDI_POSIX_NB_RG_update_release_flag_completion, NULL);
    gather_type_id =
        MPIR_TSP_sched_new_type(sched, MPIDI_POSIX_NB_RG_empty_issue,
                                MPIDI_POSIX_NB_RG_gather_completion, NULL);

    my_rank = MPIR_Comm_rank(comm_ptr);
    MPIR_Datatype_is_contig(datatype, &is_contig);

    if (is_contig) {
        MPIR_Datatype_get_size_macro(datatype, type_size);
    } else {
        MPIR_Pack_size_impl(1, datatype, comm_ptr, &type_size);
    }

    nbytes = type_size * count;

    if (is_contig) {
        /* contiguous. No need to pack */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        local_buf = (char *) ori_local_buf + true_lb;
    } else {
        local_buf = MPIR_TSP_sched_malloc(count, sched);
        if (my_rank == root) {
            /* Root packs the data before sending, for non contiguous datatypes */
            mpi_errno_ret =
                MPIR_TSP_sched_localcopy(ori_local_buf, ori_count, ori_datatype, local_buf, nbytes,
                                         MPI_BYTE, sched, 0, NULL, &pack_vtx_id);
            if (mpi_errno_ret)
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
        }
    }

    /* Calculate chunking information for pipelining */
    /* Chunking information is calculated in terms of bytes (not type_size), so we may copy only
     * half a datatype in one chunk, but that is fine */
    MPIR_Algo_calculate_pipeline_chunk_info(MPIDI_POSIX_RELEASE_GATHER_BCAST_CELLSIZE, 1,
                                            nbytes, &num_chunks, &chunk_count_floor,
                                            &chunk_count_ceil);
    MPIR_CHKLMEM_MALLOC(sixth_vtx_id, int *, num_chunks * sizeof(int), mpi_errno, "sixth_vtx_ids",
                        MPL_MEM_COLL);

    /* Do pipelined release-gather */
    /* A schedule gets created in the form of a forest, where each tree has 6 vertices (to perform
     * release_gather) and number of trees is same as number of chunks the message is divided
     * into. Then, there is a dependence from first vertex of previous chunk to first vertex of next
     * chunk. */

    for (i = 0; i < num_chunks; i++) {
        MPI_Aint chunk_count = (i == 0) ? chunk_count_floor : chunk_count_ceil;
        int n_incoming = 0;
        if (i == 0 && !is_contig) {
            /* Create dependence from the pack vertex to the first vertex of the first chunk */
            n_incoming = 1;
            prev_vtx_id = pack_vtx_id;
        }

        data = (MPIDI_POSIX_per_call_ibcast_info_t *)
            MPIR_TSP_sched_malloc(sizeof(MPIDI_POSIX_per_call_ibcast_info_t), sched);
        MPIR_ERR_CHKANDJUMP(!data, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        /* Every iteration gets a unique sequence number. This is needed to determine which cell in
         * shm_buffer should be used for which iteration */
        MPIDI_POSIX_COMM(comm_ptr, nb_bcast_seq_no)++;

        /* Fill in the per_call data structure */
        data->seq_no = MPIDI_POSIX_COMM(comm_ptr, nb_bcast_seq_no);
        data->local_buf = (char *) local_buf + offset;
        data->count = chunk_count;
        data->datatype = MPI_BYTE;
        data->root = root;
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
            MPIR_TSP_sched_generic(root_datacopy_type_id, data, sched, n_incoming, &prev_vtx_id,
                                   &first_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_generic(finish_send_recv_type_id, data, sched, 1, &first_vtx_id,
                                   &second_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_generic(update_release_flag_type_id, data, sched, 1, &second_vtx_id,
                                   &third_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno_ret =
            MPIR_TSP_sched_cb(&MPIDI_POSIX_NB_RG_non_root_datacopy_cb, data, sched, 1,
                              &third_vtx_id, &fourth_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
        mpi_errno_ret =
            MPIR_TSP_sched_generic(gather_type_id, data, sched, 1, &fourth_vtx_id, &fifth_vtx_id);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);

        mpi_errno =
            MPIR_TSP_sched_cb(&MPIDI_POSIX_NB_RG_update_gather_flag_cb, data, sched, 1,
                              &fifth_vtx_id, &sixth_vtx_id[i]);
        if (mpi_errno_ret)
            MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
        offset += chunk_count;
    }

    if (!is_contig) {
        if (my_rank != root) {
            /* Non-root unpack the data if expecting non-contiguous datatypes */
            /* Dependency is created from the last vertices of each chunk to the unpack vertex */
            MPIR_Assert(num_chunks <= INT_MAX);
            mpi_errno =
                MPIR_TSP_sched_localcopy(local_buf, nbytes, MPI_BYTE, ori_local_buf, ori_count,
                                         ori_datatype, sched, (int) num_chunks, sixth_vtx_id,
                                         &vtx_id);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
