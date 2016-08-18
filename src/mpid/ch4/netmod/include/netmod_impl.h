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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_init(int rank, int size, int appnum, int *tag_ub,
                                           MPIR_Comm * comm_world, MPIR_Comm * comm_self,
                                           int spawned, int num_contexts, void **netmod_contexts)
{
    return MPIDI_NM_func->init(rank, size, appnum, tag_ub, comm_world, comm_self, spawned,
                               num_contexts, netmod_contexts);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_finalize(void)
{
    return MPIDI_NM_func->finalize();
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_progress(void *netmod_context, int blocking)
{
    return MPIDI_NM_func->progress(netmod_context, blocking);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_reg_hdr_handler(int handler_id,
                                                      MPIDI_NM_am_origin_handler_fn
                                                      origin_handler_fn,
                                                      MPIDI_NM_am_target_handler_fn
                                                      target_handler_fn)
{
    return MPIDI_NM_func->reg_hdr_handler(handler_id, origin_handler_fn, target_handler_fn);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_connect(const char *port_name, MPIR_Info * info,
                                                   int root, MPIR_Comm * comm,
                                                   MPIR_Comm ** newcomm_ptr)
{
    return MPIDI_NM_func->comm_connect(port_name, info, root, comm, newcomm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_disconnect(MPIR_Comm * comm_ptr)
{
    return MPIDI_NM_func->comm_disconnect(comm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_open_port(MPIR_Info * info_ptr, char *port_name)
{
    return MPIDI_NM_func->open_port(info_ptr, port_name);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_close_port(const char *port_name)
{
    return MPIDI_NM_func->close_port(port_name);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_accept(const char *port_name, MPIR_Info * info,
                                                  int root, MPIR_Comm * comm,
                                                  MPIR_Comm ** newcomm_ptr)
{
    return MPIDI_NM_func->comm_accept(port_name, info, root, comm, newcomm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_am_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, size_t am_hdr_sz,
                                                  MPIR_Request * sreq, void *netmod_context)
{
    return MPIDI_NM_func->send_am_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz, sreq,
                                      netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_inject_am_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                    const void *am_hdr, size_t am_hdr_sz,
                                                    void *netmod_context)
{
    return MPIDI_NM_func->inject_am_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz, netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_am(int rank, MPIR_Comm * comm, int handler_id,
                                              const void *am_hdr, size_t am_hdr_sz,
                                              const void *data, MPI_Count count,
                                              MPI_Datatype datatype, MPIR_Request * sreq,
                                              void *netmod_context)
{
    return MPIDI_NM_func->send_am(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                  sreq, netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_amv(int rank, MPIR_Comm * comm, int handler_id,
                                               struct iovec *am_hdrs, size_t iov_len,
                                               const void *data, MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq,
                                               void *netmod_context)
{
    return MPIDI_NM_func->send_amv(rank, comm, handler_id, am_hdrs, iov_len, data, count, datatype,
                                   sreq, netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_amv_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                   struct iovec *am_hdrs, size_t iov_len,
                                                   MPIR_Request * sreq, void *netmod_context)
{
    return MPIDI_NM_func->send_amv_hdr(rank, comm, handler_id, am_hdrs, iov_len, sreq,
                                       netmod_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_am_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank, int handler_id,
                                                        const void *am_hdr, size_t am_hdr_sz,
                                                        MPIR_Request * sreq)
{
    return MPIDI_NM_func->send_am_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz,
                                            sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_inject_am_hdr_reply(MPIR_Context_id_t context_id,
                                                          int src_rank, int handler_id,
                                                          const void *am_hdr, size_t am_hdr_sz)
{
    return MPIDI_NM_func->inject_am_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_am_reply(MPIR_Context_id_t context_id, int src_rank,
                                                    int handler_id, const void *am_hdr,
                                                    size_t am_hdr_sz, const void *data,
                                                    MPI_Count count, MPI_Datatype datatype,
                                                    MPIR_Request * sreq)
{
    return MPIDI_NM_func->send_am_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz, data,
                                        count, datatype, sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_amv_reply(MPIR_Context_id_t context_id,
                                                     int src_rank, int handler_id,
                                                     struct iovec *am_hdr, size_t iov_len,
                                                     const void *data, MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq)
{
    return MPIDI_NM_func->send_amv_reply(context_id, src_rank, handler_id, am_hdr, iov_len, data,
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

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gpid_get(MPIR_Comm * comm_ptr, int rank, MPIR_Gpid * gpid)
{
    return MPIDI_NM_func->gpid_get(comm_ptr, rank, gpid);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_getallincomm(MPIR_Comm * comm_ptr, int local_size,
                                                   MPIR_Gpid local_gpid[], int *singleAVT)
{
    return MPIDI_NM_func->getallincomm(comm_ptr, local_size, local_gpid, singleAVT);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gpid_tolpidarray(int size, MPIR_Gpid gpid[], int lpid[])
{
    return MPIDI_NM_func->gpid_tolpidarray(size, gpid, lpid);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                  int size, const int lpids[])
{
    return MPIDI_NM_func->create_intercomm_from_lpids(newcomm_ptr, size, lpids);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_create_hook(MPIR_Comm * comm)
{
    return MPIDI_NM_func->comm_create_hook(comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_free_hook(MPIR_Comm * comm)
{
    return MPIDI_NM_func->comm_free_hook(comm);
};

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{
    return MPIDI_NM_func->am_request_init(req);
};

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{
    return MPIDI_NM_func->am_request_finalize(req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send(const void *buf, int count, MPI_Datatype datatype,
                                           int rank, int tag, MPIR_Comm * comm,
                                           int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->send(buf, count, datatype, rank, tag, comm, context_offset,
                                      request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ssend(const void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->ssend(buf, count, datatype, rank, tag, comm, context_offset,
                                       request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_startall(int count, MPIR_Request * requests[])
{
    return MPIDI_NM_native_func->startall(count, requests);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_init(const void *buf, int count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIR_Request ** request)
{
    return MPIDI_NM_native_func->send_init(buf, count, datatype, rank, tag, comm, context_offset,
                                           request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ssend_init(const void *buf, int count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request)
{
    return MPIDI_NM_native_func->ssend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                            request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rsend_init(const void *buf, int count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request)
{
    return MPIDI_NM_native_func->rsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                            request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_bsend_init(const void *buf, int count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request)
{
    return MPIDI_NM_native_func->bsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                            request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_isend(const void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->isend(buf, count, datatype, rank, tag, comm, context_offset,
                                       request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_issend(const void *buf, int count, MPI_Datatype datatype,
                                             int rank, int tag, MPIR_Comm * comm,
                                             int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->issend(buf, count, datatype, rank, tag, comm, context_offset,
                                        request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_cancel_send(MPIR_Request * sreq)
{
    return MPIDI_NM_native_func->cancel_send(sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_recv_init(void *buf, int count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->recv_init(buf, count, datatype, rank, tag, comm, context_offset,
                                           request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_recv(void *buf, int count, MPI_Datatype datatype,
                                           int rank, int tag, MPIR_Comm * comm,
                                           int context_offset, MPI_Status * status,
                                           MPIR_Request ** request)
{
    return MPIDI_NM_native_func->recv(buf, count, datatype, rank, tag, comm, context_offset, status,
                                      request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_irecv(void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                       request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_imrecv(void *buf, int count, MPI_Datatype datatype,
                                             MPIR_Request * message, MPIR_Request ** rreqp)
{
    return MPIDI_NM_native_func->imrecv(buf, count, datatype, message, rreqp);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_cancel_recv(MPIR_Request * rreq)
{
    return MPIDI_NM_native_func->cancel_recv(rreq);
};

MPL_STATIC_INLINE_PREFIX void *MPIDI_NM_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDI_NM_native_func->alloc_mem(size, info_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_free_mem(void *ptr)
{
    return MPIDI_NM_native_func->free_mem(ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_improbe(int source, int tag, MPIR_Comm * comm,
                                              int context_offset, int *flag,
                                              MPIR_Request ** message, MPI_Status * status)
{
    return MPIDI_NM_native_func->improbe(source, tag, comm, context_offset, flag, message, status);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iprobe(int source, int tag, MPIR_Comm * comm,
                                             int context_offset, int *flag, MPI_Status * status)
{
    return MPIDI_NM_native_func->iprobe(source, tag, comm, context_offset, flag, status);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    return MPIDI_NM_native_func->win_set_info(win, info);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_shared_query(MPIR_Win * win, int rank,
                                                       MPI_Aint * size, int *disp_unit,
                                                       void *baseptr)
{
    return MPIDI_NM_native_func->win_shared_query(win, rank, size, disp_unit, baseptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_put(const void *origin_addr, int origin_count,
                                          MPI_Datatype origin_datatype, int target_rank,
                                          MPI_Aint target_disp, int target_count,
                                          MPI_Datatype target_datatype, MPIR_Win * win)
{
    return MPIDI_NM_native_func->put(origin_addr, origin_count, origin_datatype, target_rank,
                                     target_disp, target_count, target_datatype, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_start(group, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_complete(MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_complete(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_post(group, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_wait(MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_wait(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_test(MPIR_Win * win, int *flag)
{
    return MPIDI_NM_native_func->win_test(win, flag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_lock(lock_type, rank, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_unlock(int rank, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_unlock(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDI_NM_native_func->win_get_info(win, info_p_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_get(void *origin_addr, int origin_count,
                                          MPI_Datatype origin_datatype, int target_rank,
                                          MPI_Aint target_disp, int target_count,
                                          MPI_Datatype target_datatype, MPIR_Win * win)
{
    return MPIDI_NM_native_func->get(origin_addr, origin_count, origin_datatype, target_rank,
                                     target_disp, target_count, target_datatype, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_free(MPIR_Win ** win_ptr)
{
    return MPIDI_NM_native_func->win_free(win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_fence(int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_fence(assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_create(void *base, MPI_Aint length, int disp_unit,
                                                 MPIR_Info * info, MPIR_Comm * comm_ptr,
                                                 MPIR_Win ** win_ptr)
{
    return MPIDI_NM_native_func->win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_accumulate(const void *origin_addr, int origin_count,
                                                 MPI_Datatype origin_datatype, int target_rank,
                                                 MPI_Aint target_disp, int target_count,
                                                 MPI_Datatype target_datatype, MPI_Op op,
                                                 MPIR_Win * win)
{
    return MPIDI_NM_native_func->accumulate(origin_addr, origin_count, origin_datatype, target_rank,
                                            target_disp, target_count, target_datatype, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPIDI_NM_native_func->win_attach(win, base, size);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_allocate_shared(MPI_Aint size, int disp_unit,
                                                          MPIR_Info * info_ptr,
                                                          MPIR_Comm * comm_ptr,
                                                          void **base_ptr, MPIR_Win ** win_ptr)
{
    return MPIDI_NM_native_func->win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                                     win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rput(const void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, int target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win,
                                           MPIR_Request ** request)
{
    return MPIDI_NM_native_func->rput(origin_addr, origin_count, origin_datatype, target_rank,
                                      target_disp, target_count, target_datatype, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush_local(int rank, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_flush_local(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_detach(MPIR_Win * win, const void *base)
{
    return MPIDI_NM_native_func->win_detach(win, base);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_compare_and_swap(const void *origin_addr,
                                                       const void *compare_addr,
                                                       void *result_addr,
                                                       MPI_Datatype datatype, int target_rank,
                                                       MPI_Aint target_disp, MPIR_Win * win)
{
    return MPIDI_NM_native_func->compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                                  target_rank, target_disp, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_raccumulate(const void *origin_addr, int origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  int target_rank, MPI_Aint target_disp,
                                                  int target_count,
                                                  MPI_Datatype target_datatype, MPI_Op op,
                                                  MPIR_Win * win, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->raccumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rget_accumulate(const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      void *result_addr, int result_count,
                                                      MPI_Datatype result_datatype,
                                                      int target_rank, MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win, MPIR_Request ** request)
{
    return MPIDI_NM_native_func->rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_fetch_and_op(const void *origin_addr, void *result_addr,
                                                   MPI_Datatype datatype, int target_rank,
                                                   MPI_Aint target_disp, MPI_Op op, MPIR_Win * win)
{
    return MPIDI_NM_native_func->fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                              target_disp, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_allocate(MPI_Aint size, int disp_unit,
                                                   MPIR_Info * info, MPIR_Comm * comm,
                                                   void *baseptr, MPIR_Win ** win)
{
    return MPIDI_NM_native_func->win_allocate(size, disp_unit, info, comm, baseptr, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush(int rank, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_flush(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush_local_all(MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_flush_local_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_unlock_all(MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_unlock_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                         MPIR_Win ** win)
{
    return MPIDI_NM_native_func->win_create_dynamic(info, comm, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rget(void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, int target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win,
                                           MPIR_Request ** request)
{
    return MPIDI_NM_native_func->rget(origin_addr, origin_count, origin_datatype, target_rank,
                                      target_disp, target_count, target_datatype, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_sync(MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_sync(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_flush_all(MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_flush_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_get_accumulate(const void *origin_addr, int origin_count,
                                                     MPI_Datatype origin_datatype,
                                                     void *result_addr, int result_count,
                                                     MPI_Datatype result_datatype,
                                                     int target_rank, MPI_Aint target_disp,
                                                     int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win)
{
    return MPIDI_NM_native_func->get_accumulate(origin_addr, origin_count, origin_datatype,
                                                result_addr, result_count, result_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDI_NM_native_func->win_lock_all(assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rank_is_local(int target, MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->rank_is_local(target, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->barrier(comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_bcast(void *buffer, int count, MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->bcast(buffer, count, datatype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_allreduce(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_allgather(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_allgatherv(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 const int *recvcounts, const int *displs,
                                                 MPI_Datatype recvtype, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_scatter(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              int recvcount, MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_scatterv(const void *sendbuf, const int *sendcounts,
                                               const int *displs, MPI_Datatype sendtype,
                                               void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype, int root,
                                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_gatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                         recvtype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_alltoall(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype,
                                               MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_alltoallv(const void *sendbuf, const int *sendcounts,
                                                const int *sdispls, MPI_Datatype sendtype,
                                                void *recvbuf, const int *recvcounts,
                                                const int *rdispls, MPI_Datatype recvtype,
                                                MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                           recvcounts, rdispls, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_alltoallw(const void *sendbuf, const int *sendcounts,
                                                const int *sdispls,
                                                const MPI_Datatype sendtypes[], void *recvbuf,
                                                const int *recvcounts, const int *rdispls,
                                                const MPI_Datatype recvtypes[],
                                                MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                           recvcounts, rdispls, recvtypes, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_reduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, int root,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                        errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                     const int *recvcounts,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                           int recvcount,
                                                           MPI_Datatype datatype, MPI_Op op,
                                                           MPIR_Comm * comm_ptr,
                                                           MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op,
                                                      comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_scan(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_exscan(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_NM_native_func->exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_allgather(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int *recvcounts,
                                                          const int *displs,
                                                          MPI_Datatype recvtype, MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcounts, displs, recvtype, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_alltoallv(const void *sendbuf,
                                                         const int *sendcounts,
                                                         const int *sdispls,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         const int *recvcounts,
                                                         const int *rdispls,
                                                         MPI_Datatype recvtype, MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                    recvcounts, rdispls, recvtype, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_alltoallw(const void *sendbuf,
                                                         const int *sendcounts,
                                                         const MPI_Aint * sdispls,
                                                         const MPI_Datatype * sendtypes,
                                                         void *recvbuf, const int *recvcounts,
                                                         const MPI_Aint * rdispls,
                                                         const MPI_Datatype * recvtypes,
                                                         MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                    recvbuf, recvcounts, rdispls, recvtypes, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        MPIR_Comm * comm)
{
    return MPIDI_NM_native_func->neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          int recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype,
                                                           void *recvbuf,
                                                           const int *recvcounts,
                                                           const int *displs,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_alltoallv(const void *sendbuf,
                                                          const int *sendcounts,
                                                          const int *sdispls,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int *recvcounts,
                                                          const int *rdispls,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                     recvbuf, recvcounts, rdispls, recvtype, comm,
                                                     req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ineighbor_alltoallw(const void *sendbuf,
                                                          const int *sendcounts,
                                                          const MPI_Aint * sdispls,
                                                          const MPI_Datatype * sendtypes,
                                                          void *recvbuf, const int *recvcounts,
                                                          const MPI_Aint * rdispls,
                                                          const MPI_Datatype * recvtypes,
                                                          MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                     recvbuf, recvcounts, rdispls, recvtypes, comm,
                                                     req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ibarrier(comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                             int root, MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ibcast(buffer, count, datatype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iallgather(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iallgatherv(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, MPIR_Comm * comm,
                                                  MPI_Request * req)
{
    return MPIDI_NM_native_func->iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                             displs, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ialltoall(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                 const int *sdispls, MPI_Datatype sendtype,
                                                 void *recvbuf, const int *recvcounts,
                                                 const int *rdispls, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                            recvcounts, rdispls, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                 const int *sdispls,
                                                 const MPI_Datatype sendtypes[], void *recvbuf,
                                                 const int *recvcounts, const int *rdispls,
                                                 const MPI_Datatype recvtypes[],
                                                 MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                            recvcounts, rdispls, recvtypes, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iexscan(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_igather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              int recvcount, MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_igatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                          recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                            int recvcount,
                                                            MPI_Datatype datatype, MPI_Op op,
                                                            MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op,
                                                       comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                      const int *recvcounts,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                                 req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_ireduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, int root,
                                              MPIR_Comm * comm_ptr, MPI_Request * req)
{
    return MPIDI_NM_native_func->ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                         req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iscan(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    return MPIDI_NM_native_func->iscan(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iscatter(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_NM_native_func->iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_iscatterv(const void *sendbuf, const int *sendcounts,
                                                const int *displs, MPI_Datatype sendtype,
                                                void *recvbuf, int recvcount,
                                                MPI_Datatype recvtype, int root,
                                                MPIR_Comm * comm_ptr, MPI_Request * req)
{
    return MPIDI_NM_native_func->iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                           recvcount, recvtype, root, comm_ptr, req);
};

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_type_dup_hook(MPIR_Datatype * old_datatype_p,
                                                     MPIR_Datatype * new_datatype_p)
{
    return MPIDI_NM_native_func->type_dup_hook(old_datatype_p, new_datatype_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_type_create_hook(MPIR_Datatype * datatype_p)
{
    return MPIDI_NM_native_func->type_create_hook(datatype_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_type_free_hook(MPIR_Datatype * datatype_p)
{
    return MPIDI_NM_native_func->type_free_hook(datatype_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_op_create_hook(MPIR_Op * op_p)
{
    return MPIDI_NM_native_func->op_create_hook(op_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_op_free_hook(MPIR_Op * op_p)
{
    return MPIDI_NM_native_func->op_free_hook(op_p);
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
