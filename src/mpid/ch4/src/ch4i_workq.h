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
