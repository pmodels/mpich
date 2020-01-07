/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

/*
 * If inlining is turned off, this file will be used to call into the shared memory module. It will
 * use the function pointer structure to call the appropriate functions rather than directly
 * inlining them.
 */

#ifndef SHM_IMPL_H_INCLUDED
#define SHM_IMPL_H_INCLUDED

#include "posix_impl.h"

#ifndef SHM_INLINE
#ifndef SHM_DISABLE_INLINES

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_send_hdr(int rank, MPIR_Comm * comm, int handler_id,
                                                   const void *am_hdr, size_t am_hdr_sz)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.am_send_hdr(rank, comm, handler_id, am_hdr, am_hdr_sz);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isend(int rank, MPIR_Comm * comm, int handler_id,
                                                const void *am_hdr, size_t am_hdr_sz,
                                                const void *data, MPI_Count count,
                                                MPI_Datatype datatype, MPIR_Request * sreq)
{
    int ret;

    ret =
        MPIDI_SHM_src_funcs.am_isend(rank, comm, handler_id, am_hdr, am_hdr_sz, data, count,
                                     datatype, sreq);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isendv(int rank, MPIR_Comm * comm, int handler_id,
                                                 struct iovec *am_hdrs, size_t iov_len,
                                                 const void *data, MPI_Count count,
                                                 MPI_Datatype datatype, MPIR_Request * sreq)
{
    int ret;

    ret =
        MPIDI_SHM_src_funcs.am_isendv(rank, comm, handler_id, am_hdrs, iov_len, data, count,
                                      datatype, sreq);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_send_hdr_reply(MPIR_Context_id_t context_id,
                                                         int src_rank, int handler_id,
                                                         const void *am_hdr, size_t am_hdr_sz)
{
    int ret;

    ret =
        MPIDI_SHM_src_funcs.am_send_hdr_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_isend_reply(MPIR_Context_id_t context_id, int src_rank,
                                                      int handler_id, const void *am_hdr,
                                                      size_t am_hdr_sz, const void *data,
                                                      MPI_Count count, MPI_Datatype datatype,
                                                      MPIR_Request * sreq)
{
    int ret;

    ret =
        MPIDI_SHM_src_funcs.am_isend_reply(context_id, src_rank, handler_id, am_hdr, am_hdr_sz,
                                           data, count, datatype, sreq);

    return ret;
}

MPL_STATIC_INLINE_PREFIX size_t MPIDI_SHM_am_hdr_max_sz(void)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.am_hdr_max_sz();

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_am_recv(MPIR_Request * req)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.am_recv(req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_comm_get_lpid(MPIR_Comm * comm_ptr, int idx,
                                                     int *lpid_ptr, bool is_remote)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.comm_get_lpid(comm_ptr, idx, lpid_ptr, is_remote);

    return ret;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_init(MPIR_Request * req)
{
    MPIDI_SHM_src_funcs.am_request_init(req);

}

MPL_STATIC_INLINE_PREFIX void MPIDI_SHM_am_request_finalize(MPIR_Request * req)
{
    MPIDI_SHM_src_funcs.am_request_finalize(req);

}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_send(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_send(buf, count, datatype, rank, tag, comm, context_offset,
                                            addr, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_send_coll(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr,
                                                 MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.send_coll(buf, count, datatype, rank, tag, comm,
                                             context_offset, addr, request, errflag);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ssend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_ssend(buf, count, datatype, rank, tag, comm, context_offset,
                                             addr, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_isend(const void *buf, MPI_Aint count,
                                                 MPI_Datatype datatype, int rank, int tag,
                                                 MPIR_Comm * comm, int context_offset,
                                                 MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_isend(buf, count, datatype, rank, tag, comm, context_offset,
                                             addr, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_isend_coll(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr,
                                                  MPIR_Request ** request, MPIR_Errflag_t * errflag)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.isend_coll(buf, count, datatype, rank, tag, comm,
                                              context_offset, addr, request, errflag);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_issend(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_issend(buf, count, datatype, rank, tag, comm, context_offset,
                                              addr, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_send(MPIR_Request * sreq)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_cancel_send(sreq);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_recv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                int rank, int tag, MPIR_Comm * comm,
                                                int context_offset,
                                                MPI_Status * status, MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_recv(buf, count, datatype, rank, tag, comm, context_offset,
                                            status, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_irecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                 int rank, int tag, MPIR_Comm * comm,
                                                 int context_offset, MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_irecv(buf, count, datatype, rank, tag, comm, context_offset,
                                             request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_imrecv(void *buf, MPI_Aint count, MPI_Datatype datatype,
                                                  MPIR_Request * message)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_imrecv(buf, count, datatype, message);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_cancel_recv(MPIR_Request * rreq)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_cancel_recv(rreq);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_improbe(int source, int tag, MPIR_Comm * comm,
                                                   int context_offset,
                                                   int *flag, MPIR_Request ** message,
                                                   MPI_Status * status)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_improbe(source, tag, comm, context_offset, flag, message,
                                                 status);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iprobe(int source, int tag, MPIR_Comm * comm,
                                                  int context_offset,
                                                  int *flag, MPI_Status * status)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_iprobe(source, tag, comm, context_offset, flag, status);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_cmpl_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.rma_win_cmpl_hook(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_win_local_cmpl_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.rma_win_local_cmpl_hook(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.rma_target_cmpl_hook(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_target_local_cmpl_hook(int rank, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.rma_target_local_cmpl_hook(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_enter_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.rma_op_cs_enter_hook(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_rma_op_cs_exit_hook(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_src_funcs.rma_op_cs_exit_hook(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_shared_query(MPIR_Win * win, int rank,
                                                            MPI_Aint * size, int *disp_unit,
                                                            void *baseptr)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_shared_query(win, rank, size, disp_unit, baseptr);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_put(const void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_put(origin_addr, origin_count, origin_datatype, target_rank,
                                           target_disp, target_count, target_datatype, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_start(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_start(group, assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_complete(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_complete(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_post(MPIR_Group * group, int assert, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_post(group, assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_wait(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_wait(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_test(MPIR_Win * win, int *flag)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_test(win, flag);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock(int lock_type, int rank, int assert,
                                                    MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_lock(lock_type, rank, assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock(int rank, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_unlock(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get(void *origin_addr, int origin_count,
                                               MPI_Datatype origin_datatype, int target_rank,
                                               MPI_Aint target_disp, int target_count,
                                               MPI_Datatype target_datatype, MPIR_Win * win)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_get(origin_addr, origin_count, origin_datatype, target_rank,
                                           target_disp, target_count, target_datatype, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_fence(int assert, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_fence(assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_accumulate(const void *origin_addr, int origin_count,
                                                      MPI_Datatype origin_datatype, int target_rank,
                                                      MPI_Aint target_disp, int target_count,
                                                      MPI_Datatype target_datatype, MPI_Op op,
                                                      MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_accumulate(origin_addr, origin_count, origin_datatype,
                                                    target_rank, target_disp, target_count,
                                                    target_datatype, op, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rput(const void *origin_addr, int origin_count,
                                                MPI_Datatype origin_datatype, int target_rank,
                                                MPI_Aint target_disp, int target_count,
                                                MPI_Datatype target_datatype, MPIR_Win * win,
                                                MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_rput(origin_addr, origin_count, origin_datatype, target_rank,
                                            target_disp, target_count, target_datatype, win,
                                            request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local(int rank, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_flush_local(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_compare_and_swap(const void *origin_addr,
                                                            const void *compare_addr,
                                                            void *result_addr,
                                                            MPI_Datatype datatype, int target_rank,
                                                            MPI_Aint target_disp, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_compare_and_swap(origin_addr, compare_addr, result_addr,
                                                          datatype, target_rank, target_disp, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_raccumulate(const void *origin_addr, int origin_count,
                                                       MPI_Datatype origin_datatype,
                                                       int target_rank, MPI_Aint target_disp,
                                                       int target_count,
                                                       MPI_Datatype target_datatype, MPI_Op op,
                                                       MPIR_Win * win, MPIR_Request ** request)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_raccumulate(origin_addr, origin_count, origin_datatype,
                                                     target_rank, target_disp, target_count,
                                                     target_datatype, op, win, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget_accumulate(const void *origin_addr,
                                                           int origin_count,
                                                           MPI_Datatype origin_datatype,
                                                           void *result_addr, int result_count,
                                                           MPI_Datatype result_datatype,
                                                           int target_rank, MPI_Aint target_disp,
                                                           int target_count,
                                                           MPI_Datatype target_datatype, MPI_Op op,
                                                           MPIR_Win * win, MPIR_Request ** request)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_rget_accumulate(origin_addr, origin_count, origin_datatype,
                                                         result_addr, result_count, result_datatype,
                                                         target_rank, target_disp, target_count,
                                                         target_datatype, op, win, request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_fetch_and_op(const void *origin_addr, void *result_addr,
                                                        MPI_Datatype datatype, int target_rank,
                                                        MPI_Aint target_disp, MPI_Op op,
                                                        MPIR_Win * win)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_fetch_and_op(origin_addr, result_addr, datatype, target_rank,
                                                    target_disp, op, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush(int rank, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_flush(rank, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_local_all(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_flush_local_all(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_unlock_all(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_unlock_all(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_rget(void *origin_addr, int origin_count,
                                                MPI_Datatype origin_datatype, int target_rank,
                                                MPI_Aint target_disp, int target_count,
                                                MPI_Datatype target_datatype, MPIR_Win * win,
                                                MPIR_Request ** request)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_rget(origin_addr, origin_count, origin_datatype, target_rank,
                                            target_disp, target_count, target_datatype, win,
                                            request);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_sync(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_sync(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_flush_all(MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_flush_all(win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_get_accumulate(const void *origin_addr, int origin_count,
                                                          MPI_Datatype origin_datatype,
                                                          void *result_addr, int result_count,
                                                          MPI_Datatype result_datatype,
                                                          int target_rank, MPI_Aint target_disp,
                                                          int target_count,
                                                          MPI_Datatype target_datatype, MPI_Op op,
                                                          MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_get_accumulate(origin_addr, origin_count, origin_datatype,
                                                        result_addr, result_count, result_datatype,
                                                        target_rank, target_disp, target_count,
                                                        target_datatype, op, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_win_lock_all(int assert, MPIR_Win * win)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_win_lock_all(assert, win);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_barrier(comm, errflag, algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                                 int root, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag,
                                                 const void *algo_parameters_container)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_bcast(buffer, count, datatype, root, comm, errflag,
                                             algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm,
                                                 errflag, algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, comm, errflag,
                                                   algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPIR_Errflag_t * errflag,
                                                      const void *algo_parameters_container)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                  displs, recvtype, comm, errflag,
                                                  algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scatter(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, root, comm, errflag,
                                                 algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                                    const int *displs, MPI_Datatype sendtype,
                                                    void *recvbuf, int recvcount,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                                  recvcount, recvtype, root, comm_ptr, errflag,
                                                  algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_gather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype, int root,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, root, comm, errflag,
                                                algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_gatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int *recvcounts, const int *displs,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                 displs, recvtype, root, comm, errflag,
                                                 algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, comm, errflag,
                                                  algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls, MPI_Datatype sendtype,
                                                     void *recvbuf, const int *recvcounts,
                                                     const int *rdispls, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                   recvcounts, rdispls, recvtype, comm, errflag,
                                                   algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoallw(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls,
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const int *recvcounts, const int *rdispls,
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                   recvcounts, rdispls, recvtypes, comm, errflag,
                                                   algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op, int root,
                                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                              errflag, algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                          const int *recvcounts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag,
                                                          const void *algo_parameters_container)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                        comm_ptr, errflag,
                                                        algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag, const void
                                                                *algo_parameters_container)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                                            op, comm_ptr, errflag,
                                                            algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag,
                                                const void *algo_parameters_container)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                            algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                  const void *algo_parameters_container)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                              algo_parameters_container);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              int recvcount, MPI_Datatype recvtype,
                                                              MPIR_Comm * comm)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *displs,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcounts, displs, recvtype, comm);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const int *sdispls,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const int *recvcounts,
                                                              const int *rdispls,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                            recvbuf, recvcounts, rdispls, recvtype,
                                                            comm);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                              const int *sendcounts,
                                                              const MPI_Aint * sdispls,
                                                              const MPI_Datatype * sendtypes,
                                                              void *recvbuf, const int *recvcounts,
                                                              const MPI_Aint * rdispls,
                                                              const MPI_Datatype * recvtypes,
                                                              MPIR_Comm * comm)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                            recvbuf, recvcounts, rdispls, recvtypes,
                                                            comm);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             int recvcount, MPI_Datatype recvtype,
                                                             MPIR_Comm * comm)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, comm);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               int recvcount, MPI_Datatype recvtype,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                                             recvcount, recvtype, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                                MPI_Datatype sendtype,
                                                                void *recvbuf,
                                                                const int *recvcounts,
                                                                const int *displs,
                                                                MPI_Datatype recvtype,
                                                                MPIR_Comm * comm,
                                                                MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                              recvcounts, displs, recvtype, comm,
                                                              req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              int recvcount, MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                               const int *sendcounts,
                                                               const int *sdispls,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *rdispls,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                             recvbuf, recvcounts, rdispls, recvtype,
                                                             comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                               const int *sendcounts,
                                                               const MPI_Aint * sdispls,
                                                               const MPI_Datatype * sendtypes,
                                                               void *recvbuf, const int *recvcounts,
                                                               const MPI_Aint * rdispls,
                                                               const MPI_Datatype * recvtypes,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request ** req)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                           recvbuf, recvcounts, rdispls, recvtypes,
                                                           comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ibarrier(comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                                  int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ibcast(buffer, count, datatype, root, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request ** req)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                                   recvcounts, displs, recvtype, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                    recvcounts, rdispls, recvtype, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls,
                                                      const MPI_Datatype sendtypes[], void *recvbuf,
                                                      const int *recvcounts, const int *rdispls,
                                                      const MPI_Datatype recvtypes[],
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                  recvcounts, rdispls, recvtypes, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_igather(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                 recvtype, root, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_igatherv(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int *recvcounts, const int *displs,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                  displs, recvtype, root, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                 int recvcount,
                                                                 MPI_Datatype datatype, MPI_Op op,
                                                                 MPIR_Comm * comm,
                                                                 MPIR_Request ** req)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                                             op, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                           const int *recvcounts,
                                                           MPI_Datatype datatype, MPI_Op op,
                                                           MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                                         comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int ret;

    ret =
        MPIDI_SHM_native_src_funcs.mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root,
                                               comm_ptr, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscatter(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                  recvtype, root, comm, req);

    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                                     const int *displs, MPI_Datatype sendtype,
                                                     void *recvbuf, int recvcount,
                                                     MPI_Datatype recvtype, int root,
                                                     MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int ret;

    ret = MPIDI_SHM_native_src_funcs.mpi_iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                                   recvcount, recvtype, root, comm_ptr, req);

    return ret;
}

#endif /* SHM_DISABLE_INLINES  */

#else

#include "../src/shm_inline.h"

#endif /* SHM_INLINE           */

#endif /* SHM_IMPL_H_INCLUDED */
