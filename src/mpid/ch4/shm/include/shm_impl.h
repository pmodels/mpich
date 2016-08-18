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
/* ch4 shm functions */
#ifndef SHM_IMPL_PROTOTYPES_H_INCLUDED
#define SHM_IMPL_PROTOTYPES_H_INCLUDED

#ifndef SHM_DIRECT
#ifndef SHM_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_init(int rank, int size)
{
    return MPIDI_SHM_func->init(rank, size);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_finalize(void)
{
    return MPIDI_SHM_func->finalize();
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_progress(int blocking)
{
    return MPIDI_SHM_func->progress(blocking);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_reg_hdr_handler(int handler_id,
                                                       MPIDI_SHM_am_origin_handler_fn
                                                       origin_handler_fn,
                                                       MPIDI_SHM_am_target_handler_fn
                                                       target_handler_fn)
{
    return MPIDI_SHM_func->reg_hdr_handler(handler_id, origin_handler_fn, target_handler_fn);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_connect(const char *port_name, MPIR_Info * info,
                                                    int root, MPIR_Comm * comm,
                                                    MPIR_Comm ** newcomm_ptr)
{
    return MPIDI_SHM_func->comm_connect(port_name, info, root, comm, newcomm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_disconnect(MPIR_Comm * comm_ptr)
{
    return MPIDI_SHM_func->comm_disconnect(comm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_open_port(MPIR_Info * info_ptr, char *port_name)
{
    return MPIDI_SHM_func->open_port(info_ptr, port_name);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_close_port(const char *port_name)
{
    return MPIDI_SHM_func->close_port(port_name);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_accept(const char *port_name, MPIR_Info * info,
                                                   int root, MPIR_Comm * comm,
                                                   MPIR_Comm ** newcomm_ptr)
{
    return MPIDI_SHM_func->comm_accept(port_name, info, root, comm, newcomm_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_am_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                   const void *am_hdr, size_t am_hdr_sz,
                                                   MPIR_Request * sreq, void *shm_context)
{
    return MPIDI_SHM_func->send_am_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz, sreq,
                                       shm_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_inject_am_hdr(int rank, MPIR_Comm * comm,
                                                     int handler_id, const void *am_hdr,
                                                     size_t am_hdr_sz, void *shm_context)
{
    return MPIDI_SHM_func->inject_am_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz, shm_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_am(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, size_t am_hdr_sz,
                                               const void *data, MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq,
                                               void *shm_context)
{
    return MPIDI_SHM_func->send_am(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                   sreq, shm_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_inject_am(int rank, MPIR_Comm * comm, int handler_id,
                                                 const void *am_hdr, size_t am_hdr_sz,
                                                 const void *data, MPI_Count count,
                                                 MPI_Datatype datatype, void *shm_context)
{
    return MPIDI_SHM_func->inject_am(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count,
                                     datatype, shm_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_amv(int rank, MPIR_Comm * comm, int handler_id,
                                                struct iovec *am_hdrs, size_t iov_len,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype, MPIR_Request * sreq,
                                                void *shm_context)
{
    return MPIDI_SHM_func->send_amv(rank, comm, handler_id, am_hdrs, iov_len, data, count, datatype,
                                    sreq, shm_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_inject_amv(int rank, MPIR_Comm * comm, int handler_id,
                                                  struct iovec *am_hdrs, size_t iov_len,
                                                  const void *data, MPI_Count count,
                                                  MPI_Datatype datatype, void *shm_context)
{
    return MPIDI_SHM_func->inject_amv(rank, comm, handler_id, am_hdrs, iov_len, data, count,
                                      datatype, shm_context);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_am_hdr_reply(MPIR_Context_id_t context_id,
                                                         int src_rank, int handler_id,
                                                         const void *am_hdr, size_t am_hdr_sz,
                                                         MPIR_Request * sreq)
{
    return MPIDI_SHM_func->send_am_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz,
                                             sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_inject_am_hdr_reply(MPIR_Context_id_t context_id,
                                                           int src_rank, int handler_id,
                                                           const void *am_hdr, size_t am_hdr_sz)
{
    return MPIDI_SHM_func->inject_am_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_am_reply(MPIR_Context_id_t context_id,
                                                     int src_rank, int handler_id,
                                                     const void *am_hdr, size_t am_hdr_sz,
                                                     const void *data, MPI_Count count,
                                                     MPI_Datatype datatype, MPIR_Request * sreq)
{
    return MPIDI_SHM_func->send_am_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz, data,
                                         count, datatype, sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_inject_am_reply(MPIR_Context_id_t context_id,
                                                       int src_rank, int handler_id,
                                                       const void *am_hdr, size_t am_hdr_sz,
                                                       const void *data, MPI_Count count,
                                                       MPI_Datatype datatype)
{
    return MPIDI_SHM_func->inject_am_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz,
                                           data, count, datatype);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_amv_reply(MPIR_Context_id_t context_id,
                                                      int src_rank, int handler_id,
                                                      struct iovec *am_hdr, size_t iov_len,
                                                      const void *data, MPI_Count count,
                                                      MPI_Datatype datatype, MPIR_Request * sreq)
{
    return MPIDI_SHM_func->send_amv_reply(context_id, src_rank, handler_id, am_hdr, iov_len, data,
                                          count, datatype, sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_inject_amv_reply(MPIR_Context_id_t context_id,
                                                        int src_rank, int handler_id,
                                                        struct iovec *am_hdrs, size_t iov_len,
                                                        const void *data, MPI_Count count,
                                                        MPI_Datatype datatype)
{
    return MPIDI_SHM_func->inject_amv_reply(context_id, src_rank, handler_id, am_hdrs, iov_len,
                                            data, count, datatype);
};

MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHM_am_hdr_max_sz(void)
{
    return MPIDI_SHM_func->am_hdr_max_sz();
};

MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHM_am_inject_max_sz(void)
{
    return MPIDI_SHM_func->am_inject_max_sz();
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_recv(MPIR_Request * req)
{
    return MPIDI_SHM_func->am_recv();
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                     int *lpid_ptr, MPL_bool is_remote)
{
    return MPIDI_SHM_func->comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_gpid_get(MPIR_Comm * comm_ptr, int rank, MPIR_Gpid * gpid)
{
    return MPIDI_SHM_func->gpid_get(comm_ptr, rank, gpid);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_node_id(MPIR_Comm * comm, int rank,
                                                   MPID_Node_id_t * id_p)
{
    return MPIDI_SHM_func->get_node_id(comm, rank, id_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_max_node_id(MPIR_Comm * comm, MPID_Node_id_t * max_id_p)
{
    return MPIDI_SHM_func->get_max_node_id(comm, max_id_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_getallincomm(MPIR_Comm * comm_ptr, int local_size,
                                                    MPIR_Gpid local_gpid[], int *singleAVT)
{
    return MPIDI_SHM_func->getallincomm(comm_ptr, local_size, local_gpid, singleAVT);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_gpid_tolpidarray(int size, MPIR_Gpid gpid[], int lpid[])
{
    return MPIDI_SHM_func->gpid_tolpidarray(size, gpid, lpid);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_create_intercomm_from_lpids(MPIR_Comm * newcomm_ptr,
                                                                   int size, const int lpids[])
{
    return MPIDI_SHM_func->create_intercomm_from_lpids(newcomm_ptr, size, lpids);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_create_hook(MPIR_Comm * comm)
{
    return MPIDI_SHM_func->comm_create_hook(comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_free_hook(MPIR_Comm * comm)
{
    return MPIDI_SHM_func->comm_free_hook(comm);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_type_create_hook(MPIR_Datatype * type)
{
    return MPIDI_SHM_func->type_create_hook(type);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_type_free_hook(MPIR_Datatype * type)
{
    return MPIDI_SHM_func->type_free_hook(type);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_op_create_hook(MPIR_Op * op)
{
    return MPIDI_SHM_func->op_create_hook(op);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_op_free_hook(MPIR_Op * op)
{
    return MPIDI_SHM_func->op_free_hook(op);
};

MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_init(MPIR_Request * req)
{
    return MPIDI_SHM_func->am_request_init(req);
};

MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_finalize(MPIR_Request * req)
{
    return MPIDI_SHM_func->am_request_finalize(req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send(const void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->send(buf, count, datatype, rank, tag, comm, context_offset,
                                       request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ssend(const void *buf, int count,
                                             MPI_Datatype datatype, int rank, int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->ssend(buf, count, datatype, rank, tag, comm, context_offset,
                                        request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_startall(int count, MPIR_Request * requests[])
{
    return MPIDI_SHM_native_func->startall(count, requests);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_init(const void *buf, int count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->send_init(buf, count, datatype, rank, tag, comm, context_offset,
                                            request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ssend_init(const void *buf, int count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->ssend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                             request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rsend_init(const void *buf, int count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->rsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                             request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_bsend_init(const void *buf, int count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->bsend_init(buf, count, datatype, rank, tag, comm, context_offset,
                                             request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_isend(const void *buf, int count,
                                             MPI_Datatype datatype, int rank, int tag,
                                             MPIR_Comm * comm, int context_offset,
                                             MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->isend(buf, count, datatype, rank, tag, comm, context_offset,
                                        request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_issend(const void *buf, int count,
                                              MPI_Datatype datatype, int rank, int tag,
                                              MPIR_Comm * comm, int context_offset,
                                              MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->issend(buf, count, datatype, rank, tag, comm, context_offset,
                                         request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_cancel_send(MPIR_Request * sreq)
{
    return MPIDI_SHM_native_func->cancel_send(sreq);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_recv_init(void *buf, int count, MPI_Datatype datatype,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->recv_init(buf, count, datatype, rank, tag, comm, context_offset,
                                            request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_recv(void *buf, int count, MPI_Datatype datatype,
                                            int rank, int tag, MPIR_Comm * comm,
                                            int context_offset, MPI_Status * status,
                                            MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->recv(buf, count, datatype, rank, tag, comm, context_offset,
                                       status, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_irecv(void *buf, int count, MPI_Datatype datatype,
                                             int rank, int tag, MPIR_Comm * comm,
                                             int context_offset, MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                        request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_imrecv(void *buf, int count, MPI_Datatype datatype,
                                              MPIR_Request * message, MPIR_Request ** rreqp)
{
    return MPIDI_SHM_native_func->imrecv(buf, count, datatype, message, rreqp);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_cancel_recv(MPIR_Request * rreq)
{
    return MPIDI_SHM_native_func->cancel_recv(rreq);
};

MPL_STATIC_INLINE_PREFIX void *MPIDI_SHM_alloc_mem(size_t size, MPIR_Info * info_ptr)
{
    return MPIDI_SHM_native_func->alloc_mem(size, info_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_free_mem(void *ptr)
{
    return MPIDI_SHM_native_func->free_mem(ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_improbe(int source, int tag, MPIR_Comm * comm,
                                               int context_offset, int *flag,
                                               MPIR_Request ** message, MPI_Status * status)
{
    return MPIDI_SHM_native_func->improbe(source, tag, comm, context_offset, flag, message, status);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iprobe(int source, int tag, MPIR_Comm * comm,
                                              int context_offset, int *flag, MPI_Status * status)
{
    return MPIDI_SHM_native_func->iprobe(source, tag, comm, context_offset, flag, status);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_set_info(MPIR_Win * win, MPIR_Info * info)
{
    return MPIDI_SHM_native_func->win_set_info(win, info);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_shared_query(MPIR_Win * win, int rank,
                                                        MPI_Aint * size, int *disp_unit,
                                                        void *baseptr)
{
    return MPIDI_SHM_native_func->win_shared_query(win, rank, size, disp_unit, baseptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_put(const void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, int target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->put(origin_addr, origin_count, origin_datatype, target_rank,
                                      target_disp, target_count, target_datatype, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_start(group, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_complete(MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_complete(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_post(group, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_wait(MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_wait(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_test(MPIR_Win * win, int *flag)
{
    return MPIDI_SHM_native_func->win_test(win, flag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_lock(int lock_type, int rank, int assert, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_lock(lock_type, rank, assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_unlock(int rank, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_unlock(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_get_info(MPIR_Win * win, MPIR_Info ** info_p_p)
{
    return MPIDI_SHM_native_func->win_get_info(win, info_p_p);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get(void *origin_addr, int origin_count,
                                           MPI_Datatype origin_datatype, int target_rank,
                                           MPI_Aint target_disp, int target_count,
                                           MPI_Datatype target_datatype, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->get(origin_addr, origin_count, origin_datatype, target_rank,
                                      target_disp, target_count, target_datatype, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_free(MPIR_Win ** win_ptr)
{
    return MPIDI_SHM_native_func->win_free(win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_fence(int assert, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_fence(assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_create(void *base, MPI_Aint length, int disp_unit,
                                                  MPIR_Info * info, MPIR_Comm * comm_ptr,
                                                  MPIR_Win ** win_ptr)
{
    return MPIDI_SHM_native_func->win_create(base, length, disp_unit, info, comm_ptr, win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_accumulate(const void *origin_addr, int origin_count,
                                                  MPI_Datatype origin_datatype,
                                                  int target_rank, MPI_Aint target_disp,
                                                  int target_count,
                                                  MPI_Datatype target_datatype, MPI_Op op,
                                                  MPIR_Win * win)
{
    return MPIDI_SHM_native_func->accumulate(origin_addr, origin_count, origin_datatype,
                                             target_rank, target_disp, target_count,
                                             target_datatype, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_attach(MPIR_Win * win, void *base, MPI_Aint size)
{
    return MPIDI_SHM_native_func->win_attach(win, base, size);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_allocate_shared(MPI_Aint size, int disp_unit,
                                                           MPIR_Info * info_ptr,
                                                           MPIR_Comm * comm_ptr,
                                                           void **base_ptr, MPIR_Win ** win_ptr)
{
    return MPIDI_SHM_native_func->win_allocate_shared(size, disp_unit, info_ptr, comm_ptr, base_ptr,
                                                      win_ptr);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rput(const void *origin_addr, int origin_count,
                                            MPI_Datatype origin_datatype, int target_rank,
                                            MPI_Aint target_disp, int target_count,
                                            MPI_Datatype target_datatype, MPIR_Win * win,
                                            MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->rput(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_flush_local(int rank, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_flush_local(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_detach(MPIR_Win * win, const void *base)
{
    return MPIDI_SHM_native_func->win_detach(win, base);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_compare_and_swap(const void *origin_addr,
                                                        const void *compare_addr,
                                                        void *result_addr,
                                                        MPI_Datatype datatype,
                                                        int target_rank, MPI_Aint target_disp,
                                                        MPIR_Win * win)
{
    return MPIDI_SHM_native_func->compare_and_swap(origin_addr, compare_addr, result_addr, datatype,
                                                   target_rank, target_disp, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_raccumulate(const void *origin_addr, int origin_count,
                                                   MPI_Datatype origin_datatype,
                                                   int target_rank, MPI_Aint target_disp,
                                                   int target_count,
                                                   MPI_Datatype target_datatype, MPI_Op op,
                                                   MPIR_Win * win, MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->raccumulate(origin_addr, origin_count, origin_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rget_accumulate(const void *origin_addr,
                                                       int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       void *result_addr, int result_count,
                                                       MPI_Datatype result_datatype,
                                                       int target_rank, MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype,
                                                       MPI_Op op, MPIR_Win * win,
                                                       MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                  result_addr, result_count, result_datatype,
                                                  target_rank, target_disp, target_count,
                                                  target_datatype, op, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_fetch_and_op(const void *origin_addr,
                                                    void *result_addr, MPI_Datatype datatype,
                                                    int target_rank, MPI_Aint target_disp,
                                                    MPI_Op op, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                               target_disp, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_allocate(MPI_Aint size, int disp_unit,
                                                    MPIR_Info * info, MPIR_Comm * comm,
                                                    void *baseptr, MPIR_Win ** win)
{
    return MPIDI_SHM_native_func->win_allocate(size, disp_unit, info, comm, baseptr, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_flush(int rank, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_flush(rank, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_flush_local_all(MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_flush_local_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_unlock_all(MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_unlock_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_create_dynamic(MPIR_Info * info, MPIR_Comm * comm,
                                                          MPIR_Win ** win)
{
    return MPIDI_SHM_native_func->win_create_dynamic(info, comm, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rget(void *origin_addr, int origin_count,
                                            MPI_Datatype origin_datatype, int target_rank,
                                            MPI_Aint target_disp, int target_count,
                                            MPI_Datatype target_datatype, MPIR_Win * win,
                                            MPIR_Request ** request)
{
    return MPIDI_SHM_native_func->rget(origin_addr, origin_count, origin_datatype, target_rank,
                                       target_disp, target_count, target_datatype, win, request);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_sync(MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_sync(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_flush_all(MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_flush_all(win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_get_accumulate(const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      void *result_addr, int result_count,
                                                      MPI_Datatype result_datatype,
                                                      int target_rank, MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win)
{
    return MPIDI_SHM_native_func->get_accumulate(origin_addr, origin_count, origin_datatype,
                                                 result_addr, result_count, result_datatype,
                                                 target_rank, target_disp, target_count,
                                                 target_datatype, op, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_win_lock_all(int assert, MPIR_Win * win)
{
    return MPIDI_SHM_native_func->win_lock_all(assert, win);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->barrier(comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_bcast(void *buffer, int count, MPI_Datatype datatype,
                                             int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->bcast(buffer, count, datatype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_allreduce(const void *sendbuf, void *recvbuf,
                                                 int count, MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_allgather(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_allgatherv(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, MPIR_Comm * comm,
                                                  MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                             displs, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_scatter(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_scatterv(const void *sendbuf, const int *sendcounts,
                                                const int *displs, MPI_Datatype sendtype,
                                                void *recvbuf, int recvcount,
                                                MPI_Datatype recvtype, int root,
                                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                           recvcount, recvtype, root, comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_gather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              int recvcount, MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_gatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                          recvtype, root, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_alltoall(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_alltoallv(const void *sendbuf, const int *sendcounts,
                                                 const int *sdispls, MPI_Datatype sendtype,
                                                 void *recvbuf, const int *recvcounts,
                                                 const int *rdispls, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                            recvcounts, rdispls, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_alltoallw(const void *sendbuf, const int *sendcounts,
                                                 const int *sdispls,
                                                 const MPI_Datatype sendtypes[],
                                                 void *recvbuf, const int *recvcounts,
                                                 const int *rdispls,
                                                 const MPI_Datatype recvtypes[],
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                            recvcounts, rdispls, recvtypes, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_reduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, int root,
                                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                         errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                      const int *recvcounts,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                 comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_reduce_scatter_block(const void *sendbuf,
                                                            void *recvbuf, int recvcount,
                                                            MPI_Datatype datatype, MPI_Op op,
                                                            MPIR_Comm * comm_ptr,
                                                            MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op,
                                                       comm_ptr, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_scan(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op,
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_exscan(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_neighbor_allgather(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype,
                                                          void *recvbuf, int recvcount,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm * comm,
                                                          MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype,
                                                           void *recvbuf,
                                                           const int *recvcounts,
                                                           const int *displs,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm,
                                                           MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_neighbor_alltoallv(const void *sendbuf,
                                                          const int *sendcounts,
                                                          const int *sdispls,
                                                          MPI_Datatype sendtype,
                                                          void *recvbuf,
                                                          const int *recvcounts,
                                                          const int *rdispls,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm * comm,
                                                          MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                     recvbuf, recvcounts, rdispls, recvtype, comm,
                                                     errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_neighbor_alltoallw(const void *sendbuf,
                                                          const int *sendcounts,
                                                          const MPI_Aint * sdispls,
                                                          const MPI_Datatype * sendtypes,
                                                          void *recvbuf,
                                                          const int *recvcounts,
                                                          const MPI_Aint * rdispls,
                                                          const MPI_Datatype * recvtypes,
                                                          MPIR_Comm * comm,
                                                          MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                     recvbuf, recvcounts, rdispls, recvtypes, comm,
                                                     errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    return MPIDI_SHM_native_func->neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, comm, errflag);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype,
                                                           void *recvbuf, int recvcount,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ineighbor_allgatherv(const void *sendbuf,
                                                            int sendcount,
                                                            MPI_Datatype sendtype,
                                                            void *recvbuf,
                                                            const int *recvcounts,
                                                            const int *displs,
                                                            MPI_Datatype recvtype,
                                                            MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcounts, displs, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype,
                                                          void *recvbuf, int recvcount,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ineighbor_alltoallv(const void *sendbuf,
                                                           const int *sendcounts,
                                                           const int *sdispls,
                                                           MPI_Datatype sendtype,
                                                           void *recvbuf,
                                                           const int *recvcounts,
                                                           const int *rdispls,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                      recvbuf, recvcounts, rdispls, recvtype, comm,
                                                      req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ineighbor_alltoallw(const void *sendbuf,
                                                           const int *sendcounts,
                                                           const MPI_Aint * sdispls,
                                                           const MPI_Datatype * sendtypes,
                                                           void *recvbuf,
                                                           const int *recvcounts,
                                                           const MPI_Aint * rdispls,
                                                           const MPI_Datatype * recvtypes,
                                                           MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                      recvbuf, recvcounts, rdispls, recvtypes, comm,
                                                      req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ibarrier(comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                              int root, MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ibcast(buffer, count, datatype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iallgather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iallgatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts, const int *displs,
                                                   MPI_Datatype recvtype, MPIR_Comm * comm,
                                                   MPI_Request * req)
{
    return MPIDI_SHM_native_func->iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                              displs, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iallreduce(const void *sendbuf, void *recvbuf,
                                                  int count, MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ialltoall(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                  const int *sdispls, MPI_Datatype sendtype,
                                                  void *recvbuf, const int *recvcounts,
                                                  const int *rdispls, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                             recvcounts, rdispls, recvtype, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                  const int *sdispls,
                                                  const MPI_Datatype sendtypes[],
                                                  void *recvbuf, const int *recvcounts,
                                                  const int *rdispls,
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                             recvcounts, rdispls, recvtypes, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iexscan(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_igather(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_igatherv(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                const int *recvcounts, const int *displs,
                                                MPI_Datatype recvtype, int root,
                                                MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ireduce_scatter_block(const void *sendbuf,
                                                             void *recvbuf, int recvcount,
                                                             MPI_Datatype datatype, MPI_Op op,
                                                             MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op,
                                                        comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                       const int *recvcounts,
                                                       MPI_Datatype datatype, MPI_Op op,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                                  req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_ireduce(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op, int root,
                                               MPIR_Comm * comm_ptr, MPI_Request * req)
{
    return MPIDI_SHM_native_func->ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                          req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iscan(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->iscan(sendbuf, recvbuf, count, datatype, op, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iscatter(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                int root, MPIR_Comm * comm, MPI_Request * req)
{
    return MPIDI_SHM_native_func->iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm, req);
};

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_iscatterv(const void *sendbuf, const int *sendcounts,
                                                 const int *displs, MPI_Datatype sendtype,
                                                 void *recvbuf, int recvcount,
                                                 MPI_Datatype recvtype, int root,
                                                 MPIR_Comm * comm_ptr, MPI_Request * req)
{
    return MPIDI_SHM_native_func->iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, req);
};

#endif /* SHM_DISABLE_INLINES  */

#else

#define __shm_direct_stubshm__     0
#define __shm_direct_posix__    1

#if SHM_DIRECT==__shm_direct_stubshm__
#include "../stubshm/shm_direct.h"
#elif SHM_DIRECT==__shm_direct_posix__
#include "../posix/shm_direct.h"
#else
#error "No direct shm included"
#endif


#endif /* SHM_DIRECT           */

#endif
