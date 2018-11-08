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
#ifndef POSIX_SEND_H_INCLUDED
#define POSIX_SEND_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include <../mpi/pt2pt/bsendutil.h>
/* ---------------------------------------------------- */
/* from mpid/ch3/channels/nemesis/include/mpid_nem_impl.h */
/* ---------------------------------------------------- */
/* assumes value!=0 means the fbox is full.  Contains acquire barrier to
 * ensure that later operations that are dependent on this check don't
 * escape earlier than this check. */
#define MPIDI_POSIX_fbox_is_full(pbox_) (OPA_load_acquire_int(&(pbox_)->flag.value))

/* ---------------------------------------------------- */
/* MPIDI_POSIX_do_isend                                             */
/* ---------------------------------------------------- */
#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_do_isend)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_do_isend(const void *buf,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIR_Request ** request, int type)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    MPI_Aint dt_true_lb;
    MPIR_Request *sreq = NULL;
    size_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_ISEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_ISEND);

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    MPIDI_POSIX_REQUEST_CREATE_COND_SREQ(*request);
    sreq = *request;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIDI_POSIX_ENVELOPE_SET(MPIDI_POSIX_REQUEST(sreq), comm->rank, tag,
                             comm->context_id + context_offset);
    MPIDI_POSIX_REQUEST(sreq)->user_buf = (char *) buf + dt_true_lb;
    MPIDI_POSIX_REQUEST(sreq)->user_count = count;
    MPIDI_POSIX_REQUEST(sreq)->datatype = datatype;
    MPIDI_POSIX_REQUEST(sreq)->data_sz = data_sz;
    MPIDI_POSIX_REQUEST(sreq)->type = type;
    MPIDI_POSIX_REQUEST(sreq)->dest = rank;
    MPIDI_POSIX_REQUEST(sreq)->next = NULL;
    MPIDI_POSIX_REQUEST(sreq)->pending = NULL;
    MPIDI_POSIX_REQUEST(sreq)->segment_ptr = NULL;

    if (!dt_contig) {
        MPIDI_POSIX_REQUEST(sreq)->segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1((MPIDI_POSIX_REQUEST(sreq)->segment_ptr == NULL), mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment_alloc");
        MPIR_Segment_init((char *) buf, MPIDI_POSIX_REQUEST(sreq)->user_count,
                          MPIDI_POSIX_REQUEST(sreq)->datatype,
                          MPIDI_POSIX_REQUEST(sreq)->segment_ptr);
        MPIDI_POSIX_REQUEST(sreq)->segment_first = 0;
        MPIDI_POSIX_REQUEST(sreq)->segment_size = data_sz;
    }

    MPIR_Datatype_add_ref_if_not_builtin(datatype);
    /* enqueue sreq */
    MPIDI_POSIX_REQUEST_ENQUEUE(sreq, MPIDI_POSIX_sendq);
    MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL,
                    (MPL_DBG_FDEST,
                     "Enqueued to grank %d from %d (comm_kind %d) in recv %d,%d,%d\n",
                     MPIDI_CH4U_rank_to_lpid(rank, comm), MPIDI_POSIX_mem_region.rank,
                     (int) comm->comm_kind, comm->rank, tag, comm->context_id + context_offset));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_ISEND);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_SEND)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_send(const void *buf, MPI_Aint count,
                                                  MPI_Datatype datatype, int rank, int tag,
                                                  MPIR_Comm * comm, int context_offset,
                                                  MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int dt_contig __attribute__ ((__unused__)), mpi_errno = MPI_SUCCESS;
    MPI_Aint dt_true_lb;
    size_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_SEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_SEND);
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    /* try to send immediately, contig, short message */
    if (dt_contig && data_sz <= MPIDI_POSIX_EAGER_THRESHOLD) {
        /* eager message */
        int grank = MPIDI_CH4U_rank_to_lpid(rank, comm);

        /* Try freeQ */
        if (!MPIDI_POSIX_queue_empty(MPIDI_POSIX_mem_region.my_freeQ)) {
            MPIDI_POSIX_cell_ptr_t cell;
            MPIDI_POSIX_queue_dequeue(MPIDI_POSIX_mem_region.my_freeQ, &cell);
            MPIDI_POSIX_ENVELOPE_SET(cell, comm->rank, tag, comm->context_id + context_offset);
            cell->pkt.mpich.datalen = data_sz;
            cell->pkt.mpich.type = MPIDI_POSIX_TYPEEAGER;
            MPIR_Memcpy((void *) cell->pkt.mpich.p.payload, (char *) buf + dt_true_lb, data_sz);
            cell->pending = NULL;
            MPIDI_POSIX_queue_enqueue(MPIDI_POSIX_mem_region.RecvQ[grank], cell);
            MPIDI_POSIX_REQUEST_COMPLETE_COND_SREQ(*request);
            MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL,
                            (MPL_DBG_FDEST, "Sent to grank %d from %d in send %d,%d,%d\n", grank,
                             cell->my_rank, cell->rank, cell->tag, cell->context_id));
            goto fn_exit;
        }
    }

    /* Long message or */
    /* Failed to send immediately - create and return request */
    mpi_errno =
        MPIDI_POSIX_do_isend(buf, count, datatype, rank, tag, comm, context_offset, request,
                             MPIDI_POSIX_TYPESTANDARD);

  fn_exit:
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_SEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_irsend(const void *buf, MPI_Aint count,
                                                MPI_Datatype datatype, int rank, int tag,
                                                MPIR_Comm * comm, int context_offset,
                                                MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_IRSEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_IRSEND);
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
    mpi_errno =
        MPIDI_POSIX_do_isend(buf, count, datatype, rank, tag, comm, context_offset, request,
                             MPIDI_POSIX_TYPEREADY);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_IRSEND);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_SSEND)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_ssend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * addr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_SSEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_SSEND);
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
    mpi_errno =
        MPIDI_POSIX_do_isend(buf, count, datatype, rank, tag, comm, context_offset, request,
                             MPIDI_POSIX_TYPESYNC);
    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_SSEND);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_SEND_INIT)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_send_init(const void *buf,
                                                       int count,
                                                       MPI_Datatype datatype,
                                                       int rank,
                                                       int tag,
                                                       MPIR_Comm * comm, int context_offset,
                                                       MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_SEND_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_SEND_INIT);
    MPIDI_POSIX_REQUEST_CREATE_SREQ(sreq);
    MPIR_Object_set_ref(sreq, 1);
    MPIR_cc_set(&(sreq)->cc, 0);
    sreq->kind = MPIR_REQUEST_KIND__PREQUEST_SEND;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIDI_POSIX_ENVELOPE_SET(MPIDI_POSIX_REQUEST(sreq), comm->rank, tag,
                             comm->context_id + context_offset);
    MPIDI_POSIX_REQUEST(sreq)->user_buf = (char *) buf;
    MPIDI_POSIX_REQUEST(sreq)->user_count = count;
    MPIDI_POSIX_REQUEST(sreq)->dest = rank;
    MPIDI_POSIX_REQUEST(sreq)->datatype = datatype;
    MPIDI_POSIX_REQUEST(sreq)->type = MPIDI_POSIX_TYPESTANDARD;
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_SEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_SSEND_INIT)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_ssend_init(const void *buf,
                                                        int count,
                                                        MPI_Datatype datatype,
                                                        int rank,
                                                        int tag,
                                                        MPIR_Comm * comm,
                                                        int context_offset, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_SSEND_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_SSEND_INIT);
    MPIDI_POSIX_REQUEST_CREATE_SREQ(sreq);
    MPIR_Object_set_ref(sreq, 1);
    sreq->kind = MPIR_REQUEST_KIND__PREQUEST_SEND;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIDI_POSIX_ENVELOPE_SET(MPIDI_POSIX_REQUEST(sreq), comm->rank, tag,
                             comm->context_id + context_offset);
    MPIDI_POSIX_REQUEST(sreq)->user_buf = (char *) buf;
    MPIDI_POSIX_REQUEST(sreq)->user_count = count;
    MPIDI_POSIX_REQUEST(sreq)->dest = rank;
    MPIDI_POSIX_REQUEST(sreq)->datatype = datatype;
    MPIDI_POSIX_REQUEST(sreq)->type = MPIDI_POSIX_TYPESYNC;
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_SSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_bsend_init(const void *buf,
                                                        int count,
                                                        MPI_Datatype datatype,
                                                        int rank,
                                                        int tag,
                                                        MPIR_Comm * comm,
                                                        int context_offset, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_BSEND_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_BSEND_INIT);
    MPIDI_POSIX_REQUEST_CREATE_SREQ(sreq);
    MPIR_Object_set_ref(sreq, 1);
    sreq->kind = MPIR_REQUEST_KIND__PREQUEST_SEND;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIDI_POSIX_ENVELOPE_SET(MPIDI_POSIX_REQUEST(sreq), comm->rank, tag,
                             comm->context_id + context_offset);
    MPIDI_POSIX_REQUEST(sreq)->user_buf = (char *) buf;
    MPIDI_POSIX_REQUEST(sreq)->user_count = count;
    MPIDI_POSIX_REQUEST(sreq)->dest = rank;
    MPIDI_POSIX_REQUEST(sreq)->datatype = datatype;
    MPIDI_POSIX_REQUEST(sreq)->type = MPIDI_POSIX_TYPEBUFFERED;
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_BSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_rsend_init(const void *buf,
                                                        int count,
                                                        MPI_Datatype datatype,
                                                        int rank,
                                                        int tag,
                                                        MPIR_Comm * comm,
                                                        int context_offset, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_RSEND_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_RSEND_INIT);
    MPIDI_POSIX_REQUEST_CREATE_SREQ(sreq);
    MPIR_Object_set_ref(sreq, 1);
    MPIR_cc_set(&(sreq)->cc, 0);
    sreq->kind = MPIR_REQUEST_KIND__PREQUEST_SEND;
    sreq->comm = comm;
    MPIR_Comm_add_ref(comm);
    MPIDI_POSIX_ENVELOPE_SET(MPIDI_POSIX_REQUEST(sreq), comm->rank, tag,
                             comm->context_id + context_offset);
    MPIDI_POSIX_REQUEST(sreq)->user_buf = (char *) buf;
    MPIDI_POSIX_REQUEST(sreq)->user_count = count;
    MPIDI_POSIX_REQUEST(sreq)->dest = rank;
    MPIDI_POSIX_REQUEST(sreq)->datatype = datatype;
    MPIDI_POSIX_REQUEST(sreq)->type = MPIDI_POSIX_TYPEREADY;
    *request = sreq;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_RSEND_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FCNAME
