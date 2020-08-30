/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_AM_H_INCLUDED
#define UCX_AM_H_INCLUDED

#include "ucx_impl.h"

#define MPIDI_UCX_AM_PACK_LIMIT 1024
#define MPIDI_UCX_AM(req, field) (sreq)->dev.ch4.am.netmod_am.ucx.field

/* NOTE: only accessed inside critical section */
static int next_payload_seq;
#define MPIDI_UCX_am_get_payload_seq() ((next_payload_seq++) & 0xff)

/* send side completion */
MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_send_cmpl(MPIR_Request * sreq)
{
    MPIDI_UCX_AM(sreq, cmpl_cntr)--;
    if (MPIDI_UCX_AM(sreq, cmpl_cntr) == 0) {
        MPL_gpu_free_host(MPIDI_UCX_AM(sreq, pack_buffer));
        MPIDI_UCX_AM(sreq, pack_buffer) = NULL;
        MPIDIG_global.origin_cbs[MPIDI_UCX_AM(sreq, handler_id)] (sreq);
    }
}

/* callback from am_isend, am_isendv, and am_isend_reply */
MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_isend_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);

    MPIDI_UCX_am_send_cmpl(ucp_request->req);
    MPIDI_UCX_ucp_request_free(request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_ISEND_CALLBACK);
}

/* callback from am_send_hdr and am_send_hdr_reply */
MPL_STATIC_INLINE_PREFIX void MPIDI_UCX_am_send_callback(void *request, ucs_status_t status)
{
    MPIDI_UCX_ucp_request_t *ucp_request = (MPIDI_UCX_ucp_request_t *) request;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);

    MPL_free(ucp_request->buf);
    MPIDI_UCX_ucp_request_free(ucp_request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_UCX_AM_SEND_CALLBACK);
}

