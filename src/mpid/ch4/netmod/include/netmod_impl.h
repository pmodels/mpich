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


#ifndef NETMOD_INLINE
#ifndef NETMOD_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                  const void *am_hdr, size_t am_hdr_sz)
{
    int ret;




    ret = MPIDI_NM_func->am_send_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                               const void *am_hdr, size_t am_hdr_sz,
                                               const void *data, MPI_Count count,
                                               MPI_Datatype datatype, MPIR_Request * sreq)
{
    int ret;




    ret = MPIDI_NM_func->am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count, datatype,
                                  sreq);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                                struct iovec *am_hdrs, size_t iov_len,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype, MPIR_Request * sreq)
{
    int ret;




    ret = MPIDI_NM_func->am_isendv(rank, comm, handler_id, am_hdrs, iov_len, data, count, datatype,
                                   sreq);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                        int src_rank, int handler_id,
                                                        const void *am_hdr, size_t am_hdr_sz)
{
    int ret;




    ret = MPIDI_NM_func->am_send_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                     int handler_id, const void *am_hdr,
                                                     size_t am_hdr_sz, const void *data,
                                                     MPI_Count count, MPI_Datatype datatype,
                                                     MPIR_Request * sreq)
{
    int ret;




    ret = MPIDI_NM_func->am_isend_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz, data,
                                        count, datatype, sreq);


    return ret;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_NM_am_hdr_max_sz(void)
{
    int ret;




    ret = MPIDI_NM_func->am_hdr_max_sz();


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_am_recv(MPIR_Request * req)
{
    int ret;




    ret = MPIDI_NM_func->am_recv(req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                    int *lpid_ptr, bool is_remote)
{
    int ret;




    ret = MPIDI_NM_func->comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_func->rma_win_cmpl_hook(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_func->rma_win_local_cmpl_hook(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_func->rma_target_cmpl_hook(rank, win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_func->rma_target_local_cmpl_hook(rank, win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_init(MPIR_Request * req)
{



    MPIDI_NM_func->am_request_init(req);


}

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_am_request_finalize(MPIR_Request * req)
{



    MPIDI_NM_func->am_request_finalize(req);


}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_send(const void *buf, MPI_Aint count,
                                               MPI_Datatype datatype, int rank, int tag,
                                               MPIR_Comm * comm, int context_offset,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_send(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                       request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_send_coll(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr,
                                                MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int ret;



    ret =
        MPIDI_NM_native_func->send_coll(buf, count, datatype, rank, tag, comm, context_offset,
                                        addr, request, errflag);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ssend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                        request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_isend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_isend(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                        request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_isend_coll(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr,
                                                 MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int ret;




    ret =
        MPIDI_NM_native_func->isend_coll(buf, count, datatype, rank, tag, comm, context_offset,
                                         addr, request, errflag);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_issend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_issend(buf, count, datatype, rank, tag, comm, context_offset,
                                         addr, request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_send(MPIR_Request * sreq)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_cancel_send(sreq);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                               int rank, int tag, MPIR_Comm * comm,
                                               int context_offset, MPIDI_av_entry_t * addr,
                                               MPI_Status * status, MPIR_Request ** request)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_recv(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                       status, request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset, MPIDI_av_entry_t * addr,
                                                MPIR_Request ** request)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset, addr,
                                        request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_imrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                 MPIR_Request * message)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_imrecv(buf, count, datatype, message);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_cancel_recv(MPIR_Request * rreq)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_cancel_recv(rreq);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                  int context_offset, MPIDI_av_entry_t * addr,
                                                  int *flag, MPIR_Request ** message,
                                                  MPI_Status * status)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_improbe(source, tag, comm, context_offset, addr, flag, message,
                                            status);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIDI_av_entry_t * addr,
                                                 int *flag, MPI_Status * status)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iprobe(source, tag, comm, context_offset, addr, flag, status);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_shared_query(MPIR_Win * win, int rank,
                                                           MPI_Aint * size, int *disp_unit,
                                                           void *baseptr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_shared_query(win, rank, size, disp_unit, baseptr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_put(const void *origin_addr, int origin_count,
                                              MPI_Datatype origin_datatype, int target_rank,
                                              MPI_Aint target_disp, int target_count,
                                              MPI_Datatype target_datatype, MPIR_Win * win,
                                              MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_start(group, assert, win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_complete(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_complete(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_post(group, assert, win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_wait(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_wait(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_test(win, flag);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock(int lock_type, int rank, int assert,
                                                   MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_lock(lock_type, rank, assert, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock(int rank, MPIR_Win * win,
                                                     MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_unlock(rank, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get(void *origin_addr, int origin_count,
                                              MPI_Datatype origin_datatype, int target_rank,
                                              MPI_Aint target_disp, int target_count,
                                              MPI_Datatype target_datatype, MPIR_Win * win,
                                              MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                        target_disp, target_count, target_datatype, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_fence(int assert, MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_fence(assert, win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_accumulate(const void *origin_addr, int origin_count,
                                                     MPI_Datatype origin_datatype, int target_rank,
                                                     MPI_Aint target_disp, int target_count,
                                                     MPI_Datatype target_datatype, MPI_Op op,
                                                     MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                               target_rank, target_disp, target_count,
                                               target_datatype, op, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rput(const void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, win, addr,
                                         request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local(int rank, MPIR_Win * win,
                                                          MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_flush_local(rank, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_compare_and_swap(const void *origin_addr,
                                                           const void *compare_addr,
                                                           void *result_addr,
                                                           MPI_Datatype datatype, int target_rank,
                                                           MPI_Aint target_disp, MPIR_Win * win,
                                                           MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                     datatype, target_rank, target_disp, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_raccumulate(const void *origin_addr, int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      int target_rank, MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win, MPIDI_av_entry_t * addr,
                                                      MPIR_Request ** request)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                                target_rank, target_disp, target_count,
                                                target_datatype, op, win, addr, request);


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
                                                          MPIR_Win * win, MPIDI_av_entry_t * addr,
                                                          MPIR_Request ** request)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                    result_addr, result_count, result_datatype,
                                                    target_rank, target_disp, target_count,
                                                    target_datatype, op, win, addr, request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                                       MPI_Datatype datatype, int target_rank,
                                                       MPI_Aint target_disp, MPI_Op op,
                                                       MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                                 target_disp, op, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush(int rank, MPIR_Win * win,
                                                    MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_flush(rank, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_flush_local_all(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_unlock_all(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_unlock_all(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_rget(void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win,
                                               MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                                         target_disp, target_count, target_datatype, win, addr,
                                         request);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_sync(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_sync(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_flush_all(MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_flush_all(win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_get_accumulate(const void *origin_addr, int origin_count,
                                                         MPI_Datatype origin_datatype,
                                                         void *result_addr, int result_count,
                                                         MPI_Datatype result_datatype,
                                                         int target_rank, MPI_Aint target_disp,
                                                         int target_count,
                                                         MPI_Datatype target_datatype, MPI_Op op,
                                                         MPIR_Win * win, MPIDI_av_entry_t * addr)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                   result_addr, result_count, result_datatype,
                                                   target_rank, target_disp, target_count,
                                                   target_datatype, op, win, addr);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_win_lock_all(assert, win);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_rank_is_local(int target, MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_native_func->rank_is_local(target, comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_av_is_local(MPIDI_av_entry_t * av)
{
    int ret;




    ret = MPIDI_NM_native_func->av_is_local(av);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_barrier(comm, errflag, algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                                int root, MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag,
                                                const void *algo_parameters_container)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_bcast(buffer, count, datatype, root, comm, errflag,
                                        algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                                    MPI_Datatype datatype, MPI_Op op,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                            algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgather(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm, errflag, algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const int *recvcounts, const int *displs,
                                                     MPI_Datatype recvtype, MPIR_Comm * comm,
                                                     MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                               displs, recvtype, comm, errflag,
                                               algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatter(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, root, comm, errflag,
                                            algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                                   const int *displs, MPI_Datatype sendtype,
                                                   void *recvbuf, int recvcount,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                             recvcount, recvtype, root, comm_ptr, errflag,
                                             algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gather(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype, int root,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                 const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm, errflag,
                                           algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gatherv(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, root, comm, errflag,
                                            algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoall(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, comm, errflag, algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                                    const int *sdispls, MPI_Datatype sendtype,
                                                    void *recvbuf, const int *recvcounts,
                                                    const int *rdispls, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                              recvcounts, rdispls, recvtype, comm, errflag,
                                              algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallw(const void *sendbuf, const int *sendcounts,
                                                    const int *sdispls,
                                                    const MPI_Datatype sendtypes[], void *recvbuf,
                                                    const int *recvcounts, const int *rdispls,
                                                    const MPI_Datatype recvtypes[],
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                              recvcounts, rdispls, recvtypes, comm, errflag,
                                              algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                 const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                           errflag, algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                         const int *recvcounts,
                                                         MPI_Datatype datatype, MPI_Op op,
                                                         MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag,
                                                         const void *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                   comm_ptr, errflag, algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                               int recvcount,
                                                               MPI_Datatype datatype, MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag, const void
                                                               *algo_parameters_container)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op,
                                                         comm_ptr, errflag,
                                                         algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                               MPIR_Errflag_t * errflag,
                                               const void *algo_parameters_container)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                       algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                 const void *algo_parameters_container)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                         algo_parameters_container);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm);


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




    ret = MPIDI_NM_native_func->mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcounts, displs, recvtype, comm);


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




    ret = MPIDI_NM_native_func->mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                       recvbuf, recvcounts, rdispls, recvtype,
                                                       comm);


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




    ret = MPIDI_NM_native_func->mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                       recvbuf, recvcounts, rdispls, recvtypes,
                                                       comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                            MPI_Datatype sendtype, void *recvbuf,
                                                            int recvcount, MPI_Datatype recvtype,
                                                            MPIR_Comm * comm)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, comm);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              int recvcount, MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                               MPI_Datatype sendtype,
                                                               void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *displs,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcounts, displs, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                       recvcount, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const int *sdispls,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const int *recvcounts,
                                                              const int *rdispls,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
                                                        comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const MPI_Aint * sdispls,
                                                              const MPI_Datatype * sendtypes,
                                                              void *recvbuf, const int *recvcounts,
                                                              const MPI_Aint * rdispls,
                                                              const MPI_Datatype * recvtypes,
                                                              MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                        recvbuf, recvcounts, rdispls, recvtypes,
                                                        comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ibarrier(comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                                 int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ibcast(buffer, count, datatype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                displs, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls, MPI_Datatype sendtype,
                                                     void *recvbuf, const int *recvcounts,
                                                     const int *rdispls, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                               recvcounts, rdispls, recvtype, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls,
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const int *recvcounts, const int *rdispls,
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                               recvcounts, rdispls, recvtypes, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts, const int *displs,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                             displs, recvtype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm,
                                                                MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                                          op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                          const int *recvcounts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                    comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op, int root,
                                                  MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root,
                                          comm_ptr, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatter(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, root, comm, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                                    const int *displs, MPI_Datatype sendtype,
                                                    void *recvbuf, int recvcount,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                              recvcount, recvtype, root, comm_ptr, req);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibarrier_sched(MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ibarrier_sched(comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibcast_sched(void *buffer, int count,
                                                       MPI_Datatype datatype, int root,
                                                       MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ibcast_sched(buffer, count, datatype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgather_sched(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype, void *recvbuf,
                                                           int recvcount, MPI_Datatype recvtype,
                                                           MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iallgather_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgatherv_sched(const void *sendbuf, int sendcount,
                                                            MPI_Datatype sendtype, void *recvbuf,
                                                            const int *recvcounts,
                                                            const int *displs,
                                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                                            MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iallgatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallreduce_sched(const void *sendbuf, void *recvbuf,
                                                           int count, MPI_Datatype datatype,
                                                           MPI_Op op, MPIR_Comm * comm,
                                                           MPIR_Sched_t s)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_iallreduce_sched(sendbuf, recvbuf, count, datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoall_sched(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          int recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ialltoall_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallv_sched(const void *sendbuf,
                                                           const int sendcounts[],
                                                           const int sdispls[],
                                                           MPI_Datatype sendtype, void *recvbuf,
                                                           const int recvcounts[],
                                                           const int rdispls[],
                                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                                           MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ialltoallv_sched(sendbuf, sendcounts, sdispls, sendtype,
                                                     recvbuf, recvcounts, rdispls, recvtype, comm,
                                                     s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallw_sched(const void *sendbuf,
                                                           const int sendcounts[],
                                                           const int sdispls[],
                                                           const MPI_Datatype sendtypes[],
                                                           void *recvbuf, const int recvcounts[],
                                                           const int rdispls[],
                                                           const MPI_Datatype recvtypes[],
                                                           MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ialltoallw_sched(sendbuf, sendcounts, sdispls, sendtypes,
                                                     recvbuf, recvcounts, rdispls, recvtypes, comm,
                                                     s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iexscan_sched(const void *sendbuf, void *recvbuf,
                                                        int count, MPI_Datatype datatype, MPI_Op op,
                                                        MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iexscan_sched(sendbuf, recvbuf, count, datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igather_sched(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_igather_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igatherv_sched(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         const int *recvcounts, const int *displs,
                                                         MPI_Datatype recvtype, int root,
                                                         MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_igatherv_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                 displs, recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter_block_sched(const void *sendbuf,
                                                                      void *recvbuf, int recvcount,
                                                                      MPI_Datatype datatype,
                                                                      MPI_Op op, MPIR_Comm * comm,
                                                                      MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ireduce_scatter_block_sched(sendbuf, recvbuf, recvcount,
                                                                datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter_sched(const void *sendbuf, void *recvbuf,
                                                                const int recvcounts[],
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ireduce_scatter_sched(sendbuf, recvbuf, recvcounts, datatype,
                                                          op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_sched(const void *sendbuf, void *recvbuf,
                                                        int count, MPI_Datatype datatype, MPI_Op op,
                                                        int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret =
        MPIDI_NM_native_func->mpi_ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm,
                                                s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscan_sched(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iscan_sched(sendbuf, recvbuf, count, datatype, op, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatter_sched(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iscatter_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatterv_sched(const void *sendbuf,
                                                          const int *sendcounts, const int *displs,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          int recvcount, MPI_Datatype recvtype,
                                                          int root, MPIR_Comm * comm,
                                                          MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_iscatterv_sched(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                                    recvcount, recvtype, root, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgather_sched(const void *sendbuf,
                                                                    int sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf, int recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm,
                                                                    MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_allgather_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgatherv_sched(const void *sendbuf,
                                                                     int sendcount,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     const int recvcounts[],
                                                                     const int displs[],
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_allgatherv_sched(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcounts, displs,
                                                               recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoall_sched(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf, int recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm, MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoall_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcount, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallv_sched(const void *sendbuf,
                                                                    const int sendcounts[],
                                                                    const int sdispls[],
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    const int recvcounts[],
                                                                    const int rdispls[],
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm,
                                                                    MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoallv_sched(sendbuf, sendcounts, sdispls,
                                                              sendtype, recvbuf, recvcounts,
                                                              rdispls, recvtype, comm, s);


    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallw_sched(const void *sendbuf,
                                                                    const int sendcounts[],
                                                                    const MPI_Aint sdispls[],
                                                                    const MPI_Datatype sendtypes[],
                                                                    void *recvbuf,
                                                                    const int recvcounts[],
                                                                    const MPI_Aint rdispls[],
                                                                    const MPI_Datatype recvtypes[],
                                                                    MPIR_Comm * comm,
                                                                    MPIR_Sched_t s)
{
    int ret;




    ret = MPIDI_NM_native_func->mpi_ineighbor_alltoallw_sched(sendbuf, sendcounts, sdispls,
                                                              sendtypes, recvbuf, recvcounts,
                                                              rdispls, recvtypes, comm, s);


    return ret;
}

#endif /* NETMOD_DISABLE_INLINES  */

#else
#define __netmod_inline_stubnm__   0
#define __netmod_inline_ofi__    1
#define __netmod_inline_ucx__    2

#if NETMOD_INLINE==__netmod_inline_stubnm__
#include "../stubnm/netmod_inline.h"
#elif NETMOD_INLINE==__netmod_inline_ofi__
#include "../ofi/netmod_inline.h"
#elif NETMOD_INLINE==__netmod_inline_ucx__
#include "../ucx/netmod_inline.h"
#else
#error "No direct netmod included"
#endif
#endif /* NETMOD_INLINE           */

#endif /* NETMOD_IMPL_H_INCLUDED */
