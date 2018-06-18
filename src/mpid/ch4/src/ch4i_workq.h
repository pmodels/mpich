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

MPL_STATIC_INLINE_PREFIX struct MPIDI_workq_elemt *MPIDI_workq_elemt_create(void)
{
    return MPIR_Handle_obj_alloc(&MPIDI_workq_elemt_mem);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_elemt_free(struct MPIDI_workq_elemt *elemt)
{
    MPIR_Handle_obj_free(&MPIDI_workq_elemt_mem, elemt);
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
    MPIDI_workq_elemt_t *pt2pt_elemt;

    MPIR_Assert(request != NULL);

    MPIR_Request_add_ref(request);
    pt2pt_elemt = &request->dev.ch4.command;
    pt2pt_elemt->op = op;
    pt2pt_elemt->processed = processed;

    switch (op) {
    case SEND:
    case ISEND:
    case SSEND:
    case ISSEND:
    case RSEND:
    case IRSEND:
        pt2pt_elemt->op.pt2pt.send.send_buf = send_buf;
        pt2pt_elemt->op.pt2pt.send.count = count;
        pt2pt_elemt->op.pt2pt.send.datatype = datatype;
        pt2pt_elemt->op.pt2pt.send.rank = rank;
        pt2pt_elemt->op.pt2pt.send.tag = tag;
        pt2pt_elemt->op.pt2pt.send.comm_ptr = comm_ptr;
        pt2pt_elemt->op.pt2pt.send.context_offset = context_offset;
        pt2pt_elemt->op.pt2pt.send.addr = addr;
        pt2pt_elemt->op.pt2pt.send.request = request;
        break;
    case RECV:
        pt2pt_elemt->op.pt2pt.recv.recv_buf = recv_buf;
        pt2pt_elemt->op.pt2pt.recv.count = count;
        pt2pt_elemt->op.pt2pt.recv.datatype = datatype;
        pt2pt_elemt->op.pt2pt.recv.rank = rank;
        pt2pt_elemt->op.pt2pt.recv.tag = tag;
        pt2pt_elemt->op.pt2pt.recv.comm_ptr = comm_ptr;
        pt2pt_elemt->op.pt2pt.recv.context_offset = context_offset;
        pt2pt_elemt->op.pt2pt.recv.addr = addr;
        pt2pt_elemt->op.pt2pt.recv.status = status;
        pt2pt_elemt->op.pt2pt.recv.request = request;
        break;
    case IRECV:
        pt2pt_elemt->op.pt2pt.irecv.recv_buf = recv_buf;
        pt2pt_elemt->op.pt2pt.irecv.count = count;
        pt2pt_elemt->op.pt2pt.irecv.datatype = datatype;
        pt2pt_elemt->op.pt2pt.irecv.rank = rank;
        pt2pt_elemt->op.pt2pt.irecv.tag = tag;
        pt2pt_elemt->op.pt2pt.irecv.comm_ptr = comm_ptr;
        pt2pt_elemt->op.pt2pt.irecv.context_offset = context_offset;
        pt2pt_elemt->op.pt2pt.irecv.addr = addr;
        pt2pt_elemt->op.pt2pt.irecv.request = request;
        break;
    case IPROBE:
        pt2pt_elemt->op.pt2pt.iprobe.count = count;
        pt2pt_elemt->op.pt2pt.iprobe.datatype = datatype;
        pt2pt_elemt->op.pt2pt.iprobe.rank = rank;
        pt2pt_elemt->op.pt2pt.iprobe.tag = tag;
        pt2pt_elemt->op.pt2pt.iprobe.comm_ptr = comm_ptr;
        pt2pt_elemt->op.pt2pt.iprobe.context_offset = context_offset;
        pt2pt_elemt->op.pt2pt.iprobe.addr = addr;
        pt2pt_elemt->op.pt2pt.iprobe.status = status;
        pt2pt_elemt->op.pt2pt.iprobe.request = request;
        pt2pt_elemt->op.pt2pt.iprobe.flag = flag;
        break;
    case IMPROBE:
        pt2pt_elemt->op.pt2pt.improbe.count = count;
        pt2pt_elemt->op.pt2pt.improbe.datatype = datatype;
        pt2pt_elemt->op.pt2pt.improbe.rank = rank;
        pt2pt_elemt->op.pt2pt.improbe.tag = tag;
        pt2pt_elemt->op.pt2pt.improbe.comm_ptr = comm_ptr;
        pt2pt_elemt->op.pt2pt.improbe.context_offset = context_offset;
        pt2pt_elemt->op.pt2pt.improbe.addr = addr;
        pt2pt_elemt->op.pt2pt.improbe.status = status;
        pt2pt_elemt->op.pt2pt.improbe.request = request;
        pt2pt_elemt->op.pt2pt.improbe.flag = flag;
        pt2pt_elemt->op.pt2pt.improbe.message = message;
        break;
    default:
        MPIR_Assert(0);
    })

    MPIDI_workq_enqueue(workq, pt2pt_elemt);
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
                                                      MPIR_Win * win_ptr,
                                                      MPIDI_av_entry_t * addr,
                                                      OPA_int_t * processed)
{
    MPIDI_workq_elemt_t *rma_elemt = NULL;
    rma_elemt = MPIDI_workq_elemt_create();
    rma_elemt->op = op;
    rma_elemt->processed = processed;

    switch (op) {
    case PUT:
        rma_elemt->op.rma.put.origin_addr = origin_addr;
        rma_elemt->op.rma.put.origin_count = origin_count;
        rma_elemt->op.rma.put.origin_datatype = origin_datatype;
        rma_elemt->op.rma.put.target_rank = target_rank;
        rma_elemt->op.rma.put.target_disp = target_disp;
        rma_elemt->op.rma.put.target_count = target_count;
        rma_elemt->op.rma.put.target_datatype = target_datatype;
        rma_elemt->op.rma.put.win_ptr = win_ptr;
        rma_elemt->op.rma.put.addr = addr;
        break;
    case GET:
        rma_elemt->op.rma.get.result_addr = result_addr;
        rma_elemt->op.rma.get.result_count = result_count;
        rma_elemt->op.rma.get.result_datatype = result_datatype;
        rma_elemt->op.rma.get.target_rank = target_rank;
        rma_elemt->op.rma.get.target_disp = target_disp;
        rma_elemt->op.rma.get.target_count = target_count;
        rma_elemt->op.rma.get.target_datatype = target_datatype;
        rma_elemt->op.rma.get.win_ptr = win_ptr;
        rma_elemt->op.rma.get.addr = addr;
        break;
    case POST:
        rma_elemt->op.rma.post.group = group;
        rma_elemt->op.rma.post.assert = assert;
        rma_elemt->op.rma.post.win_ptr = win_ptr;
        break;
    case COMPLETE:
        rma_elemt->op.rma.complete.win_ptr = win_ptr;
        break;
    case FENCE:
        rma_elemt->op.rma.fence.win_ptr = win_ptr;
        break;
    case LOCK:
        rma_elemt->op.rma.lock.target_rank = target_rank;
        rma_elemt->op.rma.lock.lock_type = lock_type;
        rma_elemt->op.rma.lock.assert = assert;
        rma_elemt->op.rma.lock.win_ptr = win_ptr;
        rma_elemt->op.rma.lock.addr = addr;
        break;
    case UNLOCK:
        rma_elemt->op.rma.unlock.target_rank = target_rank;
        rma_elemt->op.rma.unlock.win_ptr = win_ptr;
        rma_elemt->op.rma.unlock.addr = addr;
        break;
    case LOCK_ALL:
        rma_elemt->op.rma.lock_all.assert = assert;
        rma_elemt->op.rma.lock_all.win_ptr = win_ptr;
        break;
    case UNLOCK_ALL:
        rma_elemt->op.rma.unlock_all.win_ptr = win_ptr;
        break;
    case FLUSH:
        rma_elemt->op.rma.flush.target_rank = target_rank;
        rma_elemt->op.rma.flush.win_ptr = win_ptr;
        rma_elemt->op.rma.flush.addr = addr;
        break;
    case FLUSH_ALL:
        rma_elemt->op.rma.flush_all.win_ptr = win_ptr;
        break;
    case FLUSH_LOCAL:
        rma_elemt->op.rma.flush_local.target_rank = target_rank;
        rma_elemt->op.rma.flush_local.win_ptr = win_ptr;
        rma_elemt->op.rma.flush_local.addr = addr;
        break;
    case FLUSH_LOCAL_ALL:
        rma_elemt->op.rma.flush_local_all.win_ptr = win_ptr;
        break;
    default:
        MPIR_Assert(0);
    }
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
    MPIR_Request *req;

    switch (workq_elemt->op) {
        default:
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
    }

  fn_fail:
    return mpi_errno;
}

#else /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */
#define MPIDI_workq_pt2pt_enqueue(...)
#define MPIDI_workq_rma_enqueue(...)
#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

#endif /* CH4I_WORKQ_H_INCLUDED */