/* internal functions */

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_am_do_send(ucp_ep_h ep, void *sreq,
                                                  void *send_buf, MPI_Aint send_sz,
                                                  uint64_t ucx_tag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf, send_sz,
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_isend_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        MPIDI_UCX_am_send_cmpl(sreq);
    } else {
        ucp_request->req = sreq;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* allocate send buffer and pack data.
 *
 * Input:  am_hdr_sz, data, count, datatype
 * Output: pack_type, p_buf, p_data, data_sz
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_am_send_prepare(MPI_Aint am_hdr_sz,
                                                       const void *data,
                                                       MPI_Aint count,
                                                       MPI_Datatype datatype,
                                                       int *pack_type,
                                                       void **p_buf,
                                                       void **p_data, MPI_Aint * data_sz)
{
    int mpi_errno = MPI_SUCCESS;

    int dt_contig;
    MPIR_Datatype *dt_ptr;
    MPI_Aint dt_true_lb;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, *data_sz, dt_ptr, dt_true_lb);

    MPI_Aint header_sz = sizeof(MPIDI_UCX_am_header_t) + am_hdr_sz;
    MPI_Aint total_sz = sizeof(MPIDI_UCX_am_header_t) + am_hdr_sz + *data_sz;

    /* TODO: support noncontig data pipeline */
    bool do_pack = (!dt_contig || total_sz < MPIDI_UCX_AM_PACK_LIMIT);

    if (do_pack) {
        MPL_gpu_malloc_host(p_buf, sizeof(MPIDI_UCX_am_header_t) + am_hdr_sz + *data_sz);
        *p_data = (char *) (*p_buf) + header_sz;

        MPI_Aint actual_pack_bytes;
        mpi_errno = MPIR_Typerep_pack(data, count, datatype, 0,
                                      *p_data, *data_sz, &actual_pack_bytes);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_Assert(actual_pack_bytes == *data_sz);

        if (total_sz < MPIDI_UCX_AM_BUF_SIZE) {
            *pack_type = MPIDI_UCX_AM_PACK_ONE;
        } else if (header_sz < MPIDI_UCX_AM_BUF_SIZE) {
            *pack_type = MPIDI_UCX_AM_PACK_TWO_A;
        } else {
            *pack_type = MPIDI_UCX_AM_PACK_TWO_B;
        }
    } else {
        MPL_gpu_malloc_host(p_buf, sizeof(MPIDI_UCX_am_header_t) + am_hdr_sz);
        *p_data = (char *) data + dt_true_lb;
        if (header_sz < MPIDI_UCX_AM_BUF_SIZE) {
            *pack_type = MPIDI_UCX_AM_PACK_TWO_A;
        } else {
            /* not supported */
            MPIR_Assert(0);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_UCX_am_send(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               int pack_type,
                                               void *send_buf,
                                               void *p_data,
                                               MPI_Aint am_hdr_sz,
                                               MPI_Aint data_sz, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    ucp_ep_h ep = MPIDI_UCX_COMM_TO_EP(comm, rank, 0, 0);

    MPIDI_UCX_am_header_t *msg_hdr = send_buf;
    /* initialize our portion of the hdr */
    msg_hdr->handler_id = handler_id;
    msg_hdr->am_hdr_sz = am_hdr_sz;
    msg_hdr->data_sz = data_sz;
    msg_hdr->pack_type = pack_type;

    MPIDI_UCX_AM(sreq, pack_buffer) = send_buf;
    MPIDI_UCX_AM(sreq, handler_id) = handler_id;

    if (pack_type == MPIDI_UCX_AM_PACK_ONE) {
        MPIDI_UCX_AM(sreq, cmpl_cntr) = 1;
        /* msg_hdr->payload_seq not needed */
        uint64_t hdr_tag = MPIDI_UCX_am_init_hdr_tag(MPIR_Process.rank);

        MPI_Aint total_sz = sizeof(MPIDI_UCX_am_header_t) + am_hdr_sz + data_sz;
        mpi_errno = MPIDI_UCX_am_do_send(ep, sreq, send_buf, total_sz, hdr_tag);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (pack_type == MPIDI_UCX_AM_PACK_TWO_A) {
        MPIDI_UCX_AM(sreq, cmpl_cntr) = 2;
        msg_hdr->payload_seq = MPIDI_UCX_am_get_payload_seq();
        uint64_t hdr_tag = MPIDI_UCX_am_init_hdr_tag(MPIR_Process.rank);
        uint64_t data_tag = MPIDI_UCX_am_init_data_tag(MPIR_Process.rank, msg_hdr->payload_seq);

        MPI_Aint header_sz = sizeof(MPIDI_UCX_am_header_t) + am_hdr_sz;
        /* send header */
        mpi_errno = MPIDI_UCX_am_do_send(ep, sreq, send_buf, header_sz, hdr_tag);
        MPIR_ERR_CHECK(mpi_errno);

        /* send payload */
        mpi_errno = MPIDI_UCX_am_do_send(ep, sreq, p_data, data_sz, data_tag);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (pack_type == MPIDI_UCX_AM_PACK_TWO_B) {
        /* data has to be packed, p_data not used */
        MPIDI_UCX_AM(sreq, cmpl_cntr) = 2;
        msg_hdr->payload_seq = MPIDI_UCX_am_get_payload_seq();
        uint64_t hdr_tag = MPIDI_UCX_am_init_hdr_tag(MPIR_Process.rank);
        uint64_t data_tag = MPIDI_UCX_am_init_data_tag(MPIR_Process.rank, msg_hdr->payload_seq);

        /* send header */
        mpi_errno = MPIDI_UCX_am_do_send(ep, sreq, msg_hdr, sizeof(MPIDI_UCX_am_header_t), hdr_tag);
        MPIR_ERR_CHECK(mpi_errno);

        /* send payload */
        mpi_errno = MPIDI_UCX_am_do_send(ep, sreq, msg_hdr + 1, am_hdr_sz + data_sz, data_tag);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* NETMOD API functions */

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank,
                                               MPIR_Comm * comm,
                                               int handler_id,
                                               const void *am_hdr,
                                               size_t am_hdr_sz,
                                               const void *data,
                                               MPI_Count count, MPI_Datatype datatype,
                                               MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    int pack_type = 0;
    void *send_buf, *p_data;
    MPI_Aint data_sz;
    mpi_errno = MPIDI_UCX_am_send_prepare(am_hdr_sz, data, count, datatype,
                                          &pack_type, &send_buf, &p_data, &data_sz);
    MPIR_ERR_CHECK(mpi_errno);

    /* copying the header to pack buffer */
    MPIR_Memcpy((char *) send_buf + sizeof(MPIDI_UCX_am_header_t), am_hdr, am_hdr_sz);

    mpi_errno = MPIDI_UCX_am_send(rank, comm, handler_id, pack_type, send_buf, p_data,
                                  am_hdr_sz, data_sz, sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank,
                                                MPIR_Comm * comm,
                                                int handler_id,
                                                struct iovec *am_hdr,
                                                size_t iov_len,
                                                const void *data,
                                                MPI_Count count, MPI_Datatype datatype,
                                                MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISENDV);

    /* calculate the header size */
    size_t am_hdr_sz = 0;
    for (int i = 0; i < iov_len; i++) {
        am_hdr_sz += am_hdr[i].iov_len;
    }

    int pack_type = 0;
    void *send_buf, *p_data;
    MPI_Aint data_sz;
    mpi_errno = MPIDI_UCX_am_send_prepare(am_hdr_sz, data, count, datatype,
                                          &pack_type, &send_buf, &p_data, &data_sz);
    MPIR_ERR_CHECK(mpi_errno);

    /* copying the header to pack buffer */
    char *p = send_buf;
    p += sizeof(MPIDI_UCX_am_header_t);
    for (int i = 0; i < iov_len; i++) {
        MPIR_Memcpy(p, am_hdr[i].iov_base, am_hdr[i].iov_len);
        p += am_hdr[i].iov_len;
    }

    mpi_errno = MPIDI_UCX_am_send(rank, comm, handler_id, pack_type, send_buf, p_data,
                                  am_hdr_sz, data_sz, sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISENDV);
    return mpi_errno;

  fn_fail:
    goto fn_exit;

}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id,
                                                     int src_rank,
                                                     int handler_id,
                                                     const void *am_hdr,
                                                     size_t am_hdr_sz,
                                                     const void *data, MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq)
{
    MPIR_Comm *use_comm = MPIDIG_context_id_to_comm(context_id);
    return MPIDI_NM_am_isend(src_rank, use_comm, handler_id, am_hdr,
                             am_hdr_sz, data, count, datatype, sreq);
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);

    ret = (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    return ret;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_eager_limit(void)
{
    return (MPIDI_UCX_MAX_AM_EAGER_SZ - sizeof(MPIDI_UCX_am_header_t));
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_eager_buf_limit(void)
{
    return MPIDI_UCX_MAX_AM_EAGER_SZ;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank,
                                                  MPIR_Comm * comm,
                                                  int handler_id, const void *am_hdr,
                                                  size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    MPIDI_UCX_am_header_t ucx_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR);

    ep = MPIDI_UCX_COMM_TO_EP(comm, rank, 0, 0);
    ucx_tag = MPIDI_UCX_am_init_hdr_tag(MPIR_Process.rank);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = 0;
    ucx_hdr.pack_type = MPIDI_UCX_AM_PACK_ONE;

    /* just pack and send for now */
    send_buf = MPL_malloc(am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);

    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_send_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* inject is done */
        MPL_free(send_buf);
    } else {
        ucp_request->buf = send_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank,
                                                        int handler_id, const void *am_hdr,
                                                        size_t am_hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_UCX_ucp_request_t *ucp_request;
    ucp_ep_h ep;
    uint64_t ucx_tag;
    char *send_buf;
    MPIDI_UCX_am_header_t ucx_hdr;
    MPIR_Comm *use_comm;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);

    use_comm = MPIDIG_context_id_to_comm(context_id);
    ep = MPIDI_UCX_COMM_TO_EP(use_comm, src_rank, 0, 0);
    ucx_tag = MPIDI_UCX_am_init_hdr_tag(MPIR_Process.rank);

    /* initialize our portion of the hdr */
    ucx_hdr.handler_id = handler_id;
    ucx_hdr.data_sz = 0;
    ucx_hdr.pack_type = MPIDI_UCX_AM_PACK_ONE;

    /* just pack and send for now */
    send_buf = MPL_malloc(am_hdr_sz + sizeof(ucx_hdr), MPL_MEM_BUFFER);
    MPIR_Memcpy(send_buf, &ucx_hdr, sizeof(ucx_hdr));
    MPIR_Memcpy(send_buf + sizeof(ucx_hdr), am_hdr, am_hdr_sz);
    ucp_request = (MPIDI_UCX_ucp_request_t *) ucp_tag_send_nb(ep, send_buf,
                                                              am_hdr_sz + sizeof(ucx_hdr),
                                                              ucp_dt_make_contig(1), ucx_tag,
                                                              &MPIDI_UCX_am_send_callback);
    MPIDI_UCX_CHK_REQUEST(ucp_request);

    if (ucp_request == NULL) {
        /* inject is done */
        MPL_free(send_buf);
    } else {
        ucp_request->buf = send_buf;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* UCX_AM_H_INCLUDED */
