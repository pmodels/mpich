/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#define _GNU_SOURCE
#include "mpidimpl.h"
#include "ib_impl.h"
#include "ib_device.h"
#include "ib_cm.h"
#include "ib_utils.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_send (MPIDI_VC_t *vc, 
        MPID_nem_cell_ptr_t cell, 
        int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    int ret, i;
    MPID_nem_ib_cm_remote_id_ptr_t r_info;
    MPID_nem_ib_queue_elem_t *vce;
    MPID_nem_ib_queue_elem_t *sqe;
    MPID_nem_ib_cell_elem_t *ce;

    /* Check if VC is connected already */

    if(MPID_NEM_IB_CONN_RC == VC_FIELD(vc, conn_status)) {

        /* Process queued sends */
        while(!MPID_nem_ib_queue_empty(
                    (MPID_nem_ib_queue_t *) 
                    VC_FIELD(vc, ib_send_queue)) && 
                VC_FIELD(vc, avail_send_wqes)) {

            MPID_nem_ib_queue_dequeue(
                    (MPID_nem_ib_queue_t *) VC_FIELD(vc, ib_send_queue),
                    &sqe);

            ce = (MPID_nem_ib_cell_elem_t *) sqe->data;

            NEM_IB_DBG("Cell to send %p, Len %d",
                    ce->nem_cell, ce->datalen);

            ret = MPID_nem_ib_post_send(VC_FIELD(vc, qp),
                    &ce->desc.u.s_wr);

            MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, 
                    "**ibv_post_send", "**ibv_post_send %d", ret);

            VC_FIELD(vc, avail_send_wqes)--;
        }

        mpi_errno = MPID_nem_ib_get_cell(&ce);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        ce->nem_cell = cell;
        ce->datalen = datalen;
        ce->vc = vc;

        MPID_nem_ib_prep_cell_send(ce, 
                (void *) MPID_NEM_CELL_TO_PACKET(cell),
                (uint32_t) (datalen + MPID_NEM_MPICH2_HEAD_LEN));

        if(VC_FIELD(vc, avail_send_wqes)) {
            /* This means that the pending queue
             * was emptied */

            ret = MPID_nem_ib_post_send(VC_FIELD(vc, qp),
                    &ce->desc.u.s_wr);

            MPIU_ERR_CHKANDJUMP1(ret != 0, mpi_errno, MPI_ERR_OTHER, 
                    "**ibv_post_send", "**ibv_post_send %d", ret);

            VC_FIELD(vc, avail_send_wqes)--;

            /* Is SRQ refill required? */
            if(MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted <
                    MPID_nem_ib_dev_param_ptr->srq_n_preserve + 1) {

                MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted += 
                    MPID_nem_ib_refill_srq(
                            MPID_nem_ib_ctxt_ptr->ib_dev[0].srq, 1);
            }

        } else {

            if(!VC_FIELD(vc, in_queue)) {

                MPID_nem_ib_queue_alloc(
                        MPID_nem_ib_vc_queue, &vce);

                vce->data = vc;

                VC_FIELD(vc, in_queue) = 1;

                MPID_nem_ib_queue_enqueue(
                        MPID_nem_ib_vc_queue, vce);

            }

            /* Add this to the channel send queue */
            MPID_nem_ib_queue_alloc(
                    (MPID_nem_ib_queue_t *)(VC_FIELD(vc, ib_send_queue)), &sqe);

            sqe->data = (void *) ce;

            MPID_nem_ib_queue_enqueue(
                    (MPID_nem_ib_queue_t *)VC_FIELD(vc, ib_send_queue), sqe);
        }

    } else if(MPID_NEM_IB_CONN_IN_PROGRESS == VC_FIELD(vc, conn_status)) {

        /* Connection already in progress,
         * just enqueue the message and be
         * done with it */

        mpi_errno = MPID_nem_ib_get_cell(&ce);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        ce->nem_cell = cell;
        ce->datalen = datalen + MPID_NEM_MPICH2_HEAD_LEN;
        ce->vc = vc;

        MPID_nem_ib_prep_cell_send(ce, 
                (void *) MPID_NEM_CELL_TO_PACKET(cell),
                (uint32_t) (datalen + MPID_NEM_MPICH2_HEAD_LEN));

        /* Add this to the channel send queue */
        MPID_nem_ib_queue_alloc(
                (MPID_nem_ib_queue_t *)(VC_FIELD(vc, ib_send_queue)), &sqe);

        sqe->data = (void *) ce;

        /* The connection state MPID_NEM_IB_CONN_IN_PROGRESS
         * can only be set when "this" process started a
         * connection process. That means "this" vc was already
         * put in a queue and will be processed later by
         * the progress engine. So, there is no need to put
         * the VC into the vc_queue, rather just appending
         * the request to the send_queue is enough, for now */

        MPIU_Assert(VC_FIELD(vc, in_queue) == 1);

        MPID_nem_ib_queue_enqueue(
                (MPID_nem_ib_queue_t *)VC_FIELD(vc, ib_send_queue), sqe);

    } else {

        /* No connection and none in progress.
         * Initiate a connection and queue
         * the message */

        NEM_IB_DBG("No connection to %d, GUID %lu, UD %u, Cell %p, Len %d", 
                vc->pg_rank, VC_FIELD(vc, node_guid), VC_FIELD(vc, ud_qpn),
                cell, datalen + MPID_NEM_MPICH2_HEAD_LEN);

        r_info = (MPID_nem_ib_cm_remote_id_ptr_t) 
            MPID_nem_ib_lookup_hash_table(
                &MPID_nem_ib_cm_ctxt_ptr->hash_table,
                VC_FIELD(vc, node_guid), VC_FIELD(vc, ud_qpn));

        MPIU_Assert(NULL != r_info);

        mpi_errno = MPID_nem_ib_cm_start_connect(r_info, vc);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        MPID_nem_ib_queue_alloc(
                MPID_nem_ib_vc_queue, &vce);

        vce->data = vc;

        MPIU_Assert(VC_FIELD(vc, in_queue) == 0);

        VC_FIELD(vc, in_queue) = 1;

        MPID_nem_ib_queue_enqueue(
                MPID_nem_ib_vc_queue, vce);

        /* Get an "cell" from our pool */

        mpi_errno = MPID_nem_ib_get_cell(&ce);

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }

        ce->nem_cell = cell;
        ce->datalen = datalen + MPID_NEM_MPICH2_HEAD_LEN;
        ce->vc = vc;

        MPID_nem_ib_prep_cell_send(ce, 
                (void *) MPID_NEM_CELL_TO_PACKET(cell),
                (uint32_t) (datalen + MPID_NEM_MPICH2_HEAD_LEN));

        /* Add this to the channel send queue */
        MPID_nem_ib_queue_alloc(
                (MPID_nem_ib_queue_t *)VC_FIELD(vc, ib_send_queue), &sqe);

        sqe->data = (void *) ce;

        MPID_nem_ib_queue_enqueue(
                (MPID_nem_ib_queue_t *)VC_FIELD(vc, ib_send_queue), sqe);

    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
