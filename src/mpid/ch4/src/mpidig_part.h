/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDIG_AM_PART_H_INCLUDED
#define MPIDIG_AM_PART_H_INCLUDED

#include "ch4_impl.h"
#include "mpidig_part_utils.h"

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

    /* activated request cannot be deleted unless a wait/test has been called */
    MPIR_Part_request_activate(request);

    /* No need to increase refcnt for comm and datatype objects,
     * because it is erroneous to free an active partitioned req if it is not complete.*/
    if (request->kind == MPIR_REQUEST_KIND__PART_SEND) {
        /* cc_ptr > 0 indicate data transfer has started and will be completed when cc_ptr = 0
         * the counter depends on the total number of messages, which is know upon reception of the CTS
         * if we have not received the first CTS yet (aka the value of msg_part == -1) we set a temp value of 1*/
        MPIR_cc_set(request->cc_ptr, MPL_MAX(1, MPIDIG_PART_REQUEST(request, u.send.msg_part)));
    } else {
        /* cc_ptr > 0 indicate data transfer starts and will be completed when cc_ptr = 0
         * the counter is set to 1 temporarily and will be overwritten to the number of
         * partitions on the sender side */
        MPIR_cc_set(request->cc_ptr, 1);

        /* if the peer_req_ptr is filled the request has been matched
         * we can use the pointer to check matching status as the pointer is always written inside a lock section */
        const bool is_matched = MPIDIG_PART_REQUEST(request, peer_req_ptr);
        if (is_matched) {
            MPIDIG_part_rreq_reset_cc_part(request);

            /* cc_ptr > 0 indicate data transfer has started and will be completed when cc_ptr = 0
             * the counter is set to the total number of msgs and decreased at completion of the child's requests
             */
            const int msg_part = MPIDIG_PART_REQUEST(request, u.recv.msg_part);
            MPIR_Assert(msg_part >= 0);
            MPIR_cc_set(request->cc_ptr, msg_part);

            /* issue the CTS is last to ensure we are fully ready */
            mpi_errno = MPIDIG_part_issue_cts(request);
        }
    }

    MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    MPIR_FUNC_EXIT;
    return mpi_errno;
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

        /* is complete, try to send the msg */
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

            MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
            const int msg_lb = MPIDIG_part_idx_lb(ipart, n_part, msg_part);
            const int msg_ub = MPIDIG_part_idx_ub(ipart, n_part, msg_part);
            mpi_errno =
                MPIDIG_part_issue_msg_if_ready(msg_lb, msg_ub, part_sreq, MPIDIG_PART_REGULAR);
            MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
            MPIR_ERR_CHECK(mpi_errno);
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

    MPIR_Assert(MPIR_Part_request_is_active(request));
    MPIR_Assert(request->kind == MPIR_REQUEST_KIND__PART_RECV);
    const MPIR_cc_t *cc_part = MPIDIG_PART_REQUEST(request, u.recv.cc_part);

    // if the request has not matched or it's too early
    const bool too_early = cc_part == NULL || !MPIR_Part_request_is_active(request);

    // get the span of send request to check from start_part to end_part (included!)
    const int msg_part = too_early ? (0) : MPIDIG_PART_REQUEST(request, u.recv.msg_part);
    const int n_part = request->u.part.partitions;
    const int start_msg = MPIDIG_part_idx_lb(partition, n_part, msg_part);
    const int end_msg = MPIDIG_part_idx_ub(partition, n_part, msg_part);
    MPIR_Assert(start_msg >= 0);
    MPIR_Assert(end_msg >= 0);
    MPIR_Assert(start_msg <= msg_part);
    MPIR_Assert(end_msg <= msg_part);

    // get the number of partitions that are still busy. if the sum is 0 then they have all finished
    int is_busy = (too_early);
    for (int ip = start_msg; ip < end_msg; ++ip) {
        is_busy += MPIR_cc_get(cc_part[ip]);
    }
    if (!is_busy) {
        *flag = TRUE;
    } else {
        *flag = FALSE;
        /* Trigger progress to process AM packages in case wait with parrived in a loop. */
        // TODO: is it correct to lock the VCI that late in the process?
        MPID_THREAD_CS_ENTER(VCI, MPIDI_VCI(0).lock);
        mpi_errno = MPIDI_progress_test_vci(0);
        MPIR_ERR_CHECK(mpi_errno);
        MPID_THREAD_CS_EXIT(VCI, MPIDI_VCI(0).lock);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
#endif /* MPIDIG_AM_PART_H_INCLUDED */
