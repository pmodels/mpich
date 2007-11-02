/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ib_module_impl.h"
#include "mpidimpl.h"
#include "ib_module_cm.h"

#undef FUNCNAME
#define FUNCNAME cm_post_ud_packet
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

MPID_nem_ib_module_queue_ptr_t MPID_nem_ib_module_qp_queue = 0;

static int cm_post_ud_packet(
        MPID_nem_ib_cm_msg_t *msg,
        struct ibv_ah *ah,
        uint32_t remote_qpn)
{
    int mpi_errno = MPI_SUCCESS;
    int ret, ne;
    struct ibv_send_wr wr;
    struct ibv_send_wr *bad_wr;
    struct ibv_sge list;
    struct ibv_wc wc;

    MPIU_Assert(NULL != msg);

    memset(&wr, 0, sizeof(struct ibv_send_wr));
    memset(&list, 0, sizeof(struct ibv_sge));

    memcpy((void *) ((char *)
                MPID_nem_ib_cm_ctxt_ptr->cm_send_buf +
                MPID_nem_ib_cm_param_ptr->ud_overhead),
            msg, sizeof(MPID_nem_ib_cm_msg_t));

    list.addr = (uintptr_t) ((char *) 
            MPID_nem_ib_cm_ctxt_ptr->cm_send_buf +
            MPID_nem_ib_cm_param_ptr->ud_overhead);
    list.length = sizeof(MPID_nem_ib_cm_msg_t);
    list.lkey = MPID_nem_ib_cm_ctxt_ptr->cm_buf_mr->lkey;

    wr.wr_id = MPID_NEM_IB_CM_UD_SEND_WR_ID;
    wr.sg_list = &list;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_SEND;
    wr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_SOLICITED;
    wr.wr.ud.ah = ah;
    wr.wr.ud.remote_qpn = remote_qpn;
    wr.wr.ud.remote_qkey = 0;

    ret = ibv_post_send(MPID_nem_ib_cm_ctxt_ptr->ud_qp,
            &wr, &bad_wr);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_post_send", 
            "**ibv_post_send %s", "CM posting UD message");

    /* Poll for completion */

    while (1) {
        ne = ibv_poll_cq(MPID_nem_ib_cm_ctxt_ptr->send_cq, 1, &wc);
        if (ne < 0) {
            NEM_IB_ERR("CM poll CQ failed %d", ne);
        } else if (0 == ne) {
            continue;
        }

        if (wc.status != IBV_WC_SUCCESS) {
            NEM_IB_ERR("CM Failed status %d for wr_id %d",
                    wc.status, (int) wc.wr_id);
        }       

        if (MPID_NEM_IB_CM_UD_SEND_WR_ID == wc.wr_id) {
            NEM_IB_DBG("Got CM UD Send completion");
            break;
        } else {
            NEM_IB_ERR("unexpected completion, wr_id: %d",
                    (int) wc.wr_id);
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME cm_create_rc_qp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_create_rc_qp(struct ibv_qp **qp)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct ibv_qp_init_attr init_attr;
    struct ibv_qp_attr attr;
    MPID_nem_ib_module_queue_elem_ptr_t e;

    memset(&init_attr, 0, sizeof(init_attr));

    memcpy(&init_attr, &(MPID_nem_ib_cm_ctxt_ptr->rc_qp_init_attr),
            sizeof(struct ibv_qp_init_attr));

    *qp = ibv_create_qp(MPID_nem_ib_cm_ctxt_ptr->prot_domain,
            &init_attr);

    MPIU_ERR_CHKANDJUMP1( NULL == *qp, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_create_qp", 
            "**ibv_create_qp %s", "CM creating RC qp");

    MPID_nem_ib_module_queue_alloc(MPID_nem_ib_module_qp_queue, &e);

    e->data = *qp;

    NEM_IB_DBG("Enqueing qp %p", *qp);

    MPID_nem_ib_module_queue_enqueue(MPID_nem_ib_module_qp_queue, e);

    memset(&attr, 0, sizeof(attr));

    memcpy(&attr, &(MPID_nem_ib_cm_ctxt_ptr->rc_qp_attr_to_init),
            sizeof(struct ibv_qp_attr));

    ret = ibv_modify_qp(*qp, &attr,
            MPID_nem_ib_cm_ctxt_ptr->rc_qp_mask_to_init);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_modify_qp", 
            "**ibv_modify_qp %s", "CM modifying RC qp to INIT");

    NEM_IB_DBG("CM Created RC QP");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_enable_qp_init_to_rtr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_enable_qp_init_to_rtr(struct ibv_qp *qp,
        uint16_t dlid, uint32_t dqpn)
{
    int mpi_errno = MPI_SUCCESS;
    struct ibv_qp_attr attr;
    int ret;

    MPIU_Assert(NULL != qp);

    memset(&attr, 0, sizeof(attr));

    memcpy(&attr, &MPID_nem_ib_cm_ctxt_ptr->rc_qp_attr_to_rtr,
            sizeof(struct ibv_qp_attr));

    attr.dest_qp_num = dqpn;
    attr.ah_attr.dlid = dlid;

    ret = ibv_modify_qp(qp, &attr, 
            MPID_nem_ib_cm_ctxt_ptr->rc_qp_mask_to_rtr);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_modify_qp", 
            "**ibv_modify_qp %s", "CM modifying RC qp to RTR");
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_enable_qp_rtr_to_rts
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_enable_qp_rtr_to_rts(struct ibv_qp *qp)
{
    int mpi_errno = MPI_SUCCESS;
    struct ibv_qp_attr attr;
    int ret;

    MPIU_Assert(NULL != qp);

    memset(&attr, 0, sizeof(attr));

    memcpy(&attr, &MPID_nem_ib_cm_ctxt_ptr->rc_qp_attr_to_rts,
            sizeof(struct ibv_qp_attr));

    ret = ibv_modify_qp(qp, &attr,
            MPID_nem_ib_cm_ctxt_ptr->rc_qp_mask_to_rts);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_modify_qp", 
            "**ibv_modify_qp %s", "CM modifying RC qp to RTS");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME cm_post_ud_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_post_ud_recv(void *buf, int size)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    struct ibv_sge list;
    struct ibv_recv_wr wr;
    struct ibv_recv_wr *bad_wr;

    MPIU_Assert(NULL != buf);
    MPIU_Assert(size > 0);

    memset(&list, 0, sizeof(struct ibv_sge));
    memset(&wr, 0, sizeof(struct ibv_recv_wr));

    list.addr = (uintptr_t) buf;
    list.length = size + MPID_nem_ib_cm_param_ptr->ud_overhead;
    list.lkey = MPID_nem_ib_cm_ctxt_ptr->cm_buf_mr->lkey;

    wr.next = NULL;
    wr.wr_id = MPID_NEM_IB_CM_UD_RECV_WR_ID;
    wr.sg_list = &list;
    wr.num_sge = 1;

    ret = ibv_post_recv(MPID_nem_ib_cm_ctxt_ptr->ud_qp,
            &wr, &bad_wr);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_post_recv", 
            "**ibv_post_recv %d", ret);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_pending_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_pending_create(MPID_nem_ib_cm_pending_t **pending)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_cm_pending_t *ptr;

    MPIU_Assert(NULL != pending);

    ptr = MPIU_Malloc(sizeof(MPID_nem_ib_cm_pending_t));

    if(NULL == ptr) {
        MPIU_CHKMEM_SETERR(mpi_errno, sizeof(MPID_nem_ib_cm_pending_t), 
                "IB Module CM Pending List Element");
    }

    memset(ptr, 0, sizeof(MPID_nem_ib_cm_pending_t));

    *pending = ptr;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_pending_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_pending_init(MPID_nem_ib_cm_pending_ptr_t pending,
        MPID_nem_ib_cm_msg_ptr_t msg)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(NULL != msg);

    pending->packet = MPIU_Malloc(sizeof(MPID_nem_ib_cm_packet_t));

    memcpy(&(pending->packet->payload), msg, sizeof(MPID_nem_ib_cm_msg_t));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_pending_search_peer
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_pending_search_peer(
        MPID_nem_ib_cm_pending_ptr_t *pending_ptr,
        uint64_t peer_guid)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_nem_ib_cm_pending_t *pending = 
        MPID_nem_ib_cm_ctxt_ptr->cm_pending_head;

    *pending_ptr = NULL;

    while (pending->next != 
            MPID_nem_ib_cm_ctxt_ptr->cm_pending_head) {

        pending = pending->next;

        if (pending->peer_guid == peer_guid) {
            *pending_ptr = pending;
            break;
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_pending_list_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_pending_list_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_num = 0;

    mpi_errno = cm_pending_create(
            &MPID_nem_ib_cm_ctxt_ptr->cm_pending_head);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_head->peer_guid = 
        MPID_nem_ib_cm_ctxt_ptr->guid;

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_head->prev = 
        MPID_nem_ib_cm_ctxt_ptr->cm_pending_head;

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_head->next = 
        MPID_nem_ib_cm_ctxt_ptr->cm_pending_head;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_pending_remove_and_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_pending_remove_and_destroy(MPID_nem_ib_cm_pending_t *node)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Free(node->packet);

    node->next->prev = node->prev;
    node->prev->next = node->next;

    MPIU_Free(node);

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_num--;

    NEM_IB_DBG("CM Pending Destroy");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_pending_append
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_pending_append(MPID_nem_ib_cm_pending_t *node)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(NULL != node);

    MPID_nem_ib_cm_pending_t *last = 
        MPID_nem_ib_cm_ctxt_ptr->cm_pending_head->prev;

    last->next = node;
    node->next = 
        MPID_nem_ib_cm_ctxt_ptr->cm_pending_head;

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_head->prev = node;

    node->prev = last;

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_num++;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME cm_pending_list_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_pending_list_finalize()
{
    int mpi_errno = MPI_SUCCESS;

    while (MPID_nem_ib_cm_ctxt_ptr->cm_pending_head->next != 
            MPID_nem_ib_cm_ctxt_ptr->cm_pending_head) {
        cm_pending_remove_and_destroy(
                MPID_nem_ib_cm_ctxt_ptr->cm_pending_head->next);
    }

    MPIU_Assert( 0 == MPID_nem_ib_cm_ctxt_ptr->cm_pending_num);

    MPIU_Free(MPID_nem_ib_cm_ctxt_ptr->cm_pending_head);

    MPID_nem_ib_cm_ctxt_ptr->cm_pending_head = NULL;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_send_ud_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_send_ud_msg(MPID_nem_ib_cm_msg_t *msg,
        struct ibv_ah *ah,
        uint32_t remote_qpn,
        uint64_t dest_guid)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPID_nem_ib_cm_pending_t *pending;
    struct timeval now;

    mpi_errno = cm_pending_create(&pending);
    
    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }
    
    mpi_errno = cm_pending_init(pending, msg);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }
    
    mpi_errno = cm_pending_append(pending);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    pending->peer_guid = dest_guid;
    pending->peer_ud_qpn = remote_qpn;
    pending->peer_ah = ah;

    gettimeofday(&now, NULL);
    pending->packet->timestamp = now;

    mpi_errno = cm_post_ud_packet(&(pending->packet->payload),
            ah, remote_qpn);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    if(1 == MPID_nem_ib_cm_ctxt_ptr->cm_pending_num) { 
        pthread_cond_signal(&MPID_nem_ib_cm_ctxt_ptr->cm_cond_new_pending);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_connect_and_reply
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_connect_and_reply(MPID_nem_ib_cm_msg_t *msg,
        MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_cm_msg_t reply_msg;

    /* The VC should not have been "connected" */
    MPIU_Assert(MPID_NEM_IB_CONN_RC != VC_FIELD(vc, conn_status));

    if(NULL == VC_FIELD(vc, qp)) {
        /* Create QP */
        mpi_errno = cm_create_rc_qp(&VC_FIELD(vc, qp));

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }

    /* INIT -> RTR */
    mpi_errno = cm_enable_qp_init_to_rtr(VC_FIELD(vc, qp),
            msg->lid, msg->rc_qpn);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* RTR -> RTS */
    mpi_errno = cm_enable_qp_rtr_to_rts(VC_FIELD(vc, qp));

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Prepare reply message */
    reply_msg.msg_type = MPID_NEM_IB_CM_MSG_TYPE_REP;
    reply_msg.rc_qpn = VC_FIELD(vc, qp)->qp_num;
    reply_msg.lid = MPID_nem_ib_cm_ctxt_ptr->port_attr.lid;
    reply_msg.src_guid = MPID_nem_ib_cm_ctxt_ptr->guid;
    reply_msg.src_ud_qpn = MPID_nem_ib_cm_ctxt_ptr->ud_qp->qp_num;

    /* Send reply */
    mpi_errno = cm_post_ud_packet(&reply_msg, 
            VC_FIELD(vc, ud_ah), VC_FIELD(vc, ud_qpn));

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Mark connection as "connected" */
    VC_FIELD(vc, conn_status) = MPID_NEM_IB_CONN_RC;

    NEM_IB_DBG("RC Connection Server");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_reply
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_reply(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_cm_msg_t reply_msg;

    /* Prepare reply message */
    reply_msg.msg_type = MPID_NEM_IB_CM_MSG_TYPE_REP;
    reply_msg.rc_qpn = VC_FIELD(vc, qp)->qp_num;
    reply_msg.lid = MPID_nem_ib_cm_ctxt_ptr->port_attr.lid;
    reply_msg.src_guid = MPID_nem_ib_cm_ctxt_ptr->guid;
    reply_msg.src_ud_qpn = MPID_nem_ib_cm_ctxt_ptr->ud_qp->qp_num;

    /* Send reply */
    mpi_errno = cm_post_ud_packet(&reply_msg, 
            VC_FIELD(vc, ud_ah), VC_FIELD(vc, ud_qpn));

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME cm_connect_vc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_connect_vc(
        MPID_nem_ib_cm_msg_t *msg,
        MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(NULL != msg);

    /* INIT -> RTR */
    mpi_errno = cm_enable_qp_init_to_rtr(VC_FIELD(vc, qp),
            msg->lid, msg->rc_qpn);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* RTR -> RTS */
    mpi_errno = cm_enable_qp_rtr_to_rts(VC_FIELD(vc, qp));

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    VC_FIELD(vc, conn_status) = MPID_NEM_IB_CONN_RC;

    NEM_IB_DBG("RC Connection Client");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_handle_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static int cm_handle_msg(MPID_nem_ib_cm_msg_t *msg)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_cm_remote_id_ptr_t r_info = NULL;
    MPID_nem_ib_cm_pending_t *pending = NULL;
    MPIDI_VC_t *vc;
    int conn_status;

    MPIU_Assert(NULL != msg);

    /* Lookup source of the message from contents */

    /* The do-while loop is required since there is
     * no Barrier after VC init. So, a process might
     * still be in the process of inserting GUIDs
     * et al to the hash table, when the CM gets a
     * message from a remote node looking for a
     * message */

    /* TODO: Might be a more elegant way to handle this by
     * having CM unexpected queues? */

    do {
        r_info = (MPID_nem_ib_cm_remote_id_ptr_t) 
            MPID_nem_ib_module_lookup_hash_table(
                    &MPID_nem_ib_cm_ctxt_ptr->hash_table,
                    msg->src_guid, msg->src_ud_qpn);
    } while (NULL == r_info);

    MPIU_Assert(NULL != r_info->vc);

    vc = (MPIDI_VC_t *) r_info->vc;
    conn_status = VC_FIELD(vc, conn_status);

    switch(msg->msg_type) {

        case MPID_NEM_IB_CM_MSG_TYPE_REQ:

            pthread_mutex_lock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

            if(MPID_NEM_IB_CONN_NONE == conn_status) {

                /* No connection for this VC, start
                 * one right away */

                cm_connect_and_reply(msg, vc);

            } else if (MPID_NEM_IB_CONN_IN_PROGRESS == conn_status) {

                /* Connection process already started.
                 * REQ messages might have crossed over */

                MPIU_Assert(r_info->guid != 
                        MPID_nem_ib_cm_ctxt_ptr->guid);

                if(r_info->guid < MPID_nem_ib_cm_ctxt_ptr->guid) {

                    /* Silently ignore this case to
                     * break the tie */

                    NEM_IB_DBG("Silently ignoring crossed packet to break tie");

                } else {

                    /* I had started the connection process,
                     * however concurrently, another process
                     * with higher GUID has also started the
                     * connection process with me.
                     *
                     * Cancel timers associated with my REQ
                     * message, connect and send REP msg */

                    NEM_IB_DBG("Crossed packet, cancelling timeout");

                    cm_pending_search_peer(&pending, r_info->guid);

                    /* The timer for this REQ msg should _always_
                     * be present, since the remote side should've
                     * dropped my REQ packet and never have replied
                     * so I couldn't have cancelled this REQ
                     * anywhere else */
                    MPIU_Assert(NULL != pending);

                    cm_pending_remove_and_destroy(pending);

                    cm_connect_and_reply(msg, vc);
                }

            } else {
                /* MPID_NEM_IB_CONN_RC */

                /* I may have completed the connection, but
                 * the remote side didn't get the QPN and
                 * LID, so, its asking for it again */

                cm_reply(vc);
            }

            pthread_mutex_unlock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

            break;

        case MPID_NEM_IB_CM_MSG_TYPE_REP:

            pthread_mutex_lock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

            if(MPID_NEM_IB_CONN_NONE == conn_status) {

                /* We should never receive REP message
                 * when the connection status is NONE */

                NEM_IB_ERR("Illegal REP message received by CM");
                
            } else if (MPID_NEM_IB_CONN_IN_PROGRESS == conn_status) {

                /* This is a reply to my request, proceed to connect */

                cm_connect_vc(msg, vc);

                /* Cancel timers associated to my REQ */
                cm_pending_search_peer(&pending, r_info->guid);

                MPIU_Assert(NULL != pending);

                cm_pending_remove_and_destroy(pending);

            } else {
                /* MPID_NEM_IB_CONN_RC */

                /* We are already connected, so the only way
                 * we could've gotten this message is b'coz
                 * of a re-transmit of the REQ message and
                 * subsequent reply */

                /* Just ignore this message */
            }

            pthread_mutex_unlock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

            break;
        default:
            NEM_IB_ERR("Bogus message received by CM");
            mpi_errno = MPI_ERR_OTHER;
            MPIU_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cm_completion_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static void* cm_completion_thread()
{
    struct ibv_cq   *ev_cq;
    struct ibv_wc   wc;
    void *ev_ctx, *buf;
    int ret, spin_count, ne;
    MPID_nem_ib_cm_msg_t *msg;

    while(1) {

        do {
            ret = ibv_get_cq_event(MPID_nem_ib_cm_ctxt_ptr->comp_channel, 
                    &ev_cq, &ev_ctx);
            if (ret && errno != EINTR ) {
                NEM_IB_ERR("Error getting CM CQ Event %d", ret);
            }
        } while(ret && errno == EINTR);

        ibv_ack_cq_events(ev_cq, 1);

        if (ev_cq != MPID_nem_ib_cm_ctxt_ptr->recv_cq) {
            NEM_IB_ERR("Received Event for Unknown CQ",
                    ev_cq, MPID_nem_ib_cm_ctxt_ptr->recv_cq);
        }

        /* Process CM Message */

        spin_count = 0;

        do {

            ne = ibv_poll_cq(MPID_nem_ib_cm_ctxt_ptr->recv_cq, 1, &wc);
            if(ne < 0) {
                NEM_IB_ERR("Error polling CM CQ");
                continue;
            } else if (0 == ne) {
                spin_count++;
                continue;
            }

            /* Control comes here after discovering a message
             * from the CQ, so reset the spin_count */

            spin_count = 0;

            if(IBV_WC_SUCCESS != wc.status) {
                NEM_IB_ERR("Erroneous completion from CM CQ, status %d",
                        wc.status);
                continue;
            }

            if(MPID_NEM_IB_CM_UD_RECV_WR_ID == wc.wr_id) {
                /* Valid completion */

                buf = (void *) ((char *) MPID_nem_ib_cm_ctxt_ptr->cm_recv_buf +
                    (MPID_nem_ib_cm_ctxt_ptr->cm_recv_buf_index * 
                     (sizeof(MPID_nem_ib_cm_msg_t) + 
                      MPID_nem_ib_cm_param_ptr->ud_overhead)) + 
                    MPID_nem_ib_cm_param_ptr->ud_overhead);

                msg = (MPID_nem_ib_cm_msg_t *) buf; 

                NEM_IB_DBG("Got msg src_guid %lu, src_ud_qpn %u",
                        msg->src_guid, msg->src_ud_qpn);

                if (MPI_SUCCESS != cm_handle_msg(msg)) {
                    NEM_IB_ERR("Error handling CM message");
                }

                /* Re-post this buffer */
                if (MPI_SUCCESS != cm_post_ud_recv
                        (buf - 40, sizeof(MPID_nem_ib_cm_msg_t))) {
                    NEM_IB_ERR("Error re-posting CM buffer");
                }

                MPID_nem_ib_cm_ctxt_ptr->cm_recv_buf_index =
                    (MPID_nem_ib_cm_ctxt_ptr->cm_recv_buf_index + 1) % 
                    MPID_nem_ib_cm_param_ptr->cm_n_buf;


            } else {
                NEM_IB_ERR("Got unknown completion from CM UD Recv CQ, id %d",
                        wc.wr_id);
            }

        } while(spin_count < MPID_nem_ib_cm_param_ptr->cm_max_spin_count);

        /* Request of interrupt again */

        ret = ibv_req_notify_cq(MPID_nem_ib_cm_ctxt_ptr->recv_cq, 1);

        if(ret) {
            NEM_IB_ERR("Request for CQ notification failed %d", ret);
        }

    }

    return NULL;
}

#undef FUNCNAME
#define FUNCNAME cm_completion_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

static void* cm_timeout_thread()
{
    struct timespec cm_timeout;
    struct timespec remain;
    struct timeval now;
    uint64_t delay;

    MPID_nem_ib_cm_pending_t *p;

    cm_timeout.tv_sec = MPID_nem_ib_cm_param_ptr->cm_timeout_ms/1e6;
    cm_timeout.tv_nsec = (MPID_nem_ib_cm_param_ptr->cm_timeout_ms -
            cm_timeout.tv_sec*1e6) * 1e6;

    while(1) {

        pthread_mutex_lock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

        while (0 == MPID_nem_ib_cm_ctxt_ptr->cm_pending_num) {

            pthread_cond_wait(&MPID_nem_ib_cm_ctxt_ptr->cm_cond_new_pending, 
                    &MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);
        }

        while(1) {
            pthread_mutex_unlock(
                    &MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

            nanosleep(&cm_timeout, &remain);

            pthread_mutex_lock(
                    &MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

            NEM_IB_DBG("CM Timeout");

            if(0 == MPID_nem_ib_cm_ctxt_ptr->cm_pending_num) {

                /* Nothing more to do, goto blocking mode */
                break;
            }

            p = MPID_nem_ib_cm_ctxt_ptr->cm_pending_head;

            MPIU_Assert(p != NULL);

            gettimeofday(&now, NULL);

            while(p->next != MPID_nem_ib_cm_ctxt_ptr->cm_pending_head) {

                p = p->next;

                delay = (now.tv_sec - p->packet->timestamp.tv_sec) * 1e6 +
                    (now.tv_usec - p->packet->timestamp.tv_usec);

                if(delay > MPID_nem_ib_cm_param_ptr->cm_timeout_ms) {

                    NEM_IB_DBG("CM Packet Retransmit");

                    p->packet->timestamp = now;

                    cm_post_ud_packet(&(p->packet->payload), p->peer_ah,
                            p->peer_ud_qpn);

                    gettimeofday(&now, NULL);
                }
            }
        }

        pthread_mutex_unlock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

    }

    return NULL;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_init_cm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_cm_init_cm()
{
    int mpi_errno = MPI_SUCCESS;
    int i, ret;

    MPID_nem_ib_cm_ctxt_ptr_t ctxt_ptr = MPID_nem_ib_cm_ctxt_ptr;
    MPID_nem_ib_cm_param_ptr_t param_ptr = MPID_nem_ib_cm_param_ptr;

    ctxt_ptr->cm_buf_mr = NULL;

    /* Allocate space for the CM buffers
     *
     * There are cm_n_buf receive buffers
     * and only ONE send buffer. This is
     * because we use only one outstanding
     * send at one time to simply the code
     */

    ctxt_ptr->cm_buf = MPIU_Malloc((sizeof(MPID_nem_ib_cm_msg_t) + 
                param_ptr->ud_overhead) * (param_ptr->cm_n_buf + 1));

    if(NULL == ctxt_ptr->cm_buf) {
        MPIU_CHKMEM_SETERR(mpi_errno, (sizeof(MPID_nem_ib_cm_msg_t) + 
                    param_ptr->ud_overhead) * (param_ptr->cm_n_buf + 1), 
                "IB Module CM Buffers");
    }

    memset(ctxt_ptr->cm_buf, 0, (sizeof(MPID_nem_ib_cm_msg_t) + 
             param_ptr->ud_overhead) * (param_ptr->cm_n_buf + 1));

    ctxt_ptr->cm_buf_mr = ibv_reg_mr(ctxt_ptr->prot_domain,
            ctxt_ptr->cm_buf,
            (sizeof(MPID_nem_ib_cm_msg_t) + 
            param_ptr->ud_overhead) * (param_ptr->cm_n_buf + 1), 
            IBV_ACCESS_LOCAL_WRITE);

    MPIU_ERR_CHKANDJUMP1(NULL == ctxt_ptr->cm_buf_mr, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_reg_mr", 
            "**ibv_reg_mr %d", (sizeof(MPID_nem_ib_cm_msg_t) +
                    param_ptr->ud_overhead) * (param_ptr->cm_n_buf + 1));

    /* Set the send and receive pointers */
    ctxt_ptr->cm_send_buf = ctxt_ptr->cm_buf;
    ctxt_ptr->cm_recv_buf = (void *) ((char *) ctxt_ptr->cm_buf +
            (sizeof(MPID_nem_ib_cm_msg_t) + param_ptr->ud_overhead));

    /* Cache the port properties inside CM data structures */
    ibv_query_port(ctxt_ptr->context, ctxt_ptr->hca_port,
            &ctxt_ptr->port_attr);

    ctxt_ptr->comp_channel = ibv_create_comp_channel(ctxt_ptr->context);

    MPIU_ERR_CHKANDJUMP1( NULL == ctxt_ptr->comp_channel, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_create_comp_channel", 
            "**ibv_create_comp_channel %s", ibv_get_device_name(ctxt_ptr->nic));

    ctxt_ptr->send_cq = ibv_create_cq(ctxt_ptr->context,
            param_ptr->cm_max_send, NULL, ctxt_ptr->comp_channel, 0);

    MPIU_ERR_CHKANDJUMP1( NULL == ctxt_ptr->send_cq, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_create_cq", 
            "**ibv_create_cq %s", ibv_get_device_name(ctxt_ptr->nic));

    ctxt_ptr->recv_cq = ibv_create_cq(ctxt_ptr->context,
            param_ptr->cm_n_buf, NULL, ctxt_ptr->comp_channel, 0);

    MPIU_ERR_CHKANDJUMP1( NULL == ctxt_ptr->recv_cq, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_create_cq", 
            "**ibv_create_cq %s", ibv_get_device_name(ctxt_ptr->nic));

    {
        struct ibv_qp_init_attr attr;
        memset(&attr, 0, sizeof(struct ibv_qp_init_attr));
        attr.send_cq = ctxt_ptr->send_cq;
        attr.recv_cq = ctxt_ptr->recv_cq;
        attr.cap.max_send_wr = param_ptr->cm_max_send;
        attr.cap.max_recv_wr = param_ptr->cm_n_buf;
        attr.cap.max_send_sge = 1;
        attr.cap.max_recv_sge = 1;
        attr.qp_type = IBV_QPT_UD;

        ctxt_ptr->ud_qp = ibv_create_qp(ctxt_ptr->prot_domain, &attr); 

        MPIU_ERR_CHKANDJUMP1( NULL == ctxt_ptr->ud_qp, 
                mpi_errno, MPI_ERR_OTHER, "**ibv_create_qp", 
                "**ibv_create_qp %s", 
                ibv_get_device_name(ctxt_ptr->nic));
    }

    NEM_IB_DBG("Created UD QP with num %u", ctxt_ptr->ud_qp->qp_num);

    /* Init */

    {
        struct ibv_qp_attr attr;
        memset(&attr, 0, sizeof(struct ibv_qp_attr));

        attr.qp_state = IBV_QPS_INIT;
        attr.pkey_index = 0;
        attr.port_num = ctxt_ptr->hca_port;
        attr.qkey = 0;

        ret = ibv_modify_qp(ctxt_ptr->ud_qp, &attr,
                                 IBV_QP_STATE |
                                 IBV_QP_PKEY_INDEX |
                                 IBV_QP_PORT | IBV_QP_QKEY);

        MPIU_ERR_CHKANDJUMP1( ret != 0, 
                mpi_errno, MPI_ERR_OTHER, "**ibv_modify_qp", 
                "**ibv_modify_qp %s", ibv_get_device_name(ctxt_ptr->nic));
    }

    NEM_IB_DBG("Modified to Init");

    /* RTR */

    {
        struct ibv_qp_attr attr;
        memset(&attr, 0, sizeof(struct ibv_qp_attr));

        attr.qp_state = IBV_QPS_RTR;

        ret = ibv_modify_qp(ctxt_ptr->ud_qp, &attr, 
                IBV_QP_STATE);

        MPIU_ERR_CHKANDJUMP1( ret != 0, 
                mpi_errno, MPI_ERR_OTHER, "**ibv_modify_qp", 
                "**ibv_modify_qp %s", ibv_get_device_name(ctxt_ptr->nic));

    }

    NEM_IB_DBG("Modified to RTR");

    /* RTS */

    {
        struct ibv_qp_attr attr;
        memset(&attr, 0, sizeof(struct ibv_qp_attr));

        attr.qp_state = IBV_QPS_RTS;
        attr.sq_psn = 0;

        ret = ibv_modify_qp(ctxt_ptr->ud_qp, &attr, 
                IBV_QP_STATE | IBV_QP_SQ_PSN);

        MPIU_ERR_CHKANDJUMP1( ret != 0, 
                mpi_errno, MPI_ERR_OTHER, "**ibv_modify_qp", 
                "**ibv_modify_qp %s", ibv_get_device_name(ctxt_ptr->nic));
    }

    NEM_IB_DBG("Modified to RTS");

    for (i = 0; i < param_ptr->cm_n_buf; i++) {

        mpi_errno = cm_post_ud_recv((void *)
                ((char *) ctxt_ptr->cm_recv_buf + 
                 (sizeof(MPID_nem_ib_cm_msg_t) + 
                  param_ptr->ud_overhead) * i), 
                sizeof(MPID_nem_ib_cm_msg_t));

        if(mpi_errno) {
            MPIU_ERR_POP(mpi_errno);
        }
    }

    ctxt_ptr->cm_recv_buf_index = 0;

    pthread_mutex_init(&ctxt_ptr->cm_conn_state_lock, NULL);
    pthread_cond_init(&ctxt_ptr->cm_cond_new_pending, NULL);

    ret = ibv_req_notify_cq(ctxt_ptr->recv_cq, 1);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_req_notify_cq", 
            "**ibv_req_notify_cq %s", ibv_get_device_name(ctxt_ptr->nic));

    mpi_errno = cm_pending_list_init();

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Initialize the QP queue */

    MPID_nem_ib_module_queue_init(&MPID_nem_ib_module_qp_queue);

    /* Initialize the Hash table */

    mpi_errno = MPID_nem_ib_module_init_hash_table(&ctxt_ptr->hash_table,
            param_ptr->cm_hash_size);

    if(mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* Spawn the CM thread */

    ret = pthread_create(&ctxt_ptr->cm_comp_thread, NULL,
            cm_completion_thread, NULL);
    
    MPIU_ERR_CHKANDJUMP1( ret != 0, mpi_errno, MPI_ERR_OTHER, 
            "**pthread_create", "**pthread_create %d", ret);

    /* Spawn the CM timeout thread */

    ret = pthread_create(&ctxt_ptr->cm_timeout_thread, NULL,
            cm_timeout_thread, NULL);

    MPIU_ERR_CHKANDJUMP1( ret != 0, mpi_errno, MPI_ERR_OTHER, 
            "**pthread_create", "**pthread_create %d", ret);

fn_exit:
    return mpi_errno;
fn_fail:
    if(ctxt_ptr->ud_qp) {
        ibv_destroy_qp(ctxt_ptr->ud_qp);
    }
    if(ctxt_ptr->recv_cq) {
        ibv_destroy_cq(ctxt_ptr->recv_cq);
    }
    if(ctxt_ptr->send_cq) {
        ibv_destroy_cq(ctxt_ptr->send_cq);
    }
    if(ctxt_ptr->cm_buf_mr) {
        ibv_dereg_mr(ctxt_ptr->cm_buf_mr);
    }
    if(ctxt_ptr->cm_buf) {
        MPIU_Free(ctxt_ptr->cm_buf);
    }
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_start_connect
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_cm_start_connect(
        MPID_nem_ib_cm_remote_id_ptr_t r_info,
        MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_ib_cm_msg_t  msg;

    pthread_mutex_lock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

    /* Only this function can set status to MPID_NEM_IB_CONN_IN_PROGRESS,
     * and it definitely shouldn't have been called twice! */

    MPIU_Assert(VC_FIELD(vc, conn_status) != MPID_NEM_IB_CONN_IN_PROGRESS);

    if(MPID_NEM_IB_CONN_RC == VC_FIELD(vc, conn_status)) {
        /* Must've been a race condition in which
         * a process discovers that the VC is
         * disconnected, but by the time this
         * function is called, its connected
         * already. Do nothing in this case,
         * just report success */
        pthread_mutex_unlock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);
        return mpi_errno;
    }

    VC_FIELD(vc, conn_status) = MPID_NEM_IB_CONN_IN_PROGRESS;


    /* Never clobber an existing QP */
    MPIU_Assert(NULL == VC_FIELD(vc, qp));
    
    mpi_errno = cm_create_rc_qp(&VC_FIELD(vc, qp));

    if(mpi_errno) {
        pthread_mutex_unlock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);
        MPIU_ERR_POP(mpi_errno);
    }

    msg.msg_type = MPID_NEM_IB_CM_MSG_TYPE_REQ;
    msg.req_id = ++(MPID_nem_ib_cm_ctxt_ptr->cm_req_id_global);
    msg.rc_qpn = VC_FIELD(vc, qp)->qp_num;
    msg.lid = MPID_nem_ib_cm_ctxt_ptr->port_attr.lid;
    msg.src_guid = MPID_nem_ib_cm_ctxt_ptr->guid;
    msg.src_ud_qpn = MPID_nem_ib_cm_ctxt_ptr->ud_qp->qp_num;

    mpi_errno = cm_send_ud_msg(&msg, VC_FIELD(vc, ud_ah), 
            VC_FIELD(vc, ud_qpn), VC_FIELD(vc, node_guid));

    if(mpi_errno) {
        pthread_mutex_unlock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);
        MPIU_ERR_POP(mpi_errno);
    }

    pthread_mutex_unlock(&MPID_nem_ib_cm_ctxt_ptr->cm_conn_state_lock);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_finalize_rc_qps
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_cm_finalize_rc_qps()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    MPID_nem_ib_module_queue_elem_ptr_t e;

    while(!MPID_nem_ib_module_queue_empty(
                MPID_nem_ib_module_qp_queue)) {

        MPID_nem_ib_module_queue_dequeue(
                MPID_nem_ib_module_qp_queue, &e);

        MPIU_Assert(NULL != e->data);

        NEM_IB_DBG("Destroying qp %p", e->data);

        ret = ibv_destroy_qp((struct ibv_qp *)(e->data));

        MPIU_ERR_CHKANDJUMP1( ret != 0, 
                mpi_errno, MPI_ERR_OTHER, "**ibv_destroy_qp", 
                "**ibv_destroy_qp %s", "CM destroy RC QP");

        MPIU_Free(e);
    }

    MPID_nem_ib_module_queue_finalize(MPID_nem_ib_module_qp_queue);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_cm_finalize_cm
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)

int MPID_nem_ib_cm_finalize_cm()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;

    NEM_IB_DBG("Finalize CM");

    /* Is it really true that no pending requests will
     * be present when Finalize is called? */
    /* MPIU_Assert(0 == MPID_nem_ib_cm_ctxt_ptr->cm_pending_num); */

    mpi_errno = cm_pending_list_finalize();

    ret = pthread_cancel(MPID_nem_ib_cm_ctxt_ptr->cm_comp_thread);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**pthread_cancel", 
            "**pthread_cancel %s", "CM cancelling completion thread");

    ret = pthread_cancel(MPID_nem_ib_cm_ctxt_ptr->cm_timeout_thread);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**pthread_cancel", 
            "**pthread_cancel %s", "CM cancelling timeout thread");

    ret = ibv_destroy_qp(MPID_nem_ib_cm_ctxt_ptr->ud_qp);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_destroy_qp", 
            "**ibv_destroy_qp %s", "CM destroy UD QP");

    ret = ibv_destroy_comp_channel(MPID_nem_ib_cm_ctxt_ptr->comp_channel);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_destroy_comp_channel", 
            "**ibv_destroy_comp_channel %s", "CM destroy comp channel");

    ret = ibv_destroy_cq(MPID_nem_ib_cm_ctxt_ptr->recv_cq);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_destroy_cq", 
            "**ibv_destroy_cq %s", "CM destroy Recv CQ");

    ret = ibv_destroy_cq(MPID_nem_ib_cm_ctxt_ptr->send_cq);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_destroy_cq", 
            "**ibv_destroy_cq %s", "CM destroy Send CQ");

    ret = ibv_dereg_mr(MPID_nem_ib_cm_ctxt_ptr->cm_buf_mr);

    MPIU_ERR_CHKANDJUMP1( ret != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_dereg_mr", 
            "**ibv_dereg_mr %s", "CM deregister buffers");

    MPIU_Free(MPID_nem_ib_cm_ctxt_ptr->cm_buf);

    MPID_nem_ib_module_finalize_hash_table(
            &MPID_nem_ib_cm_ctxt_ptr->hash_table);


    NEM_IB_DBG("Exiting Finalize CM");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
