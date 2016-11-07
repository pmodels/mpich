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
#ifndef NETMOD_IMPL_PROTOTYPES_H_INCLUDED
#define NETMOD_IMPL_PROTOTYPES_H_INCLUDED


#ifndef NETMOD_DIRECT
#ifndef NETMOD_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_init_hook(int rank, int size, int appnum, int *tag_ub,
                                                    MPIR_Comm * comm_world, MPIR_Comm * comm_self,
                                                    int spawned, int num_contexts,
                                                    void **netmod_contexts)
{
    return MPIDI_NM_func->mpi_init(rank, size, appnum, tag_ub, comm_world, comm_self, spawned,
                                   num_contexts, netmod_contexts);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_finalize_hook(void)
{
    return MPIDI_NM_func->mpi_finalize();
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(void *netmod_context, int blocking)
{
    return MPIDI_NM_func->progress(netmod_context, blocking);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_reg_cb(int handler_id,
                                                MPIDI_NM_am_origin_cb
                                                origin_cb, MPIDI_NM_am_target_msg_cb target_msg_cb)
{
    return MPIDI_NM_func->am_reg_cb(handler_id, origin_cb, target_msg_cb);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_connect(const char *port_name, MPIR_Info * info,
                                                       int root, MPIR_Comm * comm,
                                                       MPIR_Comm ** newcomm_ptr)
{
    return MPIDI_NM_func->mpi_comm_connect(port_name, info, root, comm, newcomm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_disconnect(MPIR_Comm * comm_ptr)
{
    return MPIDI_NM_func->mpi_comm_disconnect(comm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_open_port(MPIR_Info * info_ptr, char *port_name)
{
    return MPIDI_NM_func->mpi_open_port(info_ptr, port_name);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_close_port(const char *port_name)
{
    return MPIDI_NM_func->mpi_close_port(port_name);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_accept(const char *port_name, MPIR_Info * info,
                                                      int root, MPIR_Comm * comm,
                                                      MPIR_Comm ** newcomm_ptr)
{
    return MPIDI_NM_func->mpi_comm_accept(port_name, info, root, comm, newcomm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, size_t am_hdr_sz,
                                                  void *netmod_context)
{
    return MPIDI_NM_func->am_send_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz, netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, size_t am_hdr_sz,
                                               const void *data, MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq,
                                               void *netmod_context)
{
    return MPIDI_NM_func->am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                   sreq, netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                                struct iovec *am_hdrs, size_t iov_len,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype, MPIR_Request * sreq,
                                                void *netmod_context)
{
    return MPIDI_NM_func->am_isendv(rank, comm, handler_id, am_hdrs, iov_len, data, count, datatype,
                                    sreq, netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank, int handler_id,
                                                        const void *am_hdr, size_t am_hdr_sz)
{
    return MPIDI_NM_func->am_send_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                     int handler_id, const void *am_hdr,
                                                     size_t am_hdr_sz, const void *data,
                                                     MPI_Count count, MPI_Datatype datatype,
                                                     MPIR_Request * sreq)
{
    return MPIDI_NM_func->am_isend_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz, data,
                                         count, datatype, sreq);
};

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void)
{
    return MPIDI_NM_func->am_hdr_max_sz();
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_recv(MPIR_Request * req)
{
    return MPIDI_NM_func->am_recv(req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                    int *lpid_ptr, MPL_bool is_remote)
{
    return MPIDI_NM_func->comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_get_local_upids(MPIR_Comm * comm, size_t ** local_upid_size,
                                                      char **local_upids)
{
    return MPIDI_NM_func->get_local_upids(comm, local_upid_size, local_upids);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_upids_to_lupids(int size, size_t * remote_upid_size,
                                                      char *remote_upids, int **remote_lupids)
{
    return MPIDI_NM_func->upids_to_lupids(size, remote_upid_size, remote_upids, remote_lupids);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                  int size, const int lpids[])
{
    return MPIDI_NM_func->create_intercomm_from_lpids(newcomm_ptr, size, lpids);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_create_hook(MPIR_Comm * comm)
{
    return MPIDI_NM_func->mpi_comm_create_hook(comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_comm_free_hook(MPIR_Comm * comm)
{
    return MPIDI_NM_func->mpi_comm_free_hook(comm);
};

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    return MPIDI_NM_func->am_request_init(req);
};

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    return MPIDI_NM_func->am_request_finalize(req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send(const void *buf, int count, MPI_Datatype datatype,
                                               int rank, int tag, MPIR_Comm * comm,
                                               int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_send(buf, count, datatype, rank, tag, comm, context_offset,
                                          request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend(const void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset,
                                           request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_startall(int count, MPIR_Request * requests[])
{
    return MPIDI_NM_native_func->mpi_startall(count, requests);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send_init(const void *buf, int count,
                                                    MPI_Datatype datatype, int rank, int tag,
                                                    MPIR_Comm * comm, int context_offset,
                                                    MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_send_init(buf, count, datatype, rank, tag, comm,
                                               context_offset, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_ssend_init(buf, count, datatype, rank, tag, comm,
                                                context_offset, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rsend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_rsend_init(buf, count, datatype, rank, tag, comm,
                                                context_offset, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bsend_init(const void *buf, int count,
                                                     MPI_Datatype datatype, int rank, int tag,
                                                     MPIR_Comm * comm, int context_offset,
                                                     MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_bsend_init(buf, count, datatype, rank, tag, comm,
                                                context_offset, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_isend(buf, count, datatype, rank, tag, comm, context_offset,
                                           request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf, int count, MPI_Datatype datatype,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_issend(buf, count, datatype, rank, tag, comm, context_offset,
                                            request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    return MPIDI_NM_native_func->mpi_cancel_send(sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv_init(void *buf, int count, MPI_Datatype datatype,
                                                    int rank, int tag, MPIR_Comm * comm,
                                                    int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_recv_init(buf, count, datatype, rank, tag, comm,
                                               context_offset, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf, int count, MPI_Datatype datatype,
                                               int rank, int tag, MPIR_Comm * comm,
                                               int context_offset, MPI_Status * status,
                                               MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_recv(buf, count, datatype, rank, tag, comm, context_offset,
                                          status, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                           request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf, int count, MPI_Datatype datatype,
                                                 MPIR_Request * message, MPIR_Request ** rreqp)
{
    return MPIDI_NM_native_func->mpi_imrecv(buf, count, datatype, message, rreqp);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{
    return MPIDI_NM_native_func->mpi_cancel_recv(rreq);
};

MPL_STATIC_INLINE_PREFIX void *MPIDI_NM_mpi_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDI_NM_native_func->mpi_alloc_mem(size, info_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_free_mem(void *ptr)
{
    return MPIDI_NM_native_func->mpi_free_mem(ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                  int context_offset, int *flag,
                                                  MPIR_Request ** message, MPI_Status * status)
{
    return MPIDI_NM_native_func->mpi_improbe(source, tag, comm, context_offset, flag, message,
                                             status);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                                 int context_offset, int *flag, MPI_Status * status)
{
    return MPIDI_NM_native_func->mpi_iprobe(source, tag, comm, context_offset, flag, status);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    return MPIDI_NM_native_func->mpi_win_set_info(win, info);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win, int rank,
                                                           MPI_Aint * size, int *disp_unit,
                                                           void *baseptr)
{
    return MPIDI_NM_native_func->mpi_win_shared_query(win, rank, size, disp_unit, baseptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_put(const void *origin_addr, int origin_count,
                                              MPI_Datatype origin_datatype, int target_rank,
                                              MPI_Aint target_disp, int target_count,
                                              MPI_Datatype target_datatype, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_start(group, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_complete(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_post(group, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_wait(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    return MPIDI_NM_native_func->mpi_win_test(win, flag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                                   MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_lock(lock_type, rank, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_unlock(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDI_NM_native_func->mpi_win_get_info(win, info_p_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get(void *origin_addr, int origin_count,
                                              MPI_Datatype origin_datatype, int target_rank,
                                              MPI_Aint target_disp, int target_count,
                                              MPI_Datatype target_datatype, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_free(MPIR_Win ** win_ptr)
{
    return MPIDI_NM_native_func->mpi_win_free(win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_fence(assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create(void *base, MPI_Aint length, int disp_unit,
                                                     MPIR_Info * info, MPIR_Comm * comm_ptr,
                                                     MPIR_Win ** win_ptr)
{
    return MPIDI_NM_native_func->mpi_win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_accumulate(const void *origin_addr, int origin_count,
                                                     MPI_Datatype origin_datatype, int target_rank,
                                                     MPI_Aint target_disp, int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPIDI_NM_native_func->mpi_win_attach(win, base, size);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate_shared(MPI_Aint size, int disp_unit,
                                                              MPIR_Info * info_ptr,
                                                              MPIR_Comm * comm_ptr,
                                                              void **base_ptr, MPIR_Win ** win_ptr)
{
    return MPIDI_NM_native_func->mpi_win_allocate_shared(size, disp_unit, info_ptr, comm_ptr,
                                                         base_ptr, win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rput(const void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win,
                                               MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_flush_local(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_detach(MPIR_Win * win, const void *base)
{
    return MPIDI_NM_native_func->mpi_win_detach(win, base);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype, int target_rank,
                                                           MPI_Aint target_disp, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                      datatype, target_rank, target_disp, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_raccumulate(const void *origin_addr, int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank, MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget_accumulate(const void *origin_addr,
                                                          int origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr, int result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank, MPI_Aint target_disp,
                                                          int target_count,
                                                          MPI_Datatype target_datatype, MPI_Op op,
                                                          MPIR_Win * win, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                     result_addr, result_count, result_datatype,
                                                     target_rank, target_disp, target_count,
                                                     target_datatype, op, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                                       MPI_Datatype datatype, int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                                  target_disp, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_allocate(MPI_Aint size, int disp_unit,
                                                       MPIR_Info * info, MPIR_Comm * comm,
                                                       void *baseptr, MPIR_Win ** win)
{
    return MPIDI_NM_native_func->mpi_win_allocate(size, disp_unit, info, comm, baseptr, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_flush(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_flush_local_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_unlock_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                             MPIR_Win ** win)
{
    return MPIDI_NM_native_func->mpi_win_create_dynamic(info, comm, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget(void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win,
                                               MPIR_Request ** request)
{
    return MPIDI_NM_native_func->mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_sync(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_flush_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get_accumulate(const void *origin_addr, int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr, int result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank, MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype, MPI_Op op,
                                                         MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                    result_addr, result_count, result_datatype,
                                                    target_rank, target_disp, target_count,
                                                    target_datatype, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->mpi_win_lock_all(assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rank_is_local(int target, MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->rank_is_local(target, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_barrier(comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                                int root, MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_bcast(buffer, count, datatype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                                    MPI_Datatype datatype, MPI_Op op,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm,
                                               errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgather(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const int *recvcounts, const int *displs,
                                                     MPI_Datatype recvtype, MPIR_Comm * comm,
                                                     MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                displs, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatter(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                                   const int *displs, MPI_Datatype sendtype,
                                                   void *recvbuf, int recvcount,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                              recvcount, recvtype, root, comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gather(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype, int root,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gatherv(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                             displs, recvtype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoall(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                                    const int *sdispls, MPI_Datatype sendtype,
                                                    void *recvbuf, const int *recvcounts,
                                                    const int *rdispls, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                               recvcounts, rdispls, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallw(const void *sendbuf, const int *sendcounts,
                                                    const int *sdispls,
                                                    const MPI_Datatype sendtypes[], void *recvbuf,
                                                    const int *recvcounts, const int *rdispls,
                                                    const MPI_Datatype recvtypes[],
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                               recvcounts, rdispls, recvtypes, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                            errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                         const int *recvcounts,
                                                         MPI_Datatype datatype, MPI_Op op,
                                                         MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                    comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                               int recvcount,
                                                               MPI_Datatype datatype, MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op,
                                                          comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                               MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->mpi_neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const int *recvcounts,
                                                              const int *displs,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcounts, displs, recvtype, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                             const int *sendcounts,
                                                             const int *sdispls,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             const int *recvcounts,
                                                             const int *rdispls,
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                             const int *sendcounts,
                                                             const MPI_Aint * sdispls,
                                                             const MPI_Datatype * sendtypes,
                                                             void *recvbuf, const int *recvcounts,
                                                             const MPI_Aint * rdispls,
                                                             const MPI_Datatype * recvtypes,
                                                             MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                        recvbuf, recvcounts, rdispls, recvtypes,
                                                        comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                            MPI_Datatype sendtype, void *recvbuf,
                                                            int recvcount, MPI_Datatype recvtype,
                                                            MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->mpi_neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              int recvcount, MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                               MPI_Datatype sendtype,
                                                               void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *displs,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                          recvcounts, displs, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const int *sdispls,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const int *recvcounts,
                                                              const int *rdispls,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                         recvbuf, recvcounts, rdispls, recvtype,
                                                         comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const MPI_Aint * sdispls,
                                                              const MPI_Datatype * sendtypes,
                                                              void *recvbuf, const int *recvcounts,
                                                              const MPI_Aint * rdispls,
                                                              const MPI_Datatype * recvtypes,
                                                              MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                         recvbuf, recvcounts, rdispls, recvtypes,
                                                         comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ibarrier(comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                                 int root, MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ibcast(buffer, count, datatype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                 displs, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls, MPI_Datatype sendtype,
                                                     void *recvbuf, const int *recvcounts,
                                                     const int *rdispls, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                recvcounts, rdispls, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls,
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const int *recvcounts, const int *rdispls,
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                recvcounts, rdispls, recvtypes, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts, const int *displs,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                              displs, recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                                           op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                          const int *recvcounts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                     comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op, int root,
                                                  MPIR_Comm * comm_ptr, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                             req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatter(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                                    const int *displs, MPI_Datatype sendtype,
                                                    void *recvbuf, int recvcount,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm_ptr, MPI_Request * req)
{
    return MPIDI_NM_native_func->mpi_iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                               recvcount, recvtype, root, comm_ptr, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_type_create_hook(MPIR_Datatype * datatype_p)
{
    return MPIDI_NM_native_func->mpi_type_create_hook(datatype_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_type_free_hook(MPIR_Datatype * datatype_p)
{
    return MPIDI_NM_native_func->mpi_type_free_hook(datatype_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_op_create_hook(MPIR_Op * op_p)
{
    return MPIDI_NM_native_func->mpi_op_create_hook(op_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_op_free_hook(MPIR_Op * op_p)
{
    return MPIDI_NM_native_func->mpi_op_free_hook(op_p);
};

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

#endif
