/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
/* ch4 netmod functions */
#ifndef NETMOD_IMPL_H_INCLUDED
#define NETMOD_IMPL_H_INCLUDED


#ifndef NETMOD_DIRECT
#ifndef NETMOD_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_init_hook(int rank, int size, int appnum, int *tag_ub,
                                                    MPIR_Comm * comm_world, MPIR_Comm * comm_self,
                                                    int spawned, int *n_vnis_provided)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INIT_HOOK);

    ret = MPIDI_NM_func->mpi_init(rank, size, appnum, tag_ub, comm_world, comm_self, spawned,
                                  n_vnis_provided);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INIT_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_finalize_hook(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_FINALIZE_HOOK);

    ret = MPIDI_NM_func->mpi_finalize();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_FINALIZE_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_get_vni_attr(int vni)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_QUERY_VNI);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_QUERY_VNI);

    ret = MPIDI_NM_func->get_vni_attr(vni);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_QUERY_VNI);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(int vni, int blocking)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_PROGRESS);

    ret = MPIDI_NM_func->progress(vni, blocking);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_PROGRESS);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_connect(const char *port_name, MPIR_Info * info,
                                                       int root, int timeout, MPIR_Comm * comm,
                                                       MPIR_Comm ** newcomm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_CONNECT);

    ret = MPIDI_NM_func->mpi_comm_connect(port_name, info, root, timeout, comm, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_CONNECT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_DISCONNECT);

    ret = MPIDI_NM_func->mpi_comm_disconnect(comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_DISCONNECT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_OPEN_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_OPEN_PORT);

    ret = MPIDI_NM_func->mpi_open_port(info_ptr, port_name);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_OPEN_PORT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_close_port(const char *port_name)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CLOSE_PORT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CLOSE_PORT);

    ret = MPIDI_NM_func->mpi_close_port(port_name);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CLOSE_PORT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_accept(const char *port_name, MPIR_Info * info,
                                                      int root, MPIR_Comm * comm,
                                                      MPIR_Comm ** newcomm_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_ACCEPT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_ACCEPT);

    ret = MPIDI_NM_func->mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_ACCEPT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, size_t am_hdr_sz)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR);

    ret = MPIDI_NM_func->am_send_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, size_t am_hdr_sz,
                                               const void *data, MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND);

    ret = MPIDI_NM_func->am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                  sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                                struct iovec *am_hdrs, size_t iov_len,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype, MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISENDV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISENDV);

    ret = MPIDI_NM_func->am_isendv(rank, comm, handler_id, am_hdrs, iov_len, data, count, datatype,
                                   sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISENDV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank, int handler_id,
                                                        const void *am_hdr, size_t am_hdr_sz)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);

    ret = MPIDI_NM_func->am_send_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_SEND_HDR_REPLY);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                     int handler_id, const void *am_hdr,
                                                     size_t am_hdr_sz, const void *data,
                                                     MPI_Count count, MPI_Datatype datatype,
                                                     MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);

    ret = MPIDI_NM_func->am_isend_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz, data,
                                        count, datatype, sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_ISEND_REPLY);
    return ret;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);

    ret = MPIDI_NM_func->am_hdr_max_sz();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_HDR_MAX_SZ);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_recv(MPIR_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_RECV);

    ret = MPIDI_NM_func->am_recv(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_RECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                    int *lpid_ptr, MPL_bool is_remote)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_COMM_GET_LPID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_COMM_GET_LPID);

    ret = MPIDI_NM_func->comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_COMM_GET_LPID);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                                      char **local_upids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_GET_LOCAL_UPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_GET_LOCAL_UPIDS);

    ret = MPIDI_NM_func->get_local_upids(comm, local_upid_size, local_upids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_GET_LOCAL_UPIDS);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_upids_to_lupids(int size, size_t * remote_upid_size,
                                                      char *remote_upids, int **remote_lupids)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_UPIDS_TO_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_UPIDS_TO_LUPIDS);

    ret = MPIDI_NM_func->upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_UPIDS_TO_LUPIDS);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                  int size, const int lpids[])
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_CREATE_INTERCOMM_FROM_LPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_CREATE_INTERCOMM_FROM_LPIDS);

    ret = MPIDI_NM_func->create_intercomm_from_lpids(newcomm_ptr, size, lpids);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_CREATE_INTERCOMM_FROM_LPIDS);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);

    ret = MPIDI_NM_func->mpi_comm_create_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_CREATE_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);

    ret = MPIDI_NM_func->mpi_comm_free_hook(comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMM_FREE_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_REQUEST_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_REQUEST_INIT);

    MPIDI_NM_func->am_request_init(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_REQUEST_INIT);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AM_REQUEST_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AM_REQUEST_FINALIZE);

    MPIDI_NM_func->am_request_finalize(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AM_REQUEST_FINALIZE);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send(const void *buf, int count, MPI_Datatype datatype,
                                               int rank, int tag, MPIR_Comm * comm,
                                               int context_offset, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SEND);

    ret = MPIDI_NM_native_func->mpi_send(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                         request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend(const void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SSEND);

    ret = MPIDI_NM_native_func->mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                          request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SSEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_startall(int count, MPIR_Request * requests[])
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_STARTALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_STARTALL);

    ret = MPIDI_NM_native_func->mpi_startall(count, requests);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_STARTALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send_init(const void *buf, int count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset, MPIDI_av_entry_t *addr,
                                                    MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);

    ret = MPIDI_NM_native_func->mpi_send_init(buf, count, datatype, rank, tag, comm,
                                              context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset, MPIDI_av_entry_t *addr,
                                                     MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);

    ret = MPIDI_NM_native_func->mpi_ssend_init(buf, count, datatype, rank, tag, comm,
                                               context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SSEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rsend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset, MPIDI_av_entry_t *addr,
                                                     MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);

    ret = MPIDI_NM_native_func->mpi_rsend_init(buf, count, datatype, rank, tag, comm,
                                               context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RSEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bsend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset, MPIDI_av_entry_t *addr,
                                                     MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);

    ret = MPIDI_NM_native_func->mpi_bsend_init(buf, count, datatype, rank, tag, comm,
                                               context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BSEND_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISEND);

    ret = MPIDI_NM_native_func->mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                          request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf, int count, MPI_Datatype datatype,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISSEND);

    ret = MPIDI_NM_native_func->mpi_issend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                           request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISSEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);

    ret = MPIDI_NM_native_func->mpi_cancel_send(sreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CANCEL_SEND);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                                                    int rank, int tag, MPIR_Comm * comm,
                                                    int context_offset, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RECV_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RECV_INIT);

    ret = MPIDI_NM_native_func->mpi_recv_init(buf, count, datatype, rank, tag, comm,
                                              context_offset, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RECV_INIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf, int count, MPI_Datatype datatype,
                                               int rank, int tag, MPIR_Comm * comm,
                                               int context_offset, MPIDI_av_entry_t *addr, MPI_Status * status,
                                               MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RECV);

    ret = MPIDI_NM_native_func->mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                         status, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IRECV);

    ret = MPIDI_NM_native_func->mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                          request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IRECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf, int count, MPI_Datatype datatype,
                                                 MPIR_Request * message, MPIR_Request ** rreqp)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IMRECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IMRECV);

    ret = MPIDI_NM_native_func->mpi_imrecv(buf, count, datatype, message, rreqp);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IMRECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);

    ret = MPIDI_NM_native_func->mpi_cancel_recv(rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_CANCEL_RECV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX void *MPIDI_NM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    void *ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLOC_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLOC_MEM);

    ret = MPIDI_NM_native_func->mpi_alloc_mem(size, info_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLOC_MEM);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_free_mem(void *ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_FREE_MEM);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_FREE_MEM);

    ret = MPIDI_NM_native_func->mpi_free_mem(ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_FREE_MEM);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                  int context_offset, MPIDI_av_entry_t *addr, int *flag,
                                                  MPIR_Request ** message, MPI_Status * status)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IMPROBE);

    ret = MPIDI_NM_native_func->mpi_improbe(source, tag, comm, context_offset, addr, flag, message,
                                            status);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IMPROBE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIDI_av_entry_t *addr, int *flag, MPI_Status * status)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IPROBE);

    ret = MPIDI_NM_native_func->mpi_iprobe(source, tag, comm, context_offset, addr, flag, status);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IPROBE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);

    ret = MPIDI_NM_native_func->mpi_win_set_info(win, info);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SET_INFO);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win, int rank,
                                                           MPI_Aint * size, int *disp_unit,
                                                           void *baseptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);

    ret = MPIDI_NM_native_func->mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SHARED_QUERY);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_put(const void *origin_addr, int origin_count,
                                              MPI_Datatype origin_datatype, int target_rank,
                                              MPI_Aint target_disp, int target_count,
                                              MPI_Datatype target_datatype, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_PUT);

    ret = MPIDI_NM_native_func->mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_PUT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_START);

    ret = MPIDI_NM_native_func->mpi_win_start(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_START);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);

    ret = MPIDI_NM_native_func->mpi_win_complete(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_COMPLETE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_POST);

    ret = MPIDI_NM_native_func->mpi_win_post(group, assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_POST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);

    ret = MPIDI_NM_native_func->mpi_win_wait(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_WAIT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);

    ret = MPIDI_NM_native_func->mpi_win_test(win, flag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_TEST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                                   MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);

    ret = MPIDI_NM_native_func->mpi_win_lock(lock_type, rank, assert, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);

    ret = MPIDI_NM_native_func->mpi_win_unlock(rank, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);

    ret = MPIDI_NM_native_func->mpi_win_get_info(win, info_p_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_GET_INFO);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get(void *origin_addr, int origin_count,
                                              MPI_Datatype origin_datatype, int target_rank,
                                              MPI_Aint target_disp, int target_count,
                                              MPI_Datatype target_datatype, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GET);

    ret = MPIDI_NM_native_func->mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GET);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);

    ret = MPIDI_NM_native_func->mpi_win_free(win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FREE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);

    ret = MPIDI_NM_native_func->mpi_win_fence(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FENCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create(void *base, MPI_Aint length, int disp_unit,
                                                     MPIR_Info * info, MPIR_Comm * comm_ptr,
                                                     MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);

    ret = MPIDI_NM_native_func->mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_accumulate(const void *origin_addr, int origin_count,
                                                     MPI_Datatype origin_datatype, int target_rank,
                                                     MPI_Aint target_disp, int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);

    ret = MPIDI_NM_native_func->mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ACCUMULATE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);

    ret = MPIDI_NM_native_func->mpi_win_attach(win, base, size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ATTACH);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit,
                                                              MPIR_Info * info_ptr,
                                                              MPIR_Comm * comm_ptr,
                                                              void **base_ptr, MPIR_Win ** win_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);

    ret = MPIDI_NM_native_func->mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr,
                                                        base_ptr, win_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE_SHARED);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rput(const void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win, MPIDI_av_entry_t *addr,
                                               MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RPUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RPUT);

    ret = MPIDI_NM_native_func->mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, win, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RPUT);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);

    ret = MPIDI_NM_native_func->mpi_win_flush_local(rank, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);

    ret = MPIDI_NM_native_func->mpi_win_detach(win, base);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_DETACH);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype, int target_rank,
                                                           MPI_Aint target_disp, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);

    ret = MPIDI_NM_native_func->mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                     datatype, target_rank, target_disp, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_COMPARE_AND_SWAP);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_raccumulate(const void *origin_addr, int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank, MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);

    ret = MPIDI_NM_native_func->mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RACCUMULATE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
                                                          int origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr, int result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank, MPI_Aint target_disp,
                                                          int target_count,
                                                          MPI_Datatype target_datatype, MPI_Op op,
                                                          MPIR_Win * win, MPIDI_av_entry_t *addr, MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);

    ret = MPIDI_NM_native_func->mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                    result_addr, result_count, result_datatype,
                                                    target_rank, target_disp, target_count,
                                                    target_datatype, op, win, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RGET_ACCUMULATE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                                       MPI_Datatype datatype, int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);

    ret = MPIDI_NM_native_func->mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                                 target_disp, op, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_FETCH_AND_OP);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate(MPI_Aint size, int disp_unit,
                                                       MPIR_Info * info, MPIR_Comm * comm,
                                                       void *baseptr, MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);

    ret = MPIDI_NM_native_func->mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_ALLOCATE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);

    ret = MPIDI_NM_native_func->mpi_win_flush(rank, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);

    ret = MPIDI_NM_native_func->mpi_win_flush_local_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_LOCAL_ALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);

    ret = MPIDI_NM_native_func->mpi_win_unlock_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_UNLOCK_ALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                             MPIR_Win ** win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);

    ret = MPIDI_NM_native_func->mpi_win_create_dynamic(info, comm, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_CREATE_DYNAMIC);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget(void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win, MPIDI_av_entry_t *addr,
                                               MPIR_Request ** request)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_RGET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_RGET);

    ret = MPIDI_NM_native_func->mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, win, addr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_RGET);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);

    ret = MPIDI_NM_native_func->mpi_win_sync(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_SYNC);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);

    ret = MPIDI_NM_native_func->mpi_win_flush_all(win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_FLUSH_ALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get_accumulate(const void *origin_addr, int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr, int result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank, MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype, MPI_Op op,
                                                         MPIR_Win * win, MPIDI_av_entry_t *addr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GET_ACCUMULATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GET_ACCUMULATE);

    ret = MPIDI_NM_native_func->mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                   result_addr, result_count, result_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, win, addr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GET_ACCUMULATE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);

    ret = MPIDI_NM_native_func->mpi_win_lock_all(assert, win);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_WIN_LOCK_ALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rank_is_local(int target, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);

    ret = MPIDI_NM_native_func->rank_is_local(target, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_RANK_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_av_is_local(MPIDI_av_entry_t *av)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);

    ret = MPIDI_NM_native_func->av_is_local(av);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_AV_IS_LOCAL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  void * algo_parameters_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BARRIER);

    ret = MPIDI_NM_native_func->mpi_barrier(comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                                int root, MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag,
                                                void * algo_parameters_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BCAST);

    ret = MPIDI_NM_native_func->mpi_bcast(buffer, count, datatype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                                    MPI_Datatype datatype, MPI_Op op,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    void * algo_parameters_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);

    ret = MPIDI_NM_native_func->mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgather(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);

    ret = MPIDI_NM_native_func->mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const int *recvcounts, const int *displs,
                                                     MPI_Datatype recvtype, MPIR_Comm * comm,
                                                     MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);

    ret = MPIDI_NM_native_func->mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                               displs, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatter(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTER);

    ret = MPIDI_NM_native_func->mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                                   const int *displs, MPI_Datatype sendtype,
                                                   void *recvbuf, int recvcount,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTERV);

    ret = MPIDI_NM_native_func->mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                             recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gather(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype, int root,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHER);

    ret = MPIDI_NM_native_func->mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gatherv(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHERV);

    ret = MPIDI_NM_native_func->mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoall(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);

    ret = MPIDI_NM_native_func->mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                                    const int *sdispls, MPI_Datatype sendtype,
                                                    void *recvbuf, const int *recvcounts,
                                                    const int *rdispls, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);

    ret = MPIDI_NM_native_func->mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                              recvcounts, rdispls, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallw(const void *sendbuf, const int *sendcounts,
                                                    const int *sdispls,
                                                    const MPI_Datatype sendtypes[], void *recvbuf,
                                                    const int *recvcounts, const int *rdispls,
                                                    const MPI_Datatype recvtypes[],
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);

    ret = MPIDI_NM_native_func->mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                              recvcounts, rdispls, recvtypes, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                 void * algo_parameters_ptr)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE);

    ret = MPIDI_NM_native_func->mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                           errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                         const int *recvcounts,
                                                         MPI_Datatype datatype, MPI_Op op,
                                                         MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);

    ret = MPIDI_NM_native_func->mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                   comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                               int recvcount,
                                                               MPI_Datatype datatype, MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_native_func->mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op,
                                                         comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                               MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCAN);

    ret = MPIDI_NM_native_func->mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_EXSCAN);

    ret = MPIDI_NM_native_func->mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_EXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_native_func->mpi_neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const int *recvcounts,
                                                              const int *displs,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_native_func->mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcounts, displs, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                             const int *sendcounts,
                                                             const int *sdispls,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             const int *recvcounts,
                                                             const int *rdispls,
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_native_func->mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                       recvbuf, recvcounts, rdispls, recvtype,
                                                       comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                             const int *sendcounts,
                                                             const MPI_Aint * sdispls,
                                                             const MPI_Datatype * sendtypes,
                                                             void *recvbuf, const int *recvcounts,
                                                             const MPI_Aint * rdispls,
                                                             const MPI_Datatype * recvtypes,
                                                             MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_native_func->mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                       recvbuf, recvcounts, rdispls, recvtypes,
                                                       comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                            MPI_Datatype sendtype, void *recvbuf,
                                                            int recvcount, MPI_Datatype recvtype,
                                                            MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_native_func->mpi_neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              int recvcount, MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_native_func->mpi_ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                               MPI_Datatype sendtype,
                                                               void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *displs,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_native_func->mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const int *sdispls,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const int *recvcounts,
                                                              const int *rdispls,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const MPI_Aint * sdispls,
                                                              const MPI_Datatype * sendtypes,
                                                              void *recvbuf, const int *recvcounts,
                                                              const MPI_Aint * rdispls,
                                                              const MPI_Datatype * recvtypes,
                                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                        recvbuf, recvcounts, rdispls, recvtypes,
                                                        comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBARRIER);

    ret = MPIDI_NM_native_func->mpi_ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                                 int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBCAST);

    ret = MPIDI_NM_native_func->mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);

    ret = MPIDI_NM_native_func->mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);

    ret = MPIDI_NM_native_func->mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);

    ret = MPIDI_NM_native_func->mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);

    ret = MPIDI_NM_native_func->mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls, MPI_Datatype sendtype,
                                                     void *recvbuf, const int *recvcounts,
                                                     const int *rdispls, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);

    ret = MPIDI_NM_native_func->mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                               recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls,
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const int *recvcounts, const int *rdispls,
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);

    ret = MPIDI_NM_native_func->mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                               recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);

    ret = MPIDI_NM_native_func->mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHER);

    ret = MPIDI_NM_native_func->mpi_igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts, const int *displs,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHERV);

    ret = MPIDI_NM_native_func->mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                             displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_native_func->mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                                          op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                          const int *recvcounts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);

    ret = MPIDI_NM_native_func->mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                    comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op, int root,
                                                  MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE);

    ret = MPIDI_NM_native_func->mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                            req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCAN);

    ret = MPIDI_NM_native_func->mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatter(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTER);

    ret = MPIDI_NM_native_func->mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                                    const int *displs, MPI_Datatype sendtype,
                                                    void *recvbuf, int recvcount,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);

    ret = MPIDI_NM_native_func->mpi_iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                              recvcount, recvtype, root, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_type_commit_hook(MPIR_Datatype * datatype_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_TYPE_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_TYPE_CREATE_HOOK);

    ret = MPIDI_NM_native_func->mpi_type_commit_hook(datatype_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_TYPE_CREATE_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_TYPE_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_TYPE_FREE_HOOK);

    ret = MPIDI_NM_native_func->mpi_type_free_hook(datatype_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_TYPE_FREE_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_op_commit_hook(MPIR_Op * op_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_OP_COMMIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_OP_COMMIT_HOOK);

    ret = MPIDI_NM_native_func->mpi_op_commit_hook(op_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_OP_COMMIT_HOOK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_op_free_hook(MPIR_Op * op_p)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_OP_FREE_HOOK);

    ret = MPIDI_NM_native_func->mpi_op_free_hook(op_p);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_OP_FREE_HOOK);
    return ret;
}

#endif /* NETMOD_DISABLE_INLINES  */

#else
#define __netmod_direct_stubnm__   0
#define __netmod_direct_ofi__    1
#define __netmod_direct_shm__    2
#define __netmod_direct_ucx__    3
#define __netmod_direct_portals4__ 4

#if NETMOD_DIRECT==__netmod_direct_stubnm__
#include "../stubnm/netmod_direct.h"
#elif NETMOD_DIRECT==__netmod_direct_ofi__
#include "../ofi/netmod_direct.h"
#elif NETMOD_DIRECT==__netmod_direct_shm__
#include "../shm/netmod_direct.h"
#elif NETMOD_DIRECT==__netmod_direct_ucx__
#include "../ucx/netmod_direct.h"
#elif NETMOD_DIRECT==__netmod_direct_portals4__
#include "../portals4/netmod_direct.h"
#else
#error "No direct netmod included"
#endif
#endif /* NETMOD_DIRECT           */

#endif /* NETMOD_IMPL_H_INCLUDED */
