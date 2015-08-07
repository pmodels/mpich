/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "rptl.h"

/* FIXME: turn this into a CVAR, or fraction of the event limit from
   rptl_init */
#define PER_TARGET_THRESHOLD 50

/*
 * Prereqs:
 *
 * 1. We create an extra control portal that is only used by rportals.
 *
 * 2. All communication operations are logged at the origin process,
 * and their ACKs and NACKs are kept track of.  If an operation gets
 * an ACK, it is complete and can be deleted from the logs.  If an
 * operation gets a NACK, it will need to be retransmitted once the
 * flow-control protocol described below has completed.
 *
 *
 * Flow control algorithm:
 *
 * 1. When the primary data portal gets disabled, the target sends
 * PAUSE messages to all other processes.
 *
 * 2. Once each process confirms that it has no outstanding packets on
 * the wire (i.e., all packets have either been ACKed or NACKed), it
 * sends a PAUSE-ACK message.
 *
 * 3. When the target receives PAUSE-ACK messages from all processes
 * (thus confirming that the network traffic to itself has been
 * quiesced), it waits till the user has dequeued at least half the
 * messages from the overflow buffer.  This is done by keeping track
 * of the number of messages that are injected into the overflow
 * buffer by portals and the number of messages that are dequeued by
 * the user.
 *
 * 4. Once we know that there is enough free space in the overflow
 * buffers, the target reenables the portal and send an UNPAUSE
 * message to all processes.
 *
 *
 * Known issues:
 *
 * 1. None of the error codes specified by portals allow us to return
 * an "OTHER" error, when something bad happens internally.  So we
 * arbitrarily return PTL_FAIL when it is an internal error even
 * though that's not a specified error return code for some portals
 * functions.  When portals functions are called internally, if they
 * return an error, we funnel them back upstream.  This is not an
 * "issue" per se, but is still ugly.
 *
 * 2. None of the pt index types specified by portals allow us to
 * retuen an "INVALID" pt entry, to show that a portal is invalid.  So
 * we arbitrarily use PTL_PT_ANY in such cases.  Again, this is not an
 * "issue" per se, but is ugly.
 */

#define IDS_ARE_EQUAL(t1, t2) \
    (t1.phys.nid == t2.phys.nid && t1.phys.pid == t2.phys.pid)

struct rptl_info rptl_info;


#undef FUNCNAME
#define FUNCNAME find_target
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int find_target(ptl_process_t id, struct rptl_target **target)
{
    int mpi_errno = MPI_SUCCESS;
    int ret = PTL_OK;
    struct rptl_target *t;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_FIND_TARGET);

    MPIDI_FUNC_ENTER(MPID_STATE_FIND_TARGET);

    for (t = rptl_info.target_list; t; t = t->next)
        if (IDS_ARE_EQUAL(t->id, id))
            break;

    /* if the target does not already exist, create one */
    if (t == NULL) {
        MPIU_CHKPMEM_MALLOC(t, struct rptl_target *, sizeof(struct rptl_target), mpi_errno, "rptl target");
        MPL_DL_APPEND(rptl_info.target_list, t);

        t->id = id;
        t->state = RPTL_TARGET_STATE_ACTIVE;
        t->rptl = NULL;
        t->op_segment_list = NULL;
        t->op_pool = NULL;
        t->data_op_list = NULL;
        t->control_op_list = NULL;
        t->issued_data_ops = 0;
    }

    *target = t;

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_FIND_TARGET);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


static int rptl_put(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                    ptl_ack_req_t ack_req, ptl_process_t target_id, ptl_pt_index_t pt_index,
                    ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr,
                    ptl_hdr_data_t hdr_data, enum rptl_pt_type pt_type);

