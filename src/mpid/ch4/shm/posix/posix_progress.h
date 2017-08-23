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
#ifndef POSIX_PROGRESS_H_INCLUDED
#define POSIX_PROGRESS_H_INCLUDED

#include "posix_impl.h"

/* ----------------------------------------------------- */
/* MPIDI_POSIX_progress_recv                     */
/* ----------------------------------------------------- */
#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_progress_recv)
static inline int MPIDI_POSIX_progress_recv(int blocking, int *completion_count)
{
    int mpi_errno = MPI_SUCCESS;
    size_t data_sz;
    int in_cell = 0;
    MPIDI_POSIX_cell_ptr_t cell = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_PROGRESS_RECV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_PROGRESS_RECV);
    /* try to match with unexpected */
    MPIR_Request *sreq = MPIDI_POSIX_recvq_unexpected.head;
    MPIR_Request *prev_sreq = NULL;
  unexpected_l:

    if (sreq != NULL) {
        goto match_l;
    }

    /* try to receive from recvq */
    if (MPIDI_POSIX_mem_region.my_recvQ &&
        !MPIDI_POSIX_queue_empty(MPIDI_POSIX_mem_region.my_recvQ)) {
        MPIDI_POSIX_queue_dequeue(MPIDI_POSIX_mem_region.my_recvQ, &cell);
        in_cell = 1;
        goto match_l;
    }

    goto fn_exit;
  match_l:{
        /* traverse posted receive queue */
        MPIR_Request *req = MPIDI_POSIX_recvq_posted.head;
        MPIR_Request *prev_req = NULL;
        int continue_matching = 1;
        char *send_buffer =
            in_cell ? (char *) cell->pkt.mpich.p.payload : (char *) MPIDI_POSIX_REQUEST(sreq)->
            user_buf;
        int type = in_cell ? cell->pkt.mpich.type : MPIDI_POSIX_REQUEST(sreq)->type;
        MPIR_Request *pending = in_cell ? cell->pending : MPIDI_POSIX_REQUEST(sreq)->pending;

        if (type == MPIDI_POSIX_TYPEACK) {
            /* ACK message doesn't have a matching receive! */
            int c;
            MPIR_Assert(in_cell);
            MPIR_Assert(pending);
            MPIR_cc_decr(pending->cc_ptr, &c);
            MPIR_Request_free(pending);
            goto release_cell_l;
        }

        while (req) {
            int sender_rank, tag, context_id;
            MPI_Count count;
            MPIDI_POSIX_ENVELOPE_GET(MPIDI_POSIX_REQUEST(req), sender_rank, tag, context_id);
            MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL,
                            (MPL_DBG_FDEST, "Posted from grank %d to %d in progress %d,%d,%d\n",
                             MPIDI_CH4U_rank_to_lpid(sender_rank, req->comm),
                             MPIDI_POSIX_mem_region.rank, sender_rank, tag, context_id));

            if ((in_cell && MPIDI_POSIX_ENVELOPE_MATCH(cell, sender_rank, tag, context_id)) ||
                (sreq &&
                 MPIDI_POSIX_ENVELOPE_MATCH(MPIDI_POSIX_REQUEST(sreq), sender_rank, tag,
                                            context_id))) {

                /* Request matched */

                continue_matching = 1;

                if (MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req)) {
                    MPIDI_CH4R_anysource_matched(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req),
                                                 MPIDI_CH4R_SHM, &continue_matching);
                    MPIR_Request_free(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req));

                    /* Decouple requests */
                    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req))
                        = NULL;
                    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req) = NULL;

                    if (continue_matching)
                        break;
                }

                char *recv_buffer = (char *) MPIDI_POSIX_REQUEST(req)->user_buf;

                if (pending) {
                    /* we must send ACK */
                    int srank = in_cell ? cell->rank : sreq->status.MPI_SOURCE;
                    MPIR_Request *req_ack = NULL;
                    MPIDI_POSIX_REQUEST_CREATE_SREQ(req_ack);
                    MPIR_Object_set_ref(req_ack, 1);
                    req_ack->comm = req->comm;
                    MPIR_Comm_add_ref(req->comm);

                    MPIDI_POSIX_ENVELOPE_SET(MPIDI_POSIX_REQUEST(req_ack), req->comm->rank, tag,
                                             context_id);
                    MPIDI_POSIX_REQUEST(req_ack)->user_buf = NULL;
                    MPIDI_POSIX_REQUEST(req_ack)->user_count = 0;
                    MPIDI_POSIX_REQUEST(req_ack)->datatype = MPI_BYTE;
                    MPIDI_POSIX_REQUEST(req_ack)->data_sz = 0;
                    MPIDI_POSIX_REQUEST(req_ack)->type = MPIDI_POSIX_TYPEACK;
                    MPIDI_POSIX_REQUEST(req_ack)->dest = srank;
                    MPIDI_POSIX_REQUEST(req_ack)->next = NULL;
                    MPIDI_POSIX_REQUEST(req_ack)->segment_ptr = NULL;
                    MPIDI_POSIX_REQUEST(req_ack)->pending = pending;
                    /* enqueue req_ack */
                    MPIDI_POSIX_REQUEST_ENQUEUE(req_ack, MPIDI_POSIX_sendq);
                }

                if (type == MPIDI_POSIX_TYPEEAGER)
                    /* eager message */
                    data_sz =
                        in_cell ? cell->pkt.mpich.datalen : MPIDI_POSIX_REQUEST(sreq)->data_sz;
                else if (type == MPIDI_POSIX_TYPELMT)
                    data_sz = MPIDI_POSIX_EAGER_THRESHOLD;
                else {
                    data_sz = 0;        /*  unused warning */
                    MPIR_Assert(0);
                }
                /* check for user buffer overflow */
                size_t user_data_sz = MPIDI_POSIX_REQUEST(req)->data_sz;
                if (user_data_sz < data_sz) {
                    req->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                    data_sz = user_data_sz;
                }

                /* copy to user buffer */
                if (MPIDI_POSIX_REQUEST(req)->segment_ptr) {
                    /* non-contig */
                    size_t last = MPIDI_POSIX_REQUEST(req)->segment_first + data_sz;
                    MPIR_Segment_unpack(MPIDI_POSIX_REQUEST(req)->segment_ptr,
                                        MPIDI_POSIX_REQUEST(req)->segment_first,
                                        (MPI_Aint *) & last, send_buffer);
                    if (last != MPIDI_POSIX_REQUEST(req)->segment_first + data_sz)
                        req->status.MPI_ERROR = MPI_ERR_TYPE;
                    if (type == MPIDI_POSIX_TYPEEAGER)
                        MPIR_Segment_free(MPIDI_POSIX_REQUEST(req)->segment_ptr);
                    else
                        MPIDI_POSIX_REQUEST(req)->segment_first = last;
                }
                else
                    /* contig */
                if (send_buffer)
                    MPIR_Memcpy(recv_buffer, (void *) send_buffer, data_sz);
                MPIDI_POSIX_REQUEST(req)->data_sz -= data_sz;
                MPIDI_POSIX_REQUEST(req)->user_buf += data_sz;

                /* set status and dequeue receive request if done */
                count = MPIR_STATUS_GET_COUNT(req->status) + (MPI_Count) data_sz;
                MPIR_STATUS_SET_COUNT(req->status, count);
                if (type == MPIDI_POSIX_TYPEEAGER) {
                    if (in_cell) {
                        req->status.MPI_SOURCE = cell->rank;
                        req->status.MPI_TAG = cell->tag;
                    }
                    else {
                        req->status.MPI_SOURCE = sreq->status.MPI_SOURCE;
                        req->status.MPI_TAG = sreq->status.MPI_TAG;
                    }
                    MPIDI_POSIX_REQUEST_DEQUEUE_AND_SET_ERROR(&req, prev_req,
                                                              MPIDI_POSIX_recvq_posted,
                                                              req->status.MPI_ERROR);
                }

                goto release_cell_l;
            }   /* if matched  */

            prev_req = req;
            req = MPIDI_POSIX_REQUEST(req)->next;
        }

        /* unexpected message, no posted matching req */
        if (in_cell) {
            /* free the cell, move to unexpected queue */
            MPIR_Request *rreq;
            MPIDI_POSIX_REQUEST_CREATE_RREQ(rreq);
            MPIR_Object_set_ref(rreq, 1);
            /* set status */
            rreq->status.MPI_SOURCE = cell->rank;
            rreq->status.MPI_TAG = cell->tag;
            MPIR_STATUS_SET_COUNT(rreq->status, cell->pkt.mpich.datalen);
            MPIDI_POSIX_ENVELOPE_SET(MPIDI_POSIX_REQUEST(rreq), cell->rank, cell->tag,
                                     cell->context_id);
            data_sz = cell->pkt.mpich.datalen;
            MPIDI_POSIX_REQUEST(rreq)->data_sz = data_sz;
            MPIDI_POSIX_REQUEST(rreq)->type = cell->pkt.mpich.type;

            if (data_sz > 0) {
                MPIDI_POSIX_REQUEST(rreq)->user_buf = (char *) MPL_malloc(data_sz);
                MPIR_Memcpy(MPIDI_POSIX_REQUEST(rreq)->user_buf, (void *) cell->pkt.mpich.p.payload,
                            data_sz);
            }
            else {
                MPIDI_POSIX_REQUEST(rreq)->user_buf = NULL;
            }

            MPIDI_POSIX_REQUEST(rreq)->datatype = MPI_BYTE;
            MPIDI_POSIX_REQUEST(rreq)->next = NULL;
            MPIDI_POSIX_REQUEST(rreq)->pending = cell->pending;
            /* enqueue rreq */
            MPIDI_POSIX_REQUEST_ENQUEUE(rreq, MPIDI_POSIX_recvq_unexpected);
            MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL,
                            (MPL_DBG_FDEST, "Unexpected from grank %d to %d in progress %d,%d,%d\n",
                             cell->my_rank, MPIDI_POSIX_mem_region.rank,
                             cell->rank, cell->tag, cell->context_id));
        }
        else {
            /* examine another message in unexpected queue */
            prev_sreq = sreq;
            sreq = MPIDI_POSIX_REQUEST(sreq)->next;
            goto unexpected_l;
        }
    }
  release_cell_l:

    if (in_cell) {
        /* release cell */
        MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL,
                        (MPL_DBG_FDEST, "Received from grank %d to %d in progress %d,%d,%d\n",
                         cell->my_rank, MPIDI_POSIX_mem_region.rank, cell->rank, cell->tag,
                         cell->context_id));
        cell->pending = NULL;
        {
            MPIDI_POSIX_queue_enqueue(MPIDI_POSIX_mem_region.FreeQ[cell->my_rank], cell);
        }
    }
    else {
        /* destroy unexpected req */
        MPIDI_POSIX_REQUEST(sreq)->pending = NULL;
        MPL_free(MPIDI_POSIX_REQUEST(sreq)->user_buf);
        MPIDI_POSIX_REQUEST_DEQUEUE_AND_SET_ERROR(&sreq, prev_sreq, MPIDI_POSIX_recvq_unexpected,
                                                  mpi_errno);
    }

    (*completion_count)++;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_PROGRESS_RECV);
    return mpi_errno;
}

