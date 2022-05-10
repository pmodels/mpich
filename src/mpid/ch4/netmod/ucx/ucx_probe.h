/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef UCX_PROBE_H_INCLUDED
#define UCX_PROBE_H_INCLUDED

#include "ucx_impl.h"

#define MPIDI_UCX_PROBE_VNIS(vni_dst_) \
    do { \
        int vni_src_tmp; \
        MPIDI_EXPLICIT_VCIS(comm, attr, source, comm->rank, vni_src_tmp, vni_dst_); \
        if (vni_src_tmp == 0 && vni_dst_ == 0) { \
            vni_dst_ = MPIDI_get_vci(DST_VCI_FROM_RECVER, comm, source, comm->rank, tag); \
        } \
    } while (0)

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source,
                                                  int tag,
                                                  MPIR_Comm * comm,
                                                  int attr,
                                                  MPIDI_av_entry_t * addr,
                                                  int *flag, MPIR_Request ** message,
                                                  MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t ucp_tag, tag_mask;
    MPI_Aint count;
    ucp_tag_recv_info_t info;
    ucp_tag_message_h message_h;
    MPIR_Request *req = NULL;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    int vni_dst;
    MPIDI_UCX_PROBE_VNIS(vni_dst);

    MPIDI_UCX_THREAD_CS_ENTER_VCI(vni_dst);

    tag_mask = MPIDI_UCX_tag_mask(tag, source);
    ucp_tag = MPIDI_UCX_recv_tag(tag, source, comm->recvcontext_id + context_offset);

    message_h = ucp_tag_probe_nb(MPIDI_UCX_global.ctx[vni_dst].worker, ucp_tag, tag_mask, 1, &info);

    if (message_h) {
        *flag = 1;
        req = (MPIR_Request *) MPIR_Request_create_from_pool(MPIR_REQUEST_KIND__MPROBE, vni_dst, 2);
        MPIR_ERR_CHKANDSTMT((req) == NULL, mpi_errno, MPIX_ERR_NOREQ, goto fn_fail, "**nomemreq");
        req->comm = comm;
        MPIR_Comm_add_ref(comm);
        MPIDI_UCX_REQ(req).message_handler = message_h;

        if (status != MPI_STATUS_IGNORE) {
            status->MPI_SOURCE = MPIDI_UCX_get_source(info.sender_tag);
            status->MPI_TAG = MPIDI_UCX_get_tag(info.sender_tag);
            count = info.length;
            MPIR_STATUS_SET_COUNT(*status, count);
        }
    } else {
        *flag = 0;
    }
    *message = req;

  fn_exit:
    MPIDI_UCX_THREAD_CS_EXIT_VCI(vni_dst);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source,
                                                 int tag,
                                                 MPIR_Comm * comm,
                                                 int attr,
                                                 MPIDI_av_entry_t * addr, int *flag,
                                                 MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    uint64_t ucp_tag, tag_mask;
    MPI_Aint count;
    ucp_tag_recv_info_t info;
    ucp_tag_message_h message_h;

    int context_offset = MPIR_PT2PT_ATTR_CONTEXT_OFFSET(attr);

    int vni_dst;
    MPIDI_UCX_PROBE_VNIS(vni_dst);

    MPIDI_UCX_THREAD_CS_ENTER_VCI(vni_dst);

    tag_mask = MPIDI_UCX_tag_mask(tag, source);
    ucp_tag = MPIDI_UCX_recv_tag(tag, source, comm->recvcontext_id + context_offset);

    message_h = ucp_tag_probe_nb(MPIDI_UCX_global.ctx[vni_dst].worker, ucp_tag, tag_mask, 0, &info);

    if (message_h) {
        *flag = 1;

        if (status != MPI_STATUS_IGNORE) {
            status->MPI_ERROR = MPI_SUCCESS;
            status->MPI_SOURCE = MPIDI_UCX_get_source(info.sender_tag);
            status->MPI_TAG = MPIDI_UCX_get_tag(info.sender_tag);
            count = info.length;
            MPIR_STATUS_SET_COUNT(*status, count);
        }
    } else {
        *flag = 0;
    }

    MPIDI_UCX_THREAD_CS_EXIT_VCI(vni_dst);

    return mpi_errno;
}

#endif /* UCX_PROBE_H_INCLUDED */