#undef FUNCNAME
#define FUNCNAME poke_progress
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int poke_progress(void)
{
    int ret = PTL_OK;
    struct rptl_target *target;
    struct rptl_op *op;
    struct rptl *rptl;
    int i;
    int mpi_errno = MPI_SUCCESS;
    ptl_process_t id;
    ptl_pt_index_t data_pt, control_pt;
    MPIDI_STATE_DECL(MPID_STATE_POKE_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_POKE_PROGRESS);

    /* make progress on local RPTLs */
    for (rptl = rptl_info.rptl_list; rptl; rptl = rptl->next) {
        /* if the local state is active, there's nothing to do */
        if (rptl->local_state == RPTL_LOCAL_STATE_ACTIVE)
            continue;

        /* if we are in a local AWAITING PAUSE ACKS state, see if we
         * can send out the unpause message */
        if (rptl->local_state == RPTL_LOCAL_STATE_AWAITING_PAUSE_ACKS &&
            rptl->pause_ack_counter == rptl_info.world_size - 1) {
            /* if we are over the max count limit, do not send an
             * unpause message yet */
            if (rptl->data.ob_curr_count > rptl->data.ob_max_count)
                continue;

            ret = PtlPTEnable(rptl->ni, rptl->data.pt);
            RPTLU_ERR_POP(ret, "Error returned while reenabling PT\n");

            rptl->local_state = RPTL_LOCAL_STATE_ACTIVE;

            for (i = 0; i < rptl_info.world_size; i++) {
                if (i == MPIDI_Process.my_pg_rank)
                    continue;
                mpi_errno = rptl_info.get_target_info(i, &id, rptl->data.pt, &data_pt, &control_pt);
                if (mpi_errno) {
                    ret = PTL_FAIL;
                    RPTLU_ERR_POP(ret, "Error getting target info\n");
                }

                /* make sure the user setup a control portal */
                assert(control_pt != PTL_PT_ANY);

                ret = rptl_put(rptl->md, 0, 0, PTL_NO_ACK_REQ, id, control_pt,
                               0, 0, NULL, RPTL_CONTROL_MSG_UNPAUSE, RPTL_PT_CONTROL);
                RPTLU_ERR_POP(ret, "Error sending unpause message\n");
            }
        }
    }

    /* make progress on targets */
    for (target = rptl_info.target_list; target; target = target->next) {
        if (target->state == RPTL_TARGET_STATE_RECEIVED_PAUSE) {
            for (op = target->data_op_list; op; op = op->next)
                if (op->state == RPTL_OP_STATE_ISSUED)
                    break;
            if (op)
                continue;

            /* send a pause ack message */
            assert(target->rptl);
            for (i = 0; i < rptl_info.world_size; i++) {
                if (i == MPIDI_Process.my_pg_rank)
                    continue;
                /* find the target that has this target id and get the
                 * control portal information for it */
                mpi_errno = rptl_info.get_target_info(i, &id, target->rptl->data.pt, &data_pt, &control_pt);
                if (mpi_errno) {
                    ret = PTL_FAIL;
                    RPTLU_ERR_POP(ret, "Error getting target info\n");
                }
                if (IDS_ARE_EQUAL(id, target->id))
                    break;
            }

            /* make sure the user setup a control portal */
            assert(control_pt != PTL_PT_ANY);

            target->state = RPTL_TARGET_STATE_PAUSE_ACKED;

            ret = rptl_put(target->rptl->md, 0, 0, PTL_NO_ACK_REQ, id, control_pt, 0,
                           0, NULL, RPTL_CONTROL_MSG_PAUSE_ACK, RPTL_PT_CONTROL);
            RPTLU_ERR_POP(ret, "Error sending pause ack message\n");

            continue;
        }

        /* issue out all the control messages first */
        for (op = target->control_op_list; op; op = op->next) {
            assert(op->op_type == RPTL_OP_PUT);

            /* skip all the issued ops */
            if (op->state == RPTL_OP_STATE_ISSUED)
                continue;

            /* we should not get any NACKs on the control portal */
            assert(op->state != RPTL_OP_STATE_NACKED);

            if (rptl_info.origin_events_left < 2 || target->issued_data_ops > PER_TARGET_THRESHOLD) {
                /* too few origin events left.  we can't issue this op
                 * or any following op to this target in order to
                 * maintain ordering */
                break;
            }

            rptl_info.origin_events_left -= 2;
            target->issued_data_ops++;

            /* force request for an ACK even if the user didn't ask
             * for it.  replace the user pointer with the OP id. */
            ret = PtlPut(op->u.put.md_handle, op->u.put.local_offset, op->u.put.length,
                         PTL_ACK_REQ, op->u.put.target_id, op->u.put.pt_index,
                         op->u.put.match_bits, op->u.put.remote_offset, op,
                         op->u.put.hdr_data);
            RPTLU_ERR_POP(ret, "Error issuing PUT\n");

            op->state = RPTL_OP_STATE_ISSUED;
        }

        if (target->state == RPTL_TARGET_STATE_DISABLED || target->state == RPTL_TARGET_STATE_PAUSE_ACKED)
            continue;

        /* then issue out all the data messages */
        for (op = target->data_op_list; op; op = op->next) {
            if (op->op_type == RPTL_OP_PUT) {
                /* skip all the issued ops */
                if (op->state == RPTL_OP_STATE_ISSUED)
                    continue;

                /* if an op has been nacked, don't issue anything else
                 * to this target */
                if (op->state == RPTL_OP_STATE_NACKED)
                    break;

                if (rptl_info.origin_events_left < 2 || target->issued_data_ops > PER_TARGET_THRESHOLD) {
                    /* too few origin events left.  we can't issue
                     * this op or any following op to this target in
                     * order to maintain ordering */
                    break;
                }

                rptl_info.origin_events_left -= 2;
                target->issued_data_ops++;

                /* force request for an ACK even if the user didn't
                 * ask for it.  replace the user pointer with the OP
                 * id. */
                ret = PtlPut(op->u.put.md_handle, op->u.put.local_offset, op->u.put.length,
                             PTL_ACK_REQ, op->u.put.target_id, op->u.put.pt_index,
                             op->u.put.match_bits, op->u.put.remote_offset, op,
                             op->u.put.hdr_data);
                RPTLU_ERR_POP(ret, "Error issuing PUT\n");
            }
            else if (op->op_type == RPTL_OP_GET) {
                /* skip all the issued ops */
                if (op->state == RPTL_OP_STATE_ISSUED)
                    continue;

                /* if an op has been nacked, don't issue anything else
                 * to this target */
                if (op->state == RPTL_OP_STATE_NACKED)
                    break;

                if (rptl_info.origin_events_left < 1 || target->issued_data_ops > PER_TARGET_THRESHOLD) {
                    /* too few origin events left.  we can't issue
                     * this op or any following op to this target in
                     * order to maintain ordering */
                    break;
                }

                rptl_info.origin_events_left--;
                target->issued_data_ops++;

                ret = PtlGet(op->u.get.md_handle, op->u.get.local_offset, op->u.get.length,
                             op->u.get.target_id, op->u.get.pt_index, op->u.get.match_bits,
                             op->u.get.remote_offset, op);
                RPTLU_ERR_POP(ret, "Error issuing GET\n");
            }

            op->state = RPTL_OP_STATE_ISSUED;
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_POKE_PROGRESS);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME rptl_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int rptl_put(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                    ptl_ack_req_t ack_req, ptl_process_t target_id, ptl_pt_index_t pt_index,
                    ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr,
                    ptl_hdr_data_t hdr_data, enum rptl_pt_type pt_type)
{
    struct rptl_op *op;
    int ret = PTL_OK;
    struct rptl_target *target;
    MPIDI_STATE_DECL(MPID_STATE_RPTL_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_RPTL_PUT);

    ret = find_target(target_id, &target);
    RPTLU_ERR_POP(ret, "error finding target structure\n");

    ret = rptli_op_alloc(&op, target);
    RPTLU_ERR_POP(ret, "error allocating op\n");

    op->op_type = RPTL_OP_PUT;
    op->state = RPTL_OP_STATE_QUEUED;

    /* store the user parameters */
    op->u.put.md_handle = md_handle;
    op->u.put.local_offset = local_offset;
    op->u.put.length = length;
    op->u.put.ack_req = ack_req;
    op->u.put.target_id = target_id;
    op->u.put.pt_index = pt_index;
    op->u.put.match_bits = match_bits;
    op->u.put.remote_offset = remote_offset;
    op->u.put.user_ptr = user_ptr;
    op->u.put.hdr_data = hdr_data;

    /* place to store the send and ack events */
    op->u.put.send = NULL;
    op->u.put.ack = NULL;
    op->u.put.pt_type = pt_type;
    op->events_ready = 0;
    op->target = target;

    if (op->u.put.pt_type == RPTL_PT_DATA)
        MPL_DL_APPEND(target->data_op_list, op);
    else
        MPL_DL_APPEND(target->control_op_list, op);

    ret = poke_progress();
    RPTLU_ERR_POP(ret, "Error from poke_progress\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_RPTL_PUT);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME rptl_put
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_put(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                          ptl_ack_req_t ack_req, ptl_process_t target_id, ptl_pt_index_t pt_index,
                          ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr,
                          ptl_hdr_data_t hdr_data)
{
    return rptl_put(md_handle, local_offset, length, ack_req, target_id, pt_index, match_bits,
                    remote_offset, user_ptr, hdr_data, RPTL_PT_DATA);
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_get
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_get(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                          ptl_process_t target_id, ptl_pt_index_t pt_index,
                          ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr)
{
    struct rptl_op *op;
    int ret = PTL_OK;
    struct rptl_target *target;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_GET);

    ret = find_target(target_id, &target);
    RPTLU_ERR_POP(ret, "error finding target structure\n");

    ret = rptli_op_alloc(&op, target);
    RPTLU_ERR_POP(ret, "error allocating op\n");

    op->op_type = RPTL_OP_GET;
    op->state = RPTL_OP_STATE_QUEUED;

    /* store the user parameters */
    op->u.get.md_handle = md_handle;
    op->u.get.local_offset = local_offset;
    op->u.get.length = length;
    op->u.get.target_id = target_id;
    op->u.get.pt_index = pt_index;
    op->u.get.match_bits = match_bits;
    op->u.get.remote_offset = remote_offset;
    op->u.get.user_ptr = user_ptr;

    op->events_ready = 0;
    op->target = target;

    MPL_DL_APPEND(target->data_op_list, op);

    ret = poke_progress();
    RPTLU_ERR_POP(ret, "Error from poke_progress\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_GET);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_pause_messages
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int send_pause_messages(struct rptl *rptl)
{
    int i, mpi_errno = MPI_SUCCESS;
    ptl_process_t id;
    ptl_pt_index_t data_pt, control_pt;
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_SEND_PAUSE_MESSAGES);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_PAUSE_MESSAGES);

    /* if no control portal is setup for this rptl, we are doomed */
    assert(rptl->control.pt != PTL_PT_ANY);

    /* set the max message count in the overflow buffers we can keep
     * before sending the unpause messages */
    rptl->data.ob_max_count = rptl->data.ob_curr_count / 2;

    for (i = 0; i < rptl_info.world_size; i++) {
        if (i == MPIDI_Process.my_pg_rank)
            continue;
        mpi_errno = rptl_info.get_target_info(i, &id, rptl->data.pt, &data_pt, &control_pt);
        if (mpi_errno) {
            ret = PTL_FAIL;
            RPTLU_ERR_POP(ret, "Error getting target info while sending pause messages\n");
        }

        /* make sure the user setup a control portal */
        assert(control_pt != PTL_PT_ANY);

        ret = rptl_put(rptl->md, 0, 0, PTL_NO_ACK_REQ, id, control_pt, 0, 0,
                                    NULL, RPTL_CONTROL_MSG_PAUSE, RPTL_PT_CONTROL);
        RPTLU_ERR_POP(ret, "Error sending pause message\n");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_PAUSE_MESSAGES);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME clear_nacks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int clear_nacks(ptl_process_t target_id)
{
    struct rptl_target *target;
    struct rptl_op *op;
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_CLEAR_NACKS);

    MPIDI_FUNC_ENTER(MPID_STATE_CLEAR_NACKS);

    ret = find_target(target_id, &target);
    RPTLU_ERR_POP(ret, "error finding target\n");

    for (op = target->data_op_list; op; op = op->next) {
        if ((op->op_type == RPTL_OP_PUT && IDS_ARE_EQUAL(op->u.put.target_id, target_id)) ||
            (op->op_type == RPTL_OP_GET && IDS_ARE_EQUAL(op->u.get.target_id, target_id))) {
            if (op->state == RPTL_OP_STATE_NACKED)
                op->state = RPTL_OP_STATE_QUEUED;
        }
    }
    target->state = RPTL_TARGET_STATE_ACTIVE;

    ret = poke_progress();
    RPTLU_ERR_POP(ret, "error in poke_progress\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_CLEAR_NACKS);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME get_event_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int get_event_info(ptl_event_t * event, struct rptl **ret_rptl, struct rptl_op **ret_op)
{
    struct rptl *rptl;
    struct rptl_op *op;
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_GET_EVENT_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_EVENT_INFO);

    if (event->type == PTL_EVENT_SEND || event->type == PTL_EVENT_REPLY ||
        event->type == PTL_EVENT_ACK) {
        op = (struct rptl_op *) event->user_ptr;

        rptl_info.origin_events_left++;
        if (event->type != PTL_EVENT_SEND)
            op->target->issued_data_ops--;

        /* see if there are any pending ops to be issued */
        ret = poke_progress();
        RPTLU_ERR_POP(ret, "Error returned from poke_progress\n");

        assert(op);
        rptl = NULL;
    }
    else {
        /* for all target-side events, we look up the rptl based on
         * the pt_index */
        for (rptl = rptl_info.rptl_list; rptl; rptl = rptl->next)
            if (rptl->data.pt == event->pt_index || rptl->control.pt == event->pt_index)
                break;

        assert(rptl);
        op = NULL;
    }

    *ret_rptl = rptl;
    *ret_op = op;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_GET_EVENT_INFO);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME stash_event
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int stash_event(struct rptl_op *op, ptl_event_t event)
{
    int mpi_errno = MPI_SUCCESS;
    int ret = PTL_OK;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_STASH_EVENT);

    MPIDI_FUNC_ENTER(MPID_STATE_STASH_EVENT);

    /* make sure this is of the event type we know of */
    assert(event.type == PTL_EVENT_SEND || event.type == PTL_EVENT_ACK);

    /* only PUT events are stashed */
    assert(op->op_type == RPTL_OP_PUT);

    /* we should never stash anything when we are in events ready */
    assert(op->events_ready == 0);

    /* only one of send or ack is stashed.  if we are in this
     * function, both the events should be NULL at this point. */
    assert(op->u.put.send == NULL && op->u.put.ack == NULL);

    if (event.type == PTL_EVENT_SEND) {
        MPIU_CHKPMEM_MALLOC(op->u.put.send, ptl_event_t *, sizeof(ptl_event_t), mpi_errno,
                            "ptl event");
        memcpy(op->u.put.send, &event, sizeof(ptl_event_t));
    }
    else {
        MPIU_CHKPMEM_MALLOC(op->u.put.ack, ptl_event_t *, sizeof(ptl_event_t), mpi_errno,
                            "ptl event");
        memcpy(op->u.put.ack, &event, sizeof(ptl_event_t));
    }

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_STASH_EVENT);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


static ptl_event_t pending_event;
static int pending_event_valid = 0;

#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_eqget
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_eqget(ptl_handle_eq_t eq_handle, ptl_event_t * event)
{
    struct rptl_op *op = NULL;
    struct rptl *rptl = NULL;
    int ret = PTL_OK, tmp_ret = PTL_OK;
    int mpi_errno = MPI_SUCCESS;
    struct rptl_target *target;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_EQGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_EQGET);

    ret = poke_progress();
    RPTLU_ERR_POP(ret, "error poking progress\n");

    /* before we poll the eq, we need to check if there are any
     * completed operations that need to be returned */
    if (pending_event_valid) {
        memcpy(event, &pending_event, sizeof(ptl_event_t));
        pending_event_valid = 0;
        ret = PTL_OK;
        goto fn_exit;
    }

    ret = PtlEQGet(eq_handle, event);
    if (ret == PTL_EQ_EMPTY)
        goto fn_exit;

    /* find the rptl and op associated with this event */
    tmp_ret = get_event_info(event, &rptl, &op);
    if (tmp_ret) {
        ret = tmp_ret;
        RPTLU_ERR_POP(ret, "Error returned from get_event_info\n");
    }

    /* PT_DISABLED events only occur on the target */
    if (event->type == PTL_EVENT_PT_DISABLED) {
        /* we hide PT disabled events from the user */
        ret = PTL_EQ_EMPTY;

        /* we should only receive disable events on the data pt */
        assert(rptl->data.pt == event->pt_index);

        /* if we don't have a control PT, we don't have a way to
         * recover from disable events */
        assert(rptl->control.pt != PTL_PT_ANY);

        if (rptl->local_state == RPTL_LOCAL_STATE_ACTIVE) {
            rptl->local_state = RPTL_LOCAL_STATE_AWAITING_PAUSE_ACKS;
            rptl->pause_ack_counter = 0;

            /* send out pause messages */
            tmp_ret = send_pause_messages(rptl);
            if (tmp_ret) {
                ret = tmp_ret;
                RPTLU_ERR_POP(ret, "Error returned from send_pause_messages\n");
            }
        }
    }

    /* PUT_OVERFLOW events only occur on the target and only for the
     * data portal */
    else if (event->type == PTL_EVENT_PUT_OVERFLOW || event->type == PTL_EVENT_GET_OVERFLOW) {
        /* something is being pulled out of the overflow buffer,
         * decrement counter */
        rptl->data.ob_curr_count--;

        /* we should only receive disable events on the data pt */
        assert(rptl->data.pt == event->pt_index);
    }

    /* PUT events only occur on the target */
    else if (event->type == PTL_EVENT_PUT || event->type == PTL_EVENT_GET) {
        if (rptl->data.pt == event->pt_index) {
            /* if the event is in the OVERFLOW list, then it means we
             * just got a match in there */
            if (event->ptl_list == PTL_OVERFLOW_LIST)
                rptl->data.ob_curr_count++;
            goto fn_exit;
        }

        /* control PT should never see a GET event */
        assert(event->type == PTL_EVENT_PUT);

        /* else, this message is on the control PT, so hide this event
         * from the user */
        ret = PTL_EQ_EMPTY;

        /* the message came in on the control PT, repost it */
        tmp_ret = rptli_post_control_buffer(rptl->ni, rptl->control.pt,
                                    &rptl->control.me[rptl->control.me_idx]);
        if (tmp_ret) {
            ret = tmp_ret;
            RPTLU_ERR_POP(ret, "Error returned from rptli_post_control_buffer\n");
        }
        rptl->control.me_idx++;
        if (rptl->control.me_idx >= 2 * rptl_info.world_size)
            rptl->control.me_idx = 0;

        if (event->hdr_data == RPTL_CONTROL_MSG_PAUSE) {
            tmp_ret = find_target(event->initiator, &target);
            if (tmp_ret) {
                ret = tmp_ret;
                RPTLU_ERR_POP(ret, "Error finding target\n");
            }
            assert(target->state < RPTL_TARGET_STATE_RECEIVED_PAUSE);
            target->state = RPTL_TARGET_STATE_RECEIVED_PAUSE;
            target->rptl = rptl;
        }
        else if (event->hdr_data == RPTL_CONTROL_MSG_PAUSE_ACK) {
            rptl->pause_ack_counter++;
        }
        else {  /* got an UNPAUSE message */
            /* clear NACKs from all operations to this target and poke
             * progress */
            tmp_ret = clear_nacks(event->initiator);
            if (tmp_ret) {
                ret = tmp_ret;
                RPTLU_ERR_POP(ret, "Error returned from clear_nacks\n");
            }
        }
    }

    /* origin side events */
    else if (event->type == PTL_EVENT_SEND || event->type == PTL_EVENT_ACK ||
             event->type == PTL_EVENT_REPLY) {

        /* if this is a failed event, we simply drop this event */
        if (event->ni_fail_type == PTL_NI_PT_DISABLED) {
            /* hide the event from the user */
            ret = PTL_EQ_EMPTY;

            /* we should not get NACKs on the control portal */
            if (event->type == PTL_EVENT_ACK)
                assert(op->u.put.pt_type == RPTL_PT_DATA);

            op->state = RPTL_OP_STATE_NACKED;

            if (op->op_type == RPTL_OP_PUT) {
                assert(!(event->type == PTL_EVENT_SEND && op->u.put.send));
                assert(!(event->type == PTL_EVENT_ACK && op->u.put.ack));

                /* discard pending events, since we will retransmit
                 * this op anyway */
                if (op->u.put.ack) {
                    MPIU_Free(op->u.put.ack);
                    op->u.put.ack = NULL;
                }
                if (op->u.put.send) {
                    MPIU_Free(op->u.put.send);
                    op->u.put.send = NULL;
                }
            }

            if (op->op_type == RPTL_OP_PUT)
                tmp_ret = find_target(op->u.put.target_id, &target);
            else
                tmp_ret = find_target(op->u.get.target_id, &target);
            if (tmp_ret) {
                ret = tmp_ret;
                RPTLU_ERR_POP(ret, "Error finding target\n");
            }

            if (target->state == RPTL_TARGET_STATE_ACTIVE) {
                target->state = RPTL_TARGET_STATE_DISABLED;
                target->rptl = NULL;
            }
        }

        /* if this is a REPLY event, we are done with this op */
        else if (event->type == PTL_EVENT_REPLY) {
            assert(op->op_type == RPTL_OP_GET);

            event->user_ptr = op->u.get.user_ptr;

            /* GET operations only go into the data op list */
            MPL_DL_DELETE(op->target->data_op_list, op);
            rptli_op_free(op);
        }

        else if (event->type == PTL_EVENT_SEND && op->u.put.ack) {
            assert(op->op_type == RPTL_OP_PUT);

            /* we already got the other event we needed earlier.  mark
             * the op events as ready and return this current event to
             * the user. */
            op->events_ready = 1;
            event->user_ptr = op->u.put.user_ptr;

            /* if the message is over the control portal, ignore both
             * events */
            if (op->u.put.pt_type == RPTL_PT_CONTROL) {
                /* drop the ack event */
                MPIU_Free(op->u.put.ack);
                MPL_DL_DELETE(op->target->control_op_list, op);
                rptli_op_free(op);

                /* drop the send event */
                ret = PTL_EQ_EMPTY;
            }
            else {
                /* if the message is over the data portal, we'll
                 * return the send event.  if the user asked for an
                 * ACK, we will enqueue the ack to be returned
                 * next. */
                if (op->u.put.ack_req & PTL_ACK_REQ) {
                    /* only one event should be pending */
                    assert(pending_event_valid == 0);
                    memcpy(&pending_event, op->u.put.ack, sizeof(ptl_event_t));
                    pending_event_valid = 1;
                }
                MPIU_Free(op->u.put.ack);
                MPL_DL_DELETE(op->target->data_op_list, op);
                rptli_op_free(op);
            }
        }

        else if (event->type == PTL_EVENT_ACK && op->u.put.send) {
            assert(op->op_type == RPTL_OP_PUT);

            /* we already got the other event we needed earlier.  mark
             * the op events as ready and return this current event to
             * the user. */
            op->events_ready = 1;
            event->user_ptr = op->u.put.user_ptr;

            /* if the message is over the control portal, ignore both
             * events */
            if (op->u.put.pt_type == RPTL_PT_CONTROL) {
                /* drop the send event */
                MPIU_Free(op->u.put.send);
                MPL_DL_DELETE(op->target->control_op_list, op);
                rptli_op_free(op);

                /* drop the ack event */
                ret = PTL_EQ_EMPTY;
            }
            else {
                /* if the message is over the data portal, we'll
                 * return the send event.  if the user asked for an
                 * ACK, we will enqueue the ack to be returned
                 * next. */
                if (op->u.put.ack_req & PTL_ACK_REQ) {
                    /* user asked for an ACK, so return it to the user
                     * and queue up the SEND event for next time */
                    memcpy(&pending_event, op->u.put.send, sizeof(ptl_event_t));
                    MPIU_Free(op->u.put.send);
                    assert(pending_event_valid == 0);
                    pending_event_valid = 1;
                }
                else {
                    /* user didn't ask for an ACK, overwrite the ACK
                     * event with the pending send event */
                    memcpy(event, op->u.put.send, sizeof(ptl_event_t));
                    MPIU_Free(op->u.put.send);

                    /* set the event user pointer again, since we
                     * copied over the original event */
                    event->user_ptr = op->u.put.user_ptr;
                }
                /* we should be in the data op list */
                MPL_DL_DELETE(op->target->data_op_list, op);
                rptli_op_free(op);
            }
        }

        else {
            assert(!(event->type == PTL_EVENT_SEND && op->u.put.send));
            assert(!(event->type == PTL_EVENT_ACK && op->u.put.ack));

            /* stash this event as we need to wait for the buddy event
             * as well before returning to the user */
            ret = stash_event(op, *event);
            RPTLU_ERR_POP(ret, "error stashing event\n");
            ret = PTL_EQ_EMPTY;
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_EQGET);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    goto fn_exit;
}
