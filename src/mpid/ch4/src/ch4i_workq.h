/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef CH4I_WORKQ_H_INCLUDED
#define CH4I_WORKQ_H_INCLUDED

#include "ch4i_workq_types.h"
#include <utlist.h>

#if defined(MPIDI_CH4_USE_WORK_QUEUES)

/* For profiling */
extern double MPIDI_pt2pt_enqueue_time;
extern double MPIDI_pt2pt_progress_time;
extern double MPIDI_pt2pt_issue_pend_time;
extern double MPIDI_rma_enqueue_time;
extern double MPIDI_rma_progress_time;
extern double MPIDI_rma_issue_pend_time;

#if defined(MPIDI_WORKQ_PROFILE)
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START  double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP                          \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_pt2pt_enqueue_time += (enqueue_t2 - enqueue_t1);  \
    } while (0)
#define MPIDI_WORKQ_PROGRESS_START double progress_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PROGRESS_STOP                                       \
    do {                                                                \
        double progress_t2 = MPI_Wtime();                               \
        MPIDI_pt2pt_progress_time += (progress_t2 - progress_t1);       \
    } while (0)
#define MPIDI_WORKQ_ISSUE_START    double issue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_ISSUE_STOP                                  \
    do {                                                        \
        double issue_t2 = MPI_Wtime();                          \
        MPIDI_pt2pt_issue_pend_time += (issue_t2 - issue_t1);   \
    } while (0)

#define MPIDI_WORKQ_RMA_ENQUEUE_START    double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP                            \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_rma_enqueue_time += (enqueue_t2 - enqueue_t1);    \
    } while (0)
#else /*!defined(MPIDI_WORKQ_PROFILE) */
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP
#define MPIDI_WORKQ_PROGRESS_START
#define MPIDI_WORKQ_PROGRESS_STOP
#define MPIDI_WORKQ_ISSUE_START
#define MPIDI_WORKQ_ISSUE_STOP

#define MPIDI_WORKQ_RMA_ENQUEUE_START
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP
#endif /* defined(MPIDI_WORKQ_PROFILE) */

struct MPIDI_workq_elemt MPIDI_workq_elemt_direct[MPIDI_WORKQ_ELEMT_PREALLOC];
extern MPIR_Object_alloc_t MPIDI_workq_elemt_mem;

