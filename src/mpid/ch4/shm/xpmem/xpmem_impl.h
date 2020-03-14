/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef XPMEM_IMPL_H_INCLUDED
#define XPMEM_IMPL_H_INCLUDED

#include "mpidimpl.h"
#include "xpmem_pre.h"
#include "xpmem_types.h"

/* routines for sending control packets */
/* TODO: ideally we want to group an am send with its corresponding callback
 *       then each pair can define its own header struct rather than using a
 *       union across different packet types.
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_send_rts(int rank, MPIR_Comm * comm, int context_offset,
                                                  void *buf, MPI_Aint data_sz,
                                                  int tag, MPIR_Request * sreq)
{
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_ctrl_xpmem_send_lmt_rts_t *slmt_req_hdr = &ctrl_hdr.xpmem_slmt_rts;

    /* XPMEM internal info */
    slmt_req_hdr->src_offset = (uint64_t) buf;
    slmt_req_hdr->data_sz = data_sz;
    slmt_req_hdr->sreq_ptr = (uint64_t) sreq;
    slmt_req_hdr->src_lrank = MPIDI_XPMEM_global.local_rank;

    /* message matching info */
    slmt_req_hdr->src_rank = comm->rank;
    slmt_req_hdr->tag = tag;
    slmt_req_hdr->context_id = comm->context_id + context_offset;

    return MPIDI_SHM_am_send_hdr(rank, comm, MPIDI_SHM_XPMEM_SEND_LMT_RTS, &ctrl_hdr,
                                 sizeof(ctrl_hdr));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_send_cts(int rank, MPIR_Comm * comm,
                                                  void *buf, MPI_Aint data_sz,
                                                  uint64_t sreq_ptr, MPIR_Request * rreq,
                                                  int coop_counter_direct_flag,
                                                  uint64_t coop_counter_offset)
{
    MPIDI_SHM_ctrl_hdr_t ctrl_hdr;
    MPIDI_SHM_ctrl_xpmem_send_lmt_cts_t *slmt_cts_hdr = &ctrl_hdr.xpmem_slmt_cts;

    /* XPMEM internal info */
    slmt_cts_hdr->dest_offset = (uint64_t) buf;
    slmt_cts_hdr->data_sz = data_sz;
    slmt_cts_hdr->dest_lrank = MPIDI_XPMEM_global.local_rank;

    /* Receiver replies CTS packet */
    slmt_cts_hdr->sreq_ptr = sreq_ptr;
    slmt_cts_hdr->rreq_ptr = (uint64_t) rreq;
    slmt_req_hdr->coop_counter_direct_flag = coop_counter_direct_flag;
    slmt_req_hdr->coop_counter_offset = coop_counter_offset;

    return MPIDI_SHM_am_send_hdr(rank, comm, MPIDI_SHM_XPMEM_SEND_LMT_CTS, &ctrl_hdr,
                                 sizeof(ctrl_hdr));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_send_recv_fin(int rank, MPIR_Comm * comm,
                                                       intptr_t sreq_ptr)
{
    MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;
    ack_ctrl_hdr.xpmem_slmt_recv_fin.req_ptr = sreq_ptr;
    return MPIDI_SHM_am_send_hdr(rank, comm, MPIDI_SHM_XPMEM_SEND_LMT_RECV_FIN, &ack_ctrl_hdr,
                                 sizeof(ack_ctrl_hdr));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_send_send_fin(int rank, MPIR_Comm * comm,
                                                       intptr_t rreq_ptr)
{
    MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;
    ack_ctrl_hdr.xpmem_slmt_send_fin.req_ptr = rreq_ptr;
    return MPIDI_SHM_am_send_hdr(rank, comm, MPIDI_SHM_XPMEM_SEND_LMT_SEND_FIN, &ack_ctrl_hdr,
                                 sizeof(ack_ctrl_hdr));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_send_cnt_free(int rank, MPIR_Comm * comm,
                                                       int flag, uint64_t coop_counter_offset)
{
    MPIDI_SHM_ctrl_hdr_t ack_ctrl_hdr;
    ack_ctrl_hdr.xpmem_slmt_cnt_free.coop_counter_direct_flag = flag;
    ack_ctrl_hdr.xpmem_slmt_cnt_free.coop_counter_offset = coop_counter_offset;
    return MPIDI_SHM_am_send_hdr(rank, comm, MPIDI_SHM_XPMEM_SEND_LMT_CNT_FREE, &ack_ctrl_hdr,
                                 sizeof(ack_ctrl_hdr));
}

/* lmt cooperative copy */

/*
 * If one process finsih first, it will return LOCAL_FIN, and the other process will return BOTH_FIN.
 * Generally the former will wait for the latter to send ack. There are two special cases that we can
 * optimize and by-pass the ack, in which both processes will return SENDER_FIN or RECVER_FIN.
 */
enum {
    MPIDI_XPMEM_LOCAL_FIN,      /* this process finish first, need wait for the other process */
    MPIDI_XPMEM_BOTH_FIN,       /* this process finish second, need notify the other process */
    MPIDI_XPMEM_SENDER_FIN,     /* sender side copies all and finish first */
    MPIDI_XPMEM_RECVER_FIN      /* recver side copies all and finish first */
};

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_do_lmt_coop_copy(const void *src_buf,
                                                          size_t data_sz, void *dest_buf,
                                                          MPL_atomic_int64_t * offset_ptr,
                                                          int is_sender, int *fin_type)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_XPMEM_DO_LMT_COOP_COPY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_XPMEM_DO_LMT_COOP_COPY);

    MPI_Aint cur_offset;
    MPI_Aint copy_total = 0;
    while (true) {
        cur_offset = MPL_atomic_fetch_add_int64(offset_ptr,
                                                MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE);
        if (cur_offset >= data_sz) {
            break;
        }

        MPI_Aint copy_sz;
        if (cur_offset + MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE >= data_sz) {
            copy_sz = data_sz - cur_offset;
        } else {
            copy_sz = MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE;
        }

        MPIR_Memcpy((char *) dest_buf + cur_offset, (char *) src_buf + cur_offset, copy_sz);

        copy_total += copy_sz;
    }

    if (is_sender) {
        /* sender */
        if (cur_offset - data_sz < MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE) {
            /* finish first */
            if (copy_total == data_sz) {
                *fin_type = MPIDI_XPMEM_SENDER_FIN;
            } else {
                *fin_type = MPIDI_XPMEM_LOCAL_FIN;
            }
        } else {
            /* finish second */
            if (copy_total == 0) {
                *fin_type = MPIDI_XPMEM_RECVER_FIN;
            } else {
                *fin_type = MPIDI_XPMEM_BOTH_FIN;
            }
        }
    } else {
        /* receiver */
        if (cur_offset - data_sz < MPIR_CVAR_CH4_XPMEM_COOP_COPY_CHUNK_SIZE) {
            /* finish first */
            if (copy_total == data_sz) {
                *fin_type = MPIDI_XPMEM_RECVER_FIN;
            } else {
                *fin_type = MPIDI_XPMEM_LOCAL_FIN;
            }
        } else {
            /* finish second */
            if (copy_total == 0) {
                *fin_type = MPIDI_XPMEM_SENDER_FIN;
            } else {
                *fin_type = MPIDI_XPMEM_BOTH_FIN;
            }
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_XPMEM_DO_LMT_COOP_COPY);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_do_recv(void *src_buf, MPI_Aint data_sz, void *recv_buf,
                                                 MPL_atomic_int64_t * offset_ptr,
                                                 intptr_t sreq_ptr, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;

    int fin_type;
    mpi_errno = MPIDI_XPMEM_do_lmt_coop_copy(src_buf, data_sz, recv_buf, offset_ptr,
                                             FALSE, &fin_type);
    MPIR_ERR_CHECK(mpi_errno);

    if (fin_type == MPIDI_XPMEM_BOTH_FIN) {
        /* tell sender we finished too */
        int rank = MPIDIG_REQUEST(rreq, rank);
        MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(rreq, context_id));
        mpi_errno = MPIDI_XPMEM_send_recv_fin(rank, comm, sreq_ptr);
    }

    if (fin_type == MPIDI_XPMEM_BOTH_FIN || fin_type == MPIDI_XPMEM_SENDER_FIN) {
        /* sender is done, free the counter_ptr */
        MPIR_Handle_obj_free(&MPIDI_XPMEM_cnt_mem, MPIDI_XPMEM_REQUEST(rreq, counter_ptr));
    } else if (fin_type == MPIDI_XPMEM_RECVER_FIN) {
        /* need wait for sender to tell me when to free the counter_ptr */
        int c;
        MPIR_cc_incr(&MPIDI_XPMEM_global.num_pending_cnt, &c);
    }

    if (fin_type != MPIDI_XPMEM_LOCAL_FIN) {
        /* request is complete */
        MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(rreq, datatype));
        MPID_Request_complete(rreq);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_XPMEM_do_send(void *src_buf, MPI_Aint data_sz, void *recv_buf,
                                                 MPL_atomic_int64_t * offset_ptr,
                                                 MPIR_Request * sreq, intptr_t rreq_ptr,
                                                 int coop_counter_direct_flag,
                                                 uint64_t coop_counter_offset)
{
    int mpi_errno = MPI_SUCCESS;

    int fin_type;
    mpi_errno = MPIDI_XPMEM_do_lmt_coop_copy(src_buf, data_sz, recv_buf, offset_ptr,
                                             TRUE, &fin_type);
    MPIR_ERR_CHECK(mpi_errno);

    if (fin_type == MPIDI_XPMEM_BOTH_FIN) {
        /* tell recver we finished too */
        int rank = MPIDIG_REQUEST(sreq, rank);
        MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(sreq, context_id));
        mpi_errno = MPIDI_XPMEM_send_send_fin(rank, comm, rreq_ptr);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (fin_type == MPIDI_XPMEM_RECVER_FIN) {
        /* tell recver we finished so it can free the counter_ptr */
        int rank = MPIDIG_REQUEST(sreq, rank);
        MPIR_Comm *comm = MPIDIG_context_id_to_comm(MPIDIG_REQUEST(sreq, context_id));
        mpi_errno = MPIDI_XPMEM_send_cnt_free(rank, comm,
                                              coop_counter_direct_flag, coop_counter_offset);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (fin_type != MPIDI_XPMEM_LOCAL_FIN) {
        /* request is complete */
        MPIR_Datatype_release_if_not_builtin(MPIDIG_REQUEST(sreq, datatype));
        MPID_Request_complete(sreq);
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* XPMEM_IMPL_H_INCLUDED */
