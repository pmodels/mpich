/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#define _GNU_SOURCE
#include "mpidimpl.h"
#include "mpid_nem_impl.h"
#include "ib_impl.h"
#include "ib_device.h"
#include "ib_cm.h"
#include "ib_utils.h"

uint32_t MPID_nem_ib_refill_srq(
        struct ibv_srq *srq, uint32_t max_post);

void async_thread(void *context)
{
    struct ibv_async_event event;
    struct ibv_srq_attr srq_attr;
    int post_new, ret;

    while (1) {

        do {    
            ret = ibv_get_async_event((struct ibv_context *) 
                    context, &event);

            if (ret && errno != EINTR) {
                NEM_IB_ERR("Error getting asynchronous event!\n");
            }       
        } while(ret && errno == EINTR); 

        switch (event.event_type) {
            /* Fatal */
            case IBV_EVENT_CQ_ERR:
            case IBV_EVENT_QP_FATAL:
            case IBV_EVENT_QP_REQ_ERR:
            case IBV_EVENT_QP_ACCESS_ERR:
            case IBV_EVENT_PATH_MIG:
            case IBV_EVENT_PATH_MIG_ERR:
            case IBV_EVENT_DEVICE_FATAL:
            case IBV_EVENT_SRQ_ERR:
                NEM_IB_ERR("Got FATAL event %d\n", event.event_type);
                break;  

            case IBV_EVENT_COMM_EST:
            case IBV_EVENT_PORT_ACTIVE:
            case IBV_EVENT_SQ_DRAINED:
            case IBV_EVENT_PORT_ERR:
            case IBV_EVENT_LID_CHANGE:
            case IBV_EVENT_PKEY_CHANGE:
            case IBV_EVENT_SM_CHANGE:
            case IBV_EVENT_QP_LAST_WQE_REACHED:
                break;  

            case IBV_EVENT_SRQ_LIMIT_REACHED:

                NEM_IB_DBG("Got SRQ Limit event");

post_new_bufs:  pthread_spin_lock(
                        &MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_post_lock);
#if 1
                post_new = MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted;

                MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted +=
                    MPID_nem_ib_refill_srq(
                            MPID_nem_ib_ctxt_ptr->ib_dev[0].srq,
                            MPID_nem_ib_dev_param_ptr->max_srq_wr -
                            MPID_nem_ib_dev_param_ptr->srq_limit);

                post_new = MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted - post_new;

                pthread_spin_unlock(
                        &MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_post_lock);

                if (0 == post_new) {
                    NEM_IB_DBG("Posting ZERO buffers");
                    MPIDU_Yield();
                    goto post_new_bufs;
                }

                srq_attr.max_wr = 
                    MPID_nem_ib_dev_param_ptr->max_srq_wr;
                srq_attr.max_sge = 1;
                srq_attr.srq_limit = 
                    MPID_nem_ib_dev_param_ptr->srq_limit;

                if (MPID_nem_ib_modify_srq(
                            MPID_nem_ib_ctxt_ptr->ib_dev[0].srq, 
                            MPID_nem_ib_dev_param_ptr->max_srq_wr, 
                            MPID_nem_ib_dev_param_ptr->srq_limit)) {
                    NEM_IB_ERR("Possibly FATAL, cannot modify SRQ\n");
                }       
#endif


                break;  
            default:
                NEM_IB_ERR("Got unknown event %d ... continuing ...",
                        event.event_type);
        }

        ibv_ack_async_event(&event);
    }
}

uint32_t MPID_nem_ib_refill_srq(
        struct ibv_srq *srq, uint32_t max_post)
{
    uint32_t post_count = 0;
    MPID_nem_cell_ptr_t c;
    MPID_nem_ib_cell_elem_t *ce;
    MPID_nem_ib_cell_elem_t *ce_root;
    MPID_nem_ib_cell_elem_t *ce_prev;
    int ret;

    /* Collect all possible receive requests to be posted */

    while( (post_count < max_post) &&
           (!MPID_nem_queue_empty(MPID_nem_module_ib_free_queue))) {

        MPID_nem_queue_dequeue(MPID_nem_module_ib_free_queue, &c);

        MPID_nem_ib_get_cell(&ce);

        ce->nem_cell = c;

        MPID_nem_ib_prep_cell_recv(ce, 
                (void *) MPID_NEM_CELL_TO_PACKET(c));

        if(0 == post_count) {
            ce_root = ce;
        } else {
            ce_prev->desc.u.r_wr.next = &ce->desc.u.r_wr;
        }

        ce_prev = ce;

        post_count++;
    }