#define FCNAME MPL_QUOTE(MPIDI_POSIX_mpi_isend)
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_isend(const void *buf, MPI_Aint count,
                                                   MPI_Datatype datatype, int rank, int tag,
                                                   MPIR_Comm * comm, int context_offset,
                                                   MPIDI_av_entry_t * av, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ISEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ISEND);
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
    mpi_errno =
        MPIDI_POSIX_do_isend(buf, count, datatype, rank, tag, comm, context_offset, request,
                             MPIDI_POSIX_TYPESTANDARD);

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ISEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_issend(const void *buf, MPI_Aint count,
                                                    MPI_Datatype datatype, int rank,
                                                    int tag, MPIR_Comm * comm,
                                                    int context_offset, MPIDI_av_entry_t * av,
                                                    MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ISSEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ISSEND);
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);

    mpi_errno =
        MPIDI_POSIX_do_isend(buf, count, datatype, rank, tag, comm, context_offset, request,
                             MPIDI_POSIX_TYPESYNC);

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ISSEND);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_mpi_cancel_send(MPIR_Request * sreq)
{
    MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_Request *req = MPIDI_POSIX_sendq.head;
    MPIR_Request *prev_req = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_CANCEL_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_CANCEL_SEND);

    while (req) {
        if (req == sreq) {
            MPIR_STATUS_SET_CANCEL_BIT(sreq->status, TRUE);
            MPIR_STATUS_SET_COUNT(sreq->status, 0);
            MPIDI_POSIX_REQUEST_COMPLETE(sreq);
            MPIDI_POSIX_REQUEST_DEQUEUE_AND_SET_ERROR(&sreq, prev_req, MPIDI_POSIX_sendq,
                                                      mpi_errno);
            break;
        }

        prev_req = req;
        req = MPIDI_POSIX_REQUEST(req)->next;
    }

    MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_CANCEL_SEND);
    return mpi_errno;
}

#endif /* POSIX_SEND_H_INCLUDED */