/* ----------------------------------------------------- */
/* MPIDI_POSIX_progress_send                     */
/* ----------------------------------------------------- */
#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_progress_send)
static inline int MPIDI_POSIX_progress_send(int blocking, int *completion_count)
{
    int mpi_errno = MPI_SUCCESS;
    int dest;
    MPIDI_POSIX_cell_ptr_t cell = NULL;
    MPIR_Request *sreq = MPIDI_POSIX_sendq.head;
    MPIR_Request *prev_sreq = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_DO_PROGRESS_SEND);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_DO_PROGRESS_SEND);

    if (sreq == NULL)
        goto fn_exit;

    /* try to send via freeq */
    if (!MPIDI_POSIX_queue_empty(MPIDI_POSIX_mem_region.my_freeQ)) {
        MPIDI_POSIX_queue_dequeue(MPIDI_POSIX_mem_region.my_freeQ, &cell);
        MPIDI_POSIX_ENVELOPE_GET(MPIDI_POSIX_REQUEST(sreq), cell->rank, cell->tag,
                                 cell->context_id);
        dest = MPIDI_POSIX_REQUEST(sreq)->dest;
        char *recv_buffer = (char *) cell->pkt.mpich.p.payload;
        size_t data_sz = MPIDI_POSIX_REQUEST(sreq)->data_sz;
        /*
         * TODO: make request field dest_lpid (or even recvQ[dest_lpid]) instead of dest - no need to do rank_to_lpid each time
         */
        int grank = MPIDI_CH4U_rank_to_lpid(dest, sreq->comm);
        cell->pending = NULL;

        if (MPIDI_POSIX_REQUEST(sreq)->type == MPIDI_POSIX_TYPESYNC) {
            /* increase req cc in order to release req only after ACK, do it once per SYNC request */
            /* non-NULL pending req signal receiver about sending ACK back */
            /* the pending req should be sent back for sender to decrease cc, for it is dequeued already */
            int c;
            cell->pending = sreq;
            MPIR_cc_incr(sreq->cc_ptr, &c);
            MPIDI_POSIX_REQUEST(sreq)->type = MPIDI_POSIX_TYPESTANDARD;
        }

        if (data_sz <= MPIDI_POSIX_EAGER_THRESHOLD) {
            cell->pkt.mpich.datalen = data_sz;

            if (MPIDI_POSIX_REQUEST(sreq)->type == MPIDI_POSIX_TYPEACK) {
                cell->pkt.mpich.type = MPIDI_POSIX_TYPEACK;
                cell->pending = MPIDI_POSIX_REQUEST(sreq)->pending;
            }
            else {
                /* eager message */
                if (MPIDI_POSIX_REQUEST(sreq)->segment_ptr) {
                    /* non-contig */
                    MPIR_Segment_pack(MPIDI_POSIX_REQUEST(sreq)->segment_ptr,
                                      MPIDI_POSIX_REQUEST(sreq)->segment_first,
                                      (MPI_Aint *) & MPIDI_POSIX_REQUEST(sreq)->segment_size,
                                      recv_buffer);
                    MPIR_Segment_free(MPIDI_POSIX_REQUEST(sreq)->segment_ptr);
                }
                else {
                    /* contig */
                    MPIR_Memcpy((void *) recv_buffer, MPIDI_POSIX_REQUEST(sreq)->user_buf, data_sz);
                }

                cell->pkt.mpich.type = MPIDI_POSIX_TYPEEAGER;
                /* set status */
                /*
                 * TODO: incorrect count for LMT - set to a last chunk of data
                 * is send status required?
                 */
                sreq->status.MPI_SOURCE = cell->rank;
                sreq->status.MPI_TAG = cell->tag;
                MPIR_STATUS_SET_COUNT(sreq->status, data_sz);
            }

            /* dequeue sreq */
            MPIDI_POSIX_REQUEST_DEQUEUE_AND_SET_ERROR(&sreq, prev_sreq, MPIDI_POSIX_sendq,
                                                      mpi_errno);
        }
        else {
            /* long message */
            if (MPIDI_POSIX_REQUEST(sreq)->segment_ptr) {
                /* non-contig */
                size_t last =
                    MPIDI_POSIX_REQUEST(sreq)->segment_first + MPIDI_POSIX_EAGER_THRESHOLD;
                MPIR_Segment_pack(MPIDI_POSIX_REQUEST(sreq)->segment_ptr,
                                  MPIDI_POSIX_REQUEST(sreq)->segment_first, (MPI_Aint *) & last,
                                  recv_buffer);
                MPIDI_POSIX_REQUEST(sreq)->segment_first = last;
            }
            else {
                /* contig */
                MPIR_Memcpy((void *) recv_buffer, MPIDI_POSIX_REQUEST(sreq)->user_buf,
                            MPIDI_POSIX_EAGER_THRESHOLD);
                MPIDI_POSIX_REQUEST(sreq)->user_buf += MPIDI_POSIX_EAGER_THRESHOLD;
            }

            cell->pkt.mpich.datalen = MPIDI_POSIX_EAGER_THRESHOLD;
            MPIDI_POSIX_REQUEST(sreq)->data_sz -= MPIDI_POSIX_EAGER_THRESHOLD;
            cell->pkt.mpich.type = MPIDI_POSIX_TYPELMT;
        }

        MPL_DBG_MSG_FMT(MPIR_DBG_HANDLE, TYPICAL,
                        (MPL_DBG_FDEST, "Sent to grank %d from %d in progress %d,%d,%d\n", grank,
                         cell->my_rank, cell->rank, cell->tag, cell->context_id));
        MPIDI_POSIX_queue_enqueue(MPIDI_POSIX_mem_region.RecvQ[grank], cell);
        (*completion_count)++;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_DO_PROGRESS_SEND);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPIDI_POSIX_progress)
static inline int MPIDI_POSIX_progress(int blocking)
{
    int complete = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_PROGRESS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_PROGRESS);

    do {
        /* Receieve progress */
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
        MPIDI_POSIX_progress_recv(blocking, &complete);
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);
        /* Send progress */
        MPID_THREAD_CS_ENTER(POBJ, MPIDI_POSIX_SHM_MUTEX);
        MPIDI_POSIX_progress_send(blocking, &complete);
        MPID_THREAD_CS_EXIT(POBJ, MPIDI_POSIX_SHM_MUTEX);

        if (complete > 0)
            break;
    } while (blocking);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_PROGRESS);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_progress_test(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_progress_poke(void)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline void MPIDI_POSIX_progress_start(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

static inline void MPIDI_POSIX_progress_end(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return;
}

static inline int MPIDI_POSIX_progress_wait(MPID_Progress_state * state)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_progress_register(int (*progress_fn) (int *))
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_progress_deregister(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_progress_activate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

static inline int MPIDI_POSIX_progress_deactivate(int id)
{
    MPIR_Assert(0);
    return MPI_SUCCESS;
}

#endif /* POSIX_PROGRESS_H_INCLUDED */
