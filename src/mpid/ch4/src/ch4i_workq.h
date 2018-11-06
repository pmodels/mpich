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
struct MPIDI_workq_elemt MPIDI_workq_elemt_direct[MPIDI_WORKQ_ELEMT_PREALLOC];
extern MPIR_Object_alloc_t MPIDI_workq_elemt_mem;

/* Forward declarations of the routines that can be pushed to a work-queue */

MPL_STATIC_INLINE_PREFIX int MPIDI_send_unsafe(const void *, int, MPI_Datatype, int, int,
                                               MPIR_Comm *, int, MPIDI_av_entry_t *,
                                               MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_isend_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_ssend_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_issend_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                 MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                 MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_recv_unsafe(void *, MPI_Aint, MPI_Datatype, int, int,
                                               MPIR_Comm *, int, MPIDI_av_entry_t *, MPI_Status *,
                                               MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_irecv_unsafe(void *, MPI_Aint, MPI_Datatype, int, int,
                                                MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_imrecv_unsafe(void *, MPI_Aint, MPI_Datatype, MPIR_Request *,
                                                 MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_put_unsafe(const void *, int, MPI_Datatype, int, MPI_Aint, int,
                                              MPI_Datatype, MPIR_Win *);
MPL_STATIC_INLINE_PREFIX int MPIDI_get_unsafe(void *, int, MPI_Datatype, int, MPI_Aint, int,
                                              MPI_Datatype, MPIR_Win *);
MPL_STATIC_INLINE_PREFIX struct MPIDI_workq_elemt *MPIDI_workq_elemt_create(void)
{
    return MPIR_Handle_obj_alloc(&MPIDI_workq_elemt_mem);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_elemt_free(struct MPIDI_workq_elemt *elemt)
{
    MPIR_Handle_obj_free(&MPIDI_workq_elemt_mem, elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_pt2pt_enqueue(MPIDI_workq_op_t op,
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

    /* Find what type of work descriptor (wd) this element is and populate
     * it accordingly. */

    switch (op) {
        case SEND:
        case ISEND:
        case SSEND:
        case ISSEND:
        case RSEND:
        case IRSEND:
            {
                struct MPIDI_workq_send *wd = &pt2pt_elemt->params.pt2pt.send;
                wd->send_buf = send_buf;
                wd->count = count;
                wd->datatype = datatype;
                wd->rank = rank;
                wd->tag = tag;
                wd->comm_ptr = comm_ptr;
                wd->context_offset = context_offset;
                wd->addr = addr;
                wd->request = request;
                break;
            }
        case RECV:
            {
                struct MPIDI_workq_recv *wd = &pt2pt_elemt->params.pt2pt.recv;
                wd->recv_buf = recv_buf;
                wd->count = count;
                wd->datatype = datatype;
                wd->rank = rank;
                wd->tag = tag;
                wd->comm_ptr = comm_ptr;
                wd->context_offset = context_offset;
                wd->addr = addr;
                wd->status = status;
                wd->request = request;
                break;
            }
        case IRECV:
            {
                struct MPIDI_workq_irecv *wd = &pt2pt_elemt->params.pt2pt.irecv;
                wd->recv_buf = recv_buf;
                wd->count = count;
                wd->datatype = datatype;
                wd->rank = rank;
                wd->tag = tag;
                wd->comm_ptr = comm_ptr;
                wd->context_offset = context_offset;
                wd->addr = addr;
                wd->request = request;
                break;
            }
        case IPROBE:
            {
                struct MPIDI_workq_iprobe *wd = &pt2pt_elemt->params.pt2pt.iprobe;
                wd->count = count;
                wd->datatype = datatype;
                wd->rank = rank;
                wd->tag = tag;
                wd->comm_ptr = comm_ptr;
                wd->context_offset = context_offset;
                wd->addr = addr;
                wd->status = status;
                wd->request = request;
                wd->flag = flag;
                break;
            }
        case IMPROBE:
            {
                struct MPIDI_workq_improbe *wd = &pt2pt_elemt->params.pt2pt.improbe;
                wd->count = count;
                wd->datatype = datatype;
                wd->rank = rank;
                wd->tag = tag;
                wd->comm_ptr = comm_ptr;
                wd->context_offset = context_offset;
                wd->addr = addr;
                wd->status = status;
                wd->request = request;
                wd->flag = flag;
                wd->message = message;
                break;
            }
        case IMRECV:
            {
                struct MPIDI_workq_imrecv *wd = &pt2pt_elemt->params.pt2pt.imrecv;
                wd->buf = recv_buf;
                wd->count = count;
                wd->datatype = datatype;
                wd->message = message;
                wd->request = request;
                break;
            }
        default:
            MPIR_Assert(0);
    }

    MPIDI_workq_enqueue(&MPIDI_CH4_Global.workqueue, pt2pt_elemt);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_rma_enqueue(MPIDI_workq_op_t op,
                                                      const void *origin_addr,
                                                      int origin_count,
                                                      MPI_Datatype origin_datatype,
                                                      const void *compare_addr,
                                                      void *result_addr,
                                                      int result_count,
                                                      MPI_Datatype result_datatype,
                                                      int target_rank,
                                                      MPI_Aint target_disp,
                                                      int target_count,
                                                      MPI_Datatype target_datatype,
                                                      MPI_Op acc_op,
                                                      MPIR_Win * win_ptr,
                                                      MPIDI_av_entry_t * addr,
                                                      OPA_int_t * processed)
{
    MPIDI_workq_elemt_t *rma_elemt = NULL;
    rma_elemt = MPIDI_workq_elemt_create();
    rma_elemt->op = op;
    rma_elemt->processed = processed;

    /* Find what type of work descriptor (wd) this element is and populate
     * it accordingly. */

    switch (op) {
        case PUT:
            {
                struct MPIDI_workq_put *wd = &rma_elemt->params.rma.put;
                wd->origin_addr = origin_addr;
                wd->origin_count = origin_count;
                wd->origin_datatype = origin_datatype;
                wd->target_rank = target_rank;
                wd->target_disp = target_disp;
                wd->target_count = target_count;
                wd->target_datatype = target_datatype;
                wd->win_ptr = win_ptr;
                wd->addr = addr;
                break;
            }
        case GET:
            {
                struct MPIDI_workq_get *wd = &rma_elemt->params.rma.get;
                wd->origin_addr = result_addr;
                wd->origin_count = origin_count;
                wd->origin_datatype = origin_datatype;
                wd->target_rank = target_rank;
                wd->target_disp = target_disp;
                wd->target_count = target_count;
                wd->target_datatype = target_datatype;
                wd->win_ptr = win_ptr;
                wd->addr = addr;
                break;
            }
        default:
            MPIR_Assert(0);
    }
    MPIDI_workq_enqueue(&MPIDI_CH4_Global.workqueue, rma_elemt);
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
    MPIR_Request *req;
    MPI_Datatype datatype, origin_datatype, target_datatype;

    MPIR_Assert(workq_elemt != NULL);

    switch (workq_elemt->op) {
        case SEND:{
                struct MPIDI_workq_send *wd = &workq_elemt->params.pt2pt.send;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_send_unsafe(wd->send_buf, wd->count, wd->datatype, wd->rank,
                                  wd->tag, wd->comm_ptr, wd->context_offset, wd->addr, &req);
                MPIR_Datatype_release_if_not_builtin(datatype);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case ISEND:{
                struct MPIDI_workq_send *wd = &workq_elemt->params.pt2pt.send;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_isend_unsafe(wd->send_buf, wd->count, wd->datatype, wd->rank,
                                   wd->tag, wd->comm_ptr, wd->context_offset, wd->addr, &req);
                MPIR_Datatype_release_if_not_builtin(datatype);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case SSEND:{
                struct MPIDI_workq_send *wd = &workq_elemt->params.pt2pt.send;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_ssend_unsafe(wd->send_buf, wd->count, wd->datatype, wd->rank,
                                   wd->tag, wd->comm_ptr, wd->context_offset, wd->addr, &req);
                MPIR_Datatype_release_if_not_builtin(datatype);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case ISSEND:{
                struct MPIDI_workq_send *wd = &workq_elemt->params.pt2pt.send;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_issend_unsafe(wd->send_buf, wd->count, wd->datatype, wd->rank,
                                    wd->tag, wd->comm_ptr, wd->context_offset, wd->addr, &req);
                MPIR_Datatype_release_if_not_builtin(datatype);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case RECV:{
                struct MPIDI_workq_recv *wd = &workq_elemt->params.pt2pt.recv;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_recv_unsafe(wd->recv_buf, wd->count, wd->datatype, wd->rank, wd->tag,
                                  wd->comm_ptr, wd->context_offset, wd->addr, wd->status, &req);
                MPIR_Datatype_release_if_not_builtin(datatype);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case IRECV:{
                struct MPIDI_workq_irecv *wd = &workq_elemt->params.pt2pt.irecv;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_irecv_unsafe(wd->recv_buf, wd->count, wd->datatype, wd->rank, wd->tag,
                                   wd->comm_ptr, wd->context_offset, wd->addr, &req);
                MPIR_Datatype_release_if_not_builtin(datatype);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case IMRECV:{
                struct MPIDI_workq_imrecv *wd = &workq_elemt->params.pt2pt.imrecv;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_imrecv_unsafe(wd->buf, wd->count, wd->datatype, *wd->message, &req);
                MPIR_Datatype_release_if_not_builtin(datatype);
#ifndef MPIDI_CH4_DIRECT_NETMOD
                if (!MPIDI_CH4I_REQUEST(*wd->message, is_local))
#endif
                    MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case PUT:{
                struct MPIDI_workq_put *wd = &workq_elemt->params.rma.put;
                origin_datatype = wd->origin_datatype;
                target_datatype = wd->target_datatype;
                MPIDI_put_unsafe(wd->origin_addr, wd->origin_count, wd->origin_datatype,
                                 wd->target_rank, wd->target_disp,
                                 wd->target_count, wd->target_datatype, wd->win_ptr);
                MPIR_Datatype_release_if_not_builtin(origin_datatype);
                MPIR_Datatype_release_if_not_builtin(target_datatype);
                MPIDI_workq_elemt_free(workq_elemt);
                break;
            }
        case GET:{
                struct MPIDI_workq_get *wd = &workq_elemt->params.rma.get;
                origin_datatype = wd->origin_datatype;
                target_datatype = wd->target_datatype;
                MPIDI_get_unsafe(wd->origin_addr, wd->origin_count, wd->origin_datatype,
                                 wd->target_rank, wd->target_disp,
                                 wd->target_count, wd->target_datatype, wd->win_ptr);
                MPIR_Datatype_release_if_not_builtin(origin_datatype);
                MPIR_Datatype_release_if_not_builtin(target_datatype);
                MPIDI_workq_elemt_free(workq_elemt);
                break;
            }
        default:
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
    }

  fn_fail:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress_unsafe(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t *workq_elemt = NULL;

    MPIDI_workq_dequeue(&MPIDI_CH4_Global.workqueue, (void **) &workq_elemt);
    while (workq_elemt != NULL) {
        mpi_errno = MPIDI_workq_dispatch(workq_elemt);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
        MPIDI_workq_dequeue(&MPIDI_CH4_Global.workqueue, (void **) &workq_elemt);
    }

  fn_fail:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VNI, MPIDI_CH4_Global.vni_lock);

    mpi_errno = MPIDI_workq_vni_progress_unsafe();

    MPID_THREAD_CS_EXIT(VNI, MPIDI_CH4_Global.vni_lock);
  fn_fail:
    return mpi_errno;
}

#else /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */
#define MPIDI_workq_pt2pt_enqueue(...)
#define MPIDI_workq_rma_enqueue(...)

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress_unsafe(void)
{
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_workq_vni_progress(void)
{
    return MPI_SUCCESS;
}
#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

#endif /* CH4I_WORKQ_H_INCLUDED */