MPL_STATIC_INLINE_PREFIX struct MPIDI_workq_elemt *MPIDI_workq_elemt_create(void)
{
    return MPIR_Handle_obj_alloc(&MPIDI_workq_elemt_mem);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_elemt_free(struct MPIDI_workq_elemt *elemt)
{
    MPIR_Handle_obj_free(&MPIDI_workq_elemt_mem, elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_pt2pt_enqueue_body(MPIDI_workq_t * workq,
                                                             MPIDI_workq_op_t op,
                                                             const void *send_buf,
                                                             void *recv_buf,
                                                             MPI_Aint count,
                                                             MPI_Datatype datatype,
                                                             int rank,
                                                             int tag,
                                                             MPIR_Comm * comm_ptr,
                                                             int context_offset,
                                                             MPIDI_av_entry_t * addr,
                                                             MPI_Status * status,
                                                             MPIR_Request * request,
                                                             int *flag,
                                                             MPIR_Request ** message,
                                                             OPA_int_t * processed)
{
    MPIDI_workq_elemt_t *pt2pt_elemt;

    MPIR_Assert(request != NULL);

    MPIR_Request_add_ref(request);
    pt2pt_elemt = &request->dev.ch4.command;
    pt2pt_elemt->op = op;
    pt2pt_elemt->processed = processed;
    pt2pt_elemt->pt2pt.send_buf = send_buf;
    pt2pt_elemt->pt2pt.recv_buf = recv_buf;
    pt2pt_elemt->pt2pt.count = count;
    pt2pt_elemt->pt2pt.datatype = datatype;
    pt2pt_elemt->pt2pt.rank = rank;
    pt2pt_elemt->pt2pt.tag = tag;
    pt2pt_elemt->pt2pt.comm_ptr = comm_ptr;
    pt2pt_elemt->pt2pt.context_offset = context_offset;
    pt2pt_elemt->pt2pt.addr = addr;
    pt2pt_elemt->pt2pt.status = status;
    pt2pt_elemt->pt2pt.request = request;
    pt2pt_elemt->pt2pt.flag = flag;
    pt2pt_elemt->pt2pt.message = message;

    MPIDI_workq_enqueue(workq, pt2pt_elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_rma_enqueue_body(MPIDI_workq_t * workq,
                                                           MPIDI_workq_op_t op,
                                                           const void *origin_addr,
                                                           int origin_count,
                                                           MPI_Datatype origin_datatype,
                                                           void *result_addr,
                                                           int result_count,
                                                           MPI_Datatype result_datatype,
                                                           int target_rank,
                                                           MPI_Aint target_disp,
                                                           int target_count,
                                                           MPI_Datatype target_datatype,
                                                           MPI_Op acc_op,
                                                           MPIR_Group * group,
                                                           int lock_type,
                                                           int assert,
                                                           MPIR_Win * win_ptr,
                                                           MPIDI_av_entry_t * addr,
                                                           OPA_int_t * processed)
{
    MPIDI_workq_elemt_t *rma_elemt = NULL;
    rma_elemt = MPIDI_workq_elemt_create();
    rma_elemt->op = op;
    rma_elemt->processed = processed;
    rma_elemt->rma.origin_addr = origin_addr;
    rma_elemt->rma.origin_count = origin_count;
    rma_elemt->rma.origin_datatype = origin_datatype;
    rma_elemt->rma.result_addr = result_addr;
    rma_elemt->rma.result_count = result_count;
    rma_elemt->rma.result_datatype = result_datatype;
    rma_elemt->rma.target_rank = target_rank;
    rma_elemt->rma.target_disp = target_disp;
    rma_elemt->rma.target_count = target_count;
    rma_elemt->rma.target_datatype = target_datatype;
    rma_elemt->rma.acc_op = acc_op;
    rma_elemt->rma.group = group;
    rma_elemt->rma.lock_type = lock_type;
    rma_elemt->rma.assert = assert;
    rma_elemt->rma.win_ptr = win_ptr;
    rma_elemt->rma.addr = addr;

    MPIDI_workq_enqueue(workq, rma_elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_release_pt2pt_elemt(MPIDI_workq_elemt_t * workq_elemt)
{
    MPIR_Request *req;
    req = MPL_container_of(workq_elemt, MPIR_Request, dev.ch4.command);
    MPIR_Request_free(req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_dispatch(MPIDI_workq_elemt_t * workq_elemt)
{
    int mpi_errno = MPI_SUCCESS;

    switch (workq_elemt->op) {
        case SEND:
            MPIDI_NM_mpi_send(workq_elemt->pt2pt.send_buf, workq_elemt->pt2pt.count,
                              workq_elemt->pt2pt.datatype, workq_elemt->pt2pt.rank,
                              workq_elemt->pt2pt.tag, workq_elemt->pt2pt.comm_ptr,
                              workq_elemt->pt2pt.context_offset, workq_elemt->pt2pt.addr,
                              &workq_elemt->pt2pt.request);
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case ISEND:
            MPIDI_NM_mpi_isend(workq_elemt->pt2pt.send_buf, workq_elemt->pt2pt.count,
                               workq_elemt->pt2pt.datatype, workq_elemt->pt2pt.rank,
                               workq_elemt->pt2pt.tag, workq_elemt->pt2pt.comm_ptr,
                               workq_elemt->pt2pt.context_offset, workq_elemt->pt2pt.addr,
                               &workq_elemt->pt2pt.request);
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case SSEND:
            MPIDI_NM_mpi_ssend(workq_elemt->pt2pt.send_buf, workq_elemt->pt2pt.count,
                               workq_elemt->pt2pt.datatype, workq_elemt->pt2pt.rank,
                               workq_elemt->pt2pt.tag, workq_elemt->pt2pt.comm_ptr,
                               workq_elemt->pt2pt.context_offset, workq_elemt->pt2pt.addr,
                               &workq_elemt->pt2pt.request);
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case ISSEND:
            MPIDI_NM_mpi_issend(workq_elemt->pt2pt.send_buf, workq_elemt->pt2pt.count,
                                workq_elemt->pt2pt.datatype, workq_elemt->pt2pt.rank,
                                workq_elemt->pt2pt.tag, workq_elemt->pt2pt.comm_ptr,
                                workq_elemt->pt2pt.context_offset, workq_elemt->pt2pt.addr,
                                &workq_elemt->pt2pt.request);
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case RECV:
            MPIDI_NM_mpi_recv(workq_elemt->pt2pt.recv_buf, workq_elemt->pt2pt.count,
                              workq_elemt->pt2pt.datatype, workq_elemt->pt2pt.rank,
                              workq_elemt->pt2pt.tag, workq_elemt->pt2pt.comm_ptr,
                              workq_elemt->pt2pt.context_offset, workq_elemt->pt2pt.addr,
                              workq_elemt->pt2pt.status, &workq_elemt->pt2pt.request);
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case IRECV:
            MPIDI_NM_mpi_irecv(workq_elemt->pt2pt.recv_buf, workq_elemt->pt2pt.count,
                               workq_elemt->pt2pt.datatype, workq_elemt->pt2pt.rank,
                               workq_elemt->pt2pt.tag, workq_elemt->pt2pt.comm_ptr,
                               workq_elemt->pt2pt.context_offset, workq_elemt->pt2pt.addr,
                               &workq_elemt->pt2pt.request);
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case IPROBE:
            MPIDI_NM_mpi_iprobe(workq_elemt->pt2pt.rank, workq_elemt->pt2pt.tag,
                                workq_elemt->pt2pt.comm_ptr, workq_elemt->pt2pt.context_offset,
                                workq_elemt->pt2pt.addr, workq_elemt->pt2pt.flag,
                                workq_elemt->pt2pt.status);
            OPA_store_int(workq_elemt->processed, 1);   /* set to true to let the main thread
                                                         * learn that the item is processed */
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case IMPROBE:
            /* Note for future optimization: right now netmod allocates another request
             * object for message object. We could pass `req` instead and let netmod use
             * it, just like we did in send/recv. */
            MPIDI_NM_mpi_improbe(workq_elemt->pt2pt.rank, workq_elemt->pt2pt.tag,
                                 workq_elemt->pt2pt.comm_ptr, workq_elemt->pt2pt.context_offset,
                                 workq_elemt->pt2pt.addr, workq_elemt->pt2pt.flag,
                                 workq_elemt->pt2pt.message, workq_elemt->pt2pt.status);
            OPA_store_int(workq_elemt->processed, 1);
            MPIDI_workq_release_pt2pt_elemt(workq_elemt);
            break;
        case PUT:
            MPIDI_NM_mpi_put(workq_elemt->rma.origin_addr, workq_elemt->rma.origin_count,
                             workq_elemt->rma.origin_datatype, workq_elemt->rma.target_rank,
                             workq_elemt->rma.target_disp, workq_elemt->rma.target_count,
                             workq_elemt->rma.target_datatype, workq_elemt->rma.win_ptr,
                             workq_elemt->rma.addr);
            OPA_decr_int(&MPIDI_CH4U_WIN(workq_elemt->rma.win_ptr, local_enq_cnts));
            MPIR_Datatype_release_if_not_builtin(workq_elemt->rma.origin_datatype);
            MPIR_Datatype_release_if_not_builtin(workq_elemt->rma.target_datatype);
            MPIDI_workq_elemt_free(workq_elemt);
            break;
        case GET:
            MPIDI_NM_mpi_get(workq_elemt->rma.result_addr, workq_elemt->rma.result_count,
                             workq_elemt->rma.result_datatype, workq_elemt->rma.target_rank,
                             workq_elemt->rma.target_disp, workq_elemt->rma.target_count,
                             workq_elemt->rma.target_datatype, workq_elemt->rma.win_ptr,
                             workq_elemt->rma.addr);
            OPA_decr_int(&MPIDI_CH4U_WIN(workq_elemt->rma.win_ptr, local_enq_cnts));
            /* Get handoff uses result_datatype instead of origin_datatype */
            MPIR_Datatype_release_if_not_builtin(workq_elemt->rma.result_datatype);
            MPIR_Datatype_release_if_not_builtin(workq_elemt->rma.target_datatype);
            MPIDI_workq_elemt_free(workq_elemt);
            break;
        default:
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
    }

  fn_fail:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress_pobj(int vni_idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t *workq_elemt = NULL;
    MPIDI_workq_list_t *cur_workq;

    MPIR_Assert(MPIDI_CH4_ENABLE_POBJ_WORKQUEUES);

    MPIDI_WORKQ_PROGRESS_START;
    DL_FOREACH(MPIDI_CH4_Global.workqueues.pobj[vni_idx], cur_workq) {
        MPIDI_workq_dequeue(&cur_workq->pend_ops, (void **) &workq_elemt);
        while (workq_elemt != NULL) {
            MPIDI_WORKQ_ISSUE_START;
            mpi_errno = MPIDI_workq_dispatch(workq_elemt);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIDI_WORKQ_ISSUE_STOP;
            MPIDI_workq_dequeue(&cur_workq->pend_ops, (void **) &workq_elemt);
        }
    }
    MPIDI_WORKQ_PROGRESS_STOP;
  fn_fail:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress_pvni(int vni_idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t *workq_elemt = NULL;

    MPIR_Assert(!MPIDI_CH4_ENABLE_POBJ_WORKQUEUES);

    MPIDI_WORKQ_PROGRESS_START;

    MPIDI_workq_dequeue(&MPIDI_CH4_Global.workqueues.pvni[vni_idx], (void **) &workq_elemt);
    while (workq_elemt != NULL) {
        MPIDI_WORKQ_ISSUE_START;
        mpi_errno = MPIDI_workq_dispatch(workq_elemt);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
        MPIDI_WORKQ_ISSUE_STOP;
        MPIDI_workq_dequeue(&MPIDI_CH4_Global.workqueues.pvni[vni_idx], (void **) &workq_elemt);
    }
    MPIDI_WORKQ_PROGRESS_STOP;
  fn_fail:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_pt2pt_enqueue(MPIDI_workq_t * workq,
                                                        MPIDI_workq_op_t op,
                                                        const void *send_buf,
                                                        void *recv_buf,
                                                        MPI_Aint count,
                                                        MPI_Datatype datatype,
                                                        int rank,
                                                        int tag,
                                                        MPIR_Comm * comm_ptr,
                                                        int context_offset,
                                                        MPIDI_av_entry_t * addr,
                                                        MPI_Status * status,
                                                        MPIR_Request * request,
                                                        int *flag,
                                                        MPIR_Request ** message,
                                                        OPA_int_t * processed)
{
    MPIDI_WORKQ_PT2PT_ENQUEUE_START;
    MPIDI_workq_pt2pt_enqueue_body(workq, op, send_buf, recv_buf, count, datatype,
                                   rank, tag, comm_ptr, context_offset, addr, status,
                                   request, flag, message, processed);
    MPIDI_WORKQ_PT2PT_ENQUEUE_STOP;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_rma_enqueue(MPIDI_workq_t * workq,
                                                      MPIDI_workq_op_t op,
                                                      const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      void *result_addr,
                                                      int result_count,
                                                      MPI_Datatype result_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype,
                                                      MPI_Op acc_op,
                                                      MPIR_Group * group,
                                                      int lock_type,
                                                      int assert,
                                                      MPIR_Win * win_ptr, MPIDI_av_entry_t * addr,
                                                      OPA_int_t * processed)
{
    MPIDI_WORKQ_RMA_ENQUEUE_START;
    MPIDI_workq_rma_enqueue_body(workq, op, origin_addr, origin_count, origin_datatype,
                                 result_addr, result_count, result_datatype,
                                 target_rank, target_disp, target_count, target_datatype,
                                 acc_op, group, lock_type, assert, win_ptr, addr, processed);
    MPIDI_WORKQ_RMA_ENQUEUE_STOP;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress(int vni_idx)
{
    int mpi_errno;

    if (MPIDI_CH4_ENABLE_POBJ_WORKQUEUES) {
        MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
        mpi_errno = MPIDI_workq_vni_progress_pobj(vni_idx);
        MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_locks[vni_idx]);
    } else {
        /* Per-VNI workqueue does not require VNI lock, since
         * the queue is lock free */
        mpi_errno = MPIDI_workq_vni_progress_pvni(vni_idx);
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_global_progress(int *made_progress)
{
    int mpi_errno, vni_idx;
    *made_progress = 1;
    for (vni_idx = 0; vni_idx < MPIDI_CH4_Global.n_netmod_vnis; vni_idx++) {
        mpi_errno = MPIDI_workq_vni_progress(vni_idx);
        if (unlikely(mpi_errno != MPI_SUCCESS))
            break;
    }
    return mpi_errno;
}
#else /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */
#define MPIDI_workq_vni_progress_pobj(...)
#define MPIDI_workq_vni_progress_pvni(...)
#define MPIDI_workq_pt2pt_enqueue(...)
#define MPIDI_workq_rma_enqueue(...)
#define MPIDI_workq_vni_progress(...)
#define MPIDI_workq_global_progress(...)
#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

/* For profiling */
extern double MPIDI_pt2pt_enqueue_time;
extern double MPIDI_pt2pt_progress_time;
extern double MPIDI_pt2pt_issue_pend_time;
extern double MPIDI_rma_enqueue_time;
extern double MPIDI_rma_progress_time;
extern double MPIDI_rma_issue_pend_time;

#if defined(MPIDI_WORKQ_PROFILE)
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START  double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP                          \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_pt2pt_enqueue_time += (enqueue_t2 - enqueue_t1);  \
    } while (0)
#define MPIDI_WORKQ_PROGRESS_START double progress_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PROGRESS_STOP                                 \
    do {                                                                \
        double progress_t2 = MPI_Wtime();                               \
        MPIDI_pt2pt_progress_time += (progress_t2 - progress_t1);       \
    } while (0)
#define MPIDI_WORKQ_ISSUE_START    double issue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_ISSUE_STOP                            \
    do {                                                        \
        double issue_t2 = MPI_Wtime();                          \
        MPIDI_pt2pt_issue_pend_time += (issue_t2 - issue_t1);   \
    } while (0)

#define MPIDI_WORKQ_RMA_ENQUEUE_START    double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP                            \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_rma_enqueue_time += (enqueue_t2 - enqueue_t1);    \
    } while (0)
#else /*!defined(MPIDI_WORKQ_PROFILE) */
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP
#define MPIDI_WORKQ_PROGRESS_START
#define MPIDI_WORKQ_PROGRESS_STOP
#define MPIDI_WORKQ_ISSUE_START
#define MPIDI_WORKQ_ISSUE_STOP

#define MPIDI_WORKQ_RMA_ENQUEUE_START
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP
#endif

#endif /* CH4I_WORKQ_H_INCLUDED */