    if(post_count > 0) {

        /* Post all of them in a list */
        ret = MPID_nem_ib_post_srq(
                MPID_nem_ib_ctxt_ptr->ib_dev[0].srq,
                &ce_root->desc.u.r_wr);

        if(ret) {
            NEM_IB_ERR("Error posting to SRQ, ret %d", ret);
        }

    }

    NEM_IB_DBG("Posted %d, max post %d, free %d\n", 
            post_count, max_post, 
            MPID_nem_queue_empty(MPID_nem_module_ib_free_queue));

    return post_count;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_poll()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPID_nem_ib_queue_elem_t *e, *sqe;
    MPID_nem_ib_cell_elem_t *ce;
    MPIDI_VC_t *vc;
    struct ibv_wc wc;

    ret = MPID_nem_ib_poll_cq(
            MPID_nem_ib_ctxt_ptr->ib_dev[0].cq, &wc);

    MPIU_ERR_CHKANDJUMP1(ret < 0, mpi_errno, MPI_ERR_OTHER, 
            "**ibv_poll_cq", "**ibv_poll_cq %d", ret);

    if(ret) {

        /* Got valid completion */
        ce = (MPID_nem_ib_cell_elem_t *) wc.wr_id;

        switch(wc.opcode) {
            case IBV_WC_SEND:

                NEM_IB_DBG("Got send completion");

                MPID_nem_queue_enqueue(MPID_nem_process_free_queue, 
                        ce->nem_cell);

                VC_FIELD(ce->vc, avail_send_wqes)++;

                MPID_nem_ib_return_cell(ce);

                break;

            case IBV_WC_RECV:

                NEM_IB_DBG("Got recv completion");

                MPID_nem_queue_enqueue(MPID_nem_process_recv_queue, 
                        ce->nem_cell);

                MPID_nem_ib_return_cell(ce);

                pthread_spin_lock(
                        &MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_post_lock);

                MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted--;

                if(MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted <
                        MPID_nem_ib_dev_param_ptr->srq_n_preserve) {

                    NEM_IB_DBG("Call to refill max post: %d, "
                            " preserve %u\n", 
                            MPID_nem_ib_dev_param_ptr->max_srq_wr -
                            MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted,
                            MPID_nem_ib_dev_param_ptr->srq_n_preserve);

                    MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted += 
                        MPID_nem_ib_refill_srq(
                                MPID_nem_ib_ctxt_ptr->ib_dev[0].srq,
                            MPID_nem_ib_dev_param_ptr->max_srq_wr -
                            MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_n_posted);
                }

                pthread_spin_unlock(
                        &MPID_nem_ib_ctxt_ptr->ib_dev[0].srq_post_lock);

                break;

            default:

                NEM_IB_ERR("Got completion with unknown opcode");
                mpi_errno = MPI_ERR_OTHER;
                MPIU_ERR_POP(mpi_errno);
        }
    }

    if(!MPID_nem_ib_queue_empty(MPID_nem_ib_vc_queue)) {

        /* We got VCs to process, get busy! */

        MPID_nem_ib_queue_dequeue(MPID_nem_ib_vc_queue, &e);

        vc = (MPIDI_VC_t *) e->data;

        if(MPID_NEM_IB_CONN_RC == VC_FIELD(vc, conn_status)) {

            /* Connected */

            while(!MPID_nem_ib_queue_empty(
                        (MPID_nem_ib_queue_t *)VC_FIELD(vc, ib_send_queue)) &&
                    VC_FIELD(vc, avail_send_wqes)) {

                MPID_nem_ib_queue_dequeue(
                        (MPID_nem_ib_queue_t *)VC_FIELD(vc, ib_send_queue),
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

            if(!MPID_nem_ib_queue_empty(
                        (MPID_nem_ib_queue_t *)VC_FIELD(vc, ib_send_queue))) {
                /* Still needs processing.
                 * Back to the queue you go! */
                MPID_nem_ib_queue_enqueue(
                        MPID_nem_ib_vc_queue, e);
            } else {

                /* This VC is no longer in the queue */
                VC_FIELD(vc, in_queue) = 0;
            }

        } else {

            /* Not connected, so put it back on queue */

            MPID_nem_ib_queue_enqueue(
                    MPID_nem_ib_vc_queue, e);
        }
    }


fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
