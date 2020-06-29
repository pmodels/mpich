/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"

#if defined(MPIDI_CH4_USE_WORK_QUEUES)

static int MPIDI_workq_dispatch(MPIDI_workq_elemt_t * workq_elemt)
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
        case CSEND:{
                struct MPIDI_workq_csend *wd = &workq_elemt->params.pt2pt.csend;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_send_coll_unsafe(wd->send_buf, wd->count, wd->datatype, wd->rank,
                                       wd->tag, wd->comm_ptr, wd->context_offset, wd->addr, &req,
                                       &(wd->errflag));
                MPIR_Datatype_release_if_not_builtin(datatype);
                MPIDI_workq_release_pt2pt_elemt(workq_elemt);
                break;
            }
        case ICSEND:{
                struct MPIDI_workq_csend *wd = &workq_elemt->params.pt2pt.csend;
                req = wd->request;
                datatype = wd->datatype;
                MPIDI_isend_coll_unsafe(wd->send_buf, wd->count, wd->datatype, wd->rank,
                                        wd->tag, wd->comm_ptr, wd->context_offset, wd->addr, &req,
                                        &(wd->errflag));
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
                datatype = wd->datatype;
                MPIDI_imrecv_unsafe(wd->buf, wd->count, wd->datatype, *wd->message);
                MPIR_Datatype_release_if_not_builtin(datatype);
#ifndef MPIDI_CH4_DIRECT_NETMOD
                if (!MPIDI_REQUEST(*wd->message, is_local))
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

int MPIDI_workq_vci_progress_unsafe(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_workq_elemt_t *workq_elemt = NULL;

    MPIDI_workq_dequeue(&MPIDI_global.workqueue, (void **) &workq_elemt);
    while (workq_elemt != NULL) {
        mpi_errno = MPIDI_workq_dispatch(workq_elemt);
        if (mpi_errno != MPI_SUCCESS)
            goto fn_fail;
        MPIDI_workq_dequeue(&MPIDI_global.workqueue, (void **) &workq_elemt);
    }

  fn_fail:
    return mpi_errno;
}

int MPIDI_workq_vci_progress(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);

    mpi_errno = MPIDI_workq_vci_progress_unsafe();

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
  fn_fail:
    return mpi_errno;
}

#else /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */

int MPIDI_workq_vci_progress_unsafe(void)
{
    return MPI_SUCCESS;
}

int MPIDI_workq_vci_progress(void)
{
    return MPI_SUCCESS;
}

#endif /* #if defined(MPIDI_CH4_USE_WORK_QUEUES) */
