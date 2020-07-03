/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4I_WORKQ_H_INCLUDED
#define CH4I_WORKQ_H_INCLUDED

#include "ch4i_workq_types.h"
#include <utlist.h>

int MPIDI_workq_vci_progress_unsafe(void);
int MPIDI_workq_vci_progress(void);

#if defined(MPIDI_CH4_USE_WORK_QUEUES)
extern struct MPIDI_workq_elemt MPIDI_workq_elemt_direct[MPIDI_WORKQ_ELEMT_PREALLOC];
extern MPIR_Object_alloc_t MPIDI_workq_elemt_mem;

extern MPID_Thread_mutex_t MPIDI_workq_lock;

/* Forward declarations of the routines that can be pushed to a work-queue */

MPL_STATIC_INLINE_PREFIX int MPIDI_send_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
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
MPL_STATIC_INLINE_PREFIX int MPIDI_send_coll_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                    MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                    MPIR_Request **, MPIR_Errflag_t *);
MPL_STATIC_INLINE_PREFIX int MPIDI_isend_coll_unsafe(const void *, MPI_Aint, MPI_Datatype, int, int,
                                                     MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                     MPIR_Request **, MPIR_Errflag_t *);
MPL_STATIC_INLINE_PREFIX int MPIDI_recv_unsafe(void *, MPI_Aint, MPI_Datatype, int, int,
                                               MPIR_Comm *, int, MPIDI_av_entry_t *, MPI_Status *,
                                               MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_irecv_unsafe(void *, MPI_Aint, MPI_Datatype, int, int,
                                                MPIR_Comm *, int, MPIDI_av_entry_t *,
                                                MPIR_Request **);
MPL_STATIC_INLINE_PREFIX int MPIDI_imrecv_unsafe(void *, MPI_Aint, MPI_Datatype, MPIR_Request *);
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
                                                        MPL_atomic_int_t * processed)
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

    MPID_THREAD_CS_ENTER(VCI, MPIDI_workq_lock);
    MPIDI_workq_enqueue(&MPIDI_global.workqueue, pt2pt_elemt);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_workq_lock);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_csend_enqueue(MPIDI_workq_op_t op,
                                                        const void *send_buf,
                                                        MPI_Aint count,
                                                        MPI_Datatype datatype,
                                                        int rank,
                                                        int tag,
                                                        MPIR_Comm * comm_ptr,
                                                        int context_offset,
                                                        MPIDI_av_entry_t * addr,
                                                        MPIR_Request * request,
                                                        MPIR_Errflag_t * errflag)
{
    MPIDI_workq_elemt_t *pt2pt_elemt;
    struct MPIDI_workq_csend *wd;

    MPIR_Assert(request != NULL);
    MPIR_Assert(op == CSEND || op == ICSEND);

    MPIR_Request_add_ref(request);
    pt2pt_elemt = &request->dev.ch4.command;
    pt2pt_elemt->op = op;

    wd = &pt2pt_elemt->params.pt2pt.csend;
    wd->send_buf = send_buf;
    wd->count = count;
    wd->datatype = datatype;
    wd->rank = rank;
    wd->tag = tag;
    wd->comm_ptr = comm_ptr;
    wd->context_offset = context_offset;
    wd->addr = addr;
    wd->request = request;
    wd->errflag = *errflag;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_workq_lock);
    MPIDI_workq_enqueue(&MPIDI_global.workqueue, pt2pt_elemt);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_workq_lock);
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
                                                      MPL_atomic_int_t * processed)
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
                break;
            }
        default:
            MPIR_Assert(0);
    }
    MPID_THREAD_CS_ENTER(VCI, MPIDI_workq_lock);
    MPIDI_workq_enqueue(&MPIDI_global.workqueue, rma_elemt);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_workq_lock);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_workq_release_pt2pt_elemt(MPIDI_workq_elemt_t * workq_elemt)
{
    MPIR_Request *req;
    req = MPL_container_of(workq_elemt, MPIR_Request, dev.ch4.command);
    MPIR_Request_free(req);
}

#else /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */
#define MPIDI_workq_pt2pt_enqueue(...)
#define MPIDI_workq_rma_enqueue(...)
#define MPIDI_workq_csend_enqueue(...)

#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

#endif /* CH4I_WORKQ_H_INCLUDED */
