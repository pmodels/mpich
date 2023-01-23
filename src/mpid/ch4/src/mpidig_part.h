/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_PART_H_INCLUDED
#define MPIDIG_AM_PART_H_INCLUDED

#include <stdio.h>
#include "ch4_impl.h"
#include "ch4_send.h"
#include "mpidig_part_utils.h"
#include "mpidpre.h"
#include "mpir_request.h"

int MPIDIG_mpi_psend_init(const void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int dest, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request);
int MPIDIG_mpi_precv_init(void *buf, int partitions, MPI_Aint count,
                          MPI_Datatype datatype, int source, int tag,
                          MPIR_Comm * comm, MPIR_Info * info, MPIR_Request ** request);

MPL_STATIC_INLINE_PREFIX int MPIDIG_part_start(MPIR_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    MPIR_Assert(!MPIR_Part_request_is_active(request));

    if (request->kind == MPIR_REQUEST_KIND__PART_SEND) {
        /* cc_ptr > 0 indicate data transfer has started and will be completed when cc_ptr = 0
         * the counter depends on the total number of messages, which is know upon reception of the CTS
         * if we have not received the first CTS yet (aka the value of msg_part == -1) we set a temp value of 1*/
        const int msg_part = MPIDIG_PART_REQUEST(request, u.send.msg_part);
        MPIR_cc_set(request->cc_ptr, MPL_MAX(1, msg_part));

        /* we have to reset information for the current iteration.
         * The reset is done in the CTS reception callback as well but no msgs has been sent
         * so it's safe to overwrite it*/
        if (MPIDIG_PART_REQUEST(request, do_tag)) {
            MPIR_cc_set(&MPIDIG_PART_REQUEST(request, u.send.cc_send), msg_part);
        }
    } else {
        /* cc_ptr > 0 indicate data transfer starts and will be completed when cc_ptr = 0
         * the counter is set to the max of 1 (to avoid early completion) and the number of msg parts
         * that will actually be sent if we have already matched (-1 if not)*/
        MPIR_cc_set(request->cc_ptr, MPL_MAX(1, MPIDIG_PART_REQUEST(request, u.recv.msg_part)));

        /* if the peer_req_ptr is filled the request has been matched
         * we can use the pointer to check matching status as the pointer is always written inside a lock section */
        const bool is_matched = MPIDIG_Part_rreq_status_has_matched(request);
        if (is_matched) {
            MPIR_Assert(MPIDIG_PART_REQUEST(request, u.recv.msg_part) >= 0);
            MPIDIG_part_rreq_reset_cc_part(request);

            const bool do_tag = MPIDIG_PART_REQUEST(request, do_tag);
            if (do_tag) {
                /* in tag matching we issue the recv requests */
                mpi_errno = MPIDIG_part_issue_recv(request);
                MPIR_ERR_CHECK(mpi_errno);
                /* we need to issue the CTS at last to ensure we are fully ready
                 * done only the first time for tag-matching*/
                const bool first_cts = MPIDIG_Part_rreq_status_has_first_cts(request);
                if (!first_cts) {
                    mpi_errno = MPIDIG_part_issue_cts(request);
                    MPIR_ERR_CHECK(mpi_errno);
                    MPIDIG_Part_rreq_status_first_cts(request);
                }
            } else {
                mpi_errno = MPIDIG_part_issue_cts(request);
                MPIR_ERR_CHECK(mpi_errno);
                const bool first_cts = MPIDIG_Part_rreq_status_has_first_cts(request);
                if (!first_cts) {
                    MPIDIG_Part_rreq_status_first_cts(request);
                }
            }
        }
    }

    /* activate must be last to notify that everything has been done */
    MPIR_Part_request_activate(request);

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_pready_range(int p_low, int p_high,
                                                     MPIR_Request * part_sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_Assert(MPIR_Part_request_is_active(part_sreq));
    MPIR_Assert(part_sreq->kind == MPIR_REQUEST_KIND__PART_SEND);

    const int n_part = part_sreq->u.part.partitions;
    MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(part_sreq, u.send.cc_part);

    /* for each partition mark it as ready and start them if we can */
    for (int i = p_low; i <= p_high; ++i) {
        int incomplete;
        MPIR_cc_decr(&cc_part[i], &incomplete);

        /* send the partition if matched and is complete, try to send the msg */
        if (!incomplete) {
            const int msg_part = MPIDIG_PART_REQUEST(part_sreq, u.send.msg_part);
            MPIR_Assert(msg_part >= 0);

            // TODO change the VCI here
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
            const int msg_lb = MPIDIG_part_idx_lb(i, n_part, msg_part);
            const int msg_ub = MPIDIG_part_idx_ub(i, n_part, msg_part);
            mpi_errno =
                MPIDIG_part_issue_msg_if_ready(msg_lb, msg_ub, part_sreq, MPIDIG_PART_REGULAR);
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            const bool do_tag = MPIDIG_PART_REQUEST(part_sreq, do_tag);
            if (!do_tag) {
                /* if it's not matched or not complete then we miss the CTS (first one or not)
                 * so we poke progress and hopefully get it */
                MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
                mpi_errno = MPIDI_progress_test_vci(0);
                MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_pready_list(int length, const int array_of_partitions[],
                                                    MPIR_Request * part_sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;
    MPIR_Assert(MPIR_Part_request_is_active(part_sreq));
    MPIR_Assert(part_sreq->kind == MPIR_REQUEST_KIND__PART_SEND);

    const int n_part = part_sreq->u.part.partitions;
    MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(part_sreq, u.send.cc_part);
    for (int i = 0; i < length; i++) {
        const int ipart = array_of_partitions[i];
        // mark the partition as ready
        int incomplete;
        MPIR_cc_decr(&cc_part[ipart], &incomplete);

        /* send the partition if matched and is complete, try to send the msg */
        if (!incomplete) {
            const int msg_part = MPIDIG_PART_REQUEST(part_sreq, u.send.msg_part);
            MPIR_Assert(msg_part >= 0);

            const int msg_lb = MPIDIG_part_idx_lb(ipart, n_part, msg_part);
            const int msg_ub = MPIDIG_part_idx_ub(ipart, n_part, msg_part);
            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
            mpi_errno =
                MPIDIG_part_issue_msg_if_ready(msg_lb, msg_ub, part_sreq, MPIDIG_PART_REGULAR);
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            const bool do_tag = MPIDIG_PART_REQUEST(part_sreq, do_tag);
            if (!do_tag) {
                /* if it's not matched or not complete then we miss the CTS
                 * so we poke progress and hopefully get it
                 * if we receive the CTS then we will proceed to the send there*/
                MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
                mpi_errno = MPIDI_progress_test_vci(0);
                MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDIG_mpi_parrived(MPIR_Request * request, int partition, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    /* we must be active to call p_arrived */
    MPIR_Assert(MPIR_Part_request_is_active(request));
    MPIR_Assert(request->kind == MPIR_REQUEST_KIND__PART_RECV);

    /* to be ready to look for answers we must have matched AND started the first CTS */
    if (MPIDIG_Part_rreq_status_has_first_cts(request)) {
        // get the span of send request to check from start_part to end_part (included!)
        const int msg_part = MPIDIG_PART_REQUEST(request, u.recv.msg_part);
        const int n_part = request->u.part.partitions;
        const int start_msg = MPIDIG_part_idx_lb(partition, n_part, msg_part);
        const int end_msg = MPIDIG_part_idx_ub(partition, n_part, msg_part);
        MPIR_Assert(start_msg >= 0);
        MPIR_Assert(end_msg >= 0);
        MPIR_Assert(start_msg <= msg_part);
        MPIR_Assert(end_msg <= msg_part);

        /* it's safe to check do_tag here because we have matched */
        const bool do_tag = MPIDIG_PART_REQUEST(request, do_tag);

        /* get the number of partitions that are still busy. if the sum is 0 then they have all finished */
        int is_busy = 0;
        if (do_tag) {
            MPIR_Request **child_req = MPIDIG_PART_REQUEST(request, tag_req_ptr);
            for (int ip = start_msg; ip < end_msg; ++ip) {
                is_busy += !(MPIR_Request_is_complete(child_req[ip]));
            }
        } else {
            MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(request, u.recv.cc_part);
            for (int ip = start_msg; ip < end_msg; ++ip) {
                is_busy += MPIR_cc_get(cc_part[ip]);
            }
        }
        /* if none of the parts are busy then we can set to true and return */
        if (!is_busy) {
            *flag = TRUE;
            goto fn_exit;
        }
    }
    *flag = FALSE;
    /* Trigger progress to process AM packages in case wait with parrived in a loop.
     * also if we don't have the CTS yet we have to receive it*/
    MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
    mpi_errno = MPIDI_progress_test_vci(0);
    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDIG_AM_PART_H_INCLUDED */
