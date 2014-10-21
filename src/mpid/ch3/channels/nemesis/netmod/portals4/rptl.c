/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "rptl.h"

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

#define RPTL_OP_POOL_SEGMENT_COUNT  (1024)

static struct {
    struct rptl *rptl_list;

    struct rptl_op_pool_segment {
        struct rptl_op op[RPTL_OP_POOL_SEGMENT_COUNT];
        struct rptl_op_pool_segment *next;
        struct rptl_op_pool_segment *prev;
    } *op_segment_list;
    struct rptl_op *op_pool;

    struct rptl_op *op_list;

    /* targets that we do not send messages to either because they
     * sent a PAUSE message or because we received a NACK from them */
    struct rptl_paused_target {
        ptl_process_t id;
        enum rptl_paused_target_state {
            RPTL_TARGET_STATE_FLOWCONTROL,
            RPTL_TARGET_STATE_DISABLED,
            RPTL_TARGET_STATE_RECEIVED_PAUSE,
            RPTL_TARGET_STATE_PAUSE_ACKED
        } state;

        /* the rptl on which the pause message came in, since we need
         * to use it to send the pause ack to the right target
         * portal */
        struct rptl *rptl;

        struct rptl_paused_target *next;
        struct rptl_paused_target *prev;
    } *paused_target_list;

    int world_size;
    uint64_t origin_events_left;
    int (*get_target_info) (int rank, ptl_process_t * id, ptl_pt_index_t local_data_pt,
                            ptl_pt_index_t * target_data_pt, ptl_pt_index_t * target_control_pt);
} rptl_info;


#undef FUNCNAME
#define FUNCNAME alloc_target
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int alloc_target(ptl_process_t id, enum rptl_paused_target_state state, struct rptl *rptl)
{
    int mpi_errno = MPI_SUCCESS;
    int ret = PTL_OK;
    struct rptl_paused_target *target;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_ALLOC_TARGET);

    MPIDI_FUNC_ENTER(MPID_STATE_ALLOC_TARGET);

    for (target = rptl_info.paused_target_list; target; target = target->next)
        if (IDS_ARE_EQUAL(target->id, id))
            break;

    /* if a paused target does not already exist, create one */
    if (target == NULL) {
        /* create a new paused target */
        MPIU_CHKPMEM_MALLOC(target, struct rptl_paused_target *, sizeof(struct rptl_paused_target),
                            mpi_errno, "rptl paused target");
        MPL_DL_APPEND(rptl_info.paused_target_list, target);

        target->id = id;
        target->state = state;
        target->rptl = rptl;
    }
    else if (target->state < state) {
        target->state = state;
        target->rptl = rptl;
    }
    else {
        /* target already exists and is in a higher state than the
         * state we are trying to set.  e.g., this is possible if we
         * got a PAUSE event from a different portal and acked. */
    }

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_ALLOC_TARGET);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME alloc_op_segment
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int alloc_op_segment(void)
{
    struct rptl_op_pool_segment *op_segment;
    int mpi_errno = MPI_SUCCESS;
    int i;
    int ret = PTL_OK;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_ALLOC_OP_SEGMENT);

    MPIDI_FUNC_ENTER(MPID_STATE_ALLOC_OP_SEGMENT);

    MPIU_CHKPMEM_MALLOC(op_segment, struct rptl_op_pool_segment *, sizeof(struct rptl_op_pool_segment),
                        mpi_errno, "op pool segment");
    MPL_DL_APPEND(rptl_info.op_segment_list, op_segment);

    for (i = 0; i < RPTL_OP_POOL_SEGMENT_COUNT; i++)
        MPL_DL_APPEND(rptl_info.op_pool, &op_segment->op[i]);

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_ALLOC_OP_SEGMENT);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_init(int world_size, uint64_t max_origin_events,
                           int (*get_target_info) (int rank, ptl_process_t * id,
                                                   ptl_pt_index_t local_data_pt,
                                                   ptl_pt_index_t * target_data_pt,
                                                   ptl_pt_index_t * target_control_pt))
{
    int mpi_errno = MPI_SUCCESS;
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_INIT);

    rptl_info.rptl_list = NULL;

    rptl_info.op_pool = NULL;
    ret = alloc_op_segment();
    RPTLU_ERR_POP(ret, "error allocating op segment\n");

    rptl_info.op_list = NULL;

    rptl_info.paused_target_list = NULL;
    rptl_info.world_size = world_size;
    rptl_info.origin_events_left = max_origin_events;
    rptl_info.get_target_info = get_target_info;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_INIT);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_drain_eq
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_drain_eq(int eq_count, ptl_handle_eq_t *eq)
{
    int ret = PTL_OK;
    ptl_event_t event;
    struct rptl_op_pool_segment *op_segment;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_FINALIZE);

    while (rptl_info.op_list) {
        for (i = 0; i < eq_count; i++) {
            /* read and ignore all events */
            ret = MPID_nem_ptl_rptl_eqget(eq[i], &event);
            if (ret == PTL_EQ_EMPTY)
                ret = PTL_OK;
            RPTLU_ERR_POP(ret, "Error calling MPID_nem_ptl_rptl_eqget\n");
        }
    }

    while (rptl_info.op_segment_list) {
        op_segment = rptl_info.op_segment_list;
        MPL_DL_DELETE(rptl_info.op_segment_list, op_segment);
        MPIU_Free(op_segment);
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_FINALIZE);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME post_empty_buffer
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int post_empty_buffer(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt,
                                    ptl_handle_me_t * me_handle)
{
    int ret;
    ptl_me_t me;
    ptl_process_t id;
    MPIDI_STATE_DECL(MPID_STATE_POST_EMPTY_BUFFER);

    MPIDI_FUNC_ENTER(MPID_STATE_POST_EMPTY_BUFFER);

    id.phys.nid = PTL_NID_ANY;
    id.phys.pid = PTL_PID_ANY;

    me.start = NULL;
    me.length = 0;
    me.ct_handle = PTL_CT_NONE;
    me.uid = PTL_UID_ANY;
    me.options = (PTL_ME_OP_PUT | PTL_ME_OP_GET | PTL_ME_USE_ONCE | PTL_ME_IS_ACCESSIBLE |
                  PTL_ME_EVENT_LINK_DISABLE | PTL_ME_EVENT_UNLINK_DISABLE);
    me.match_id = id;
    me.match_bits = 0;
    me.ignore_bits = 0;
    me.min_free = 0;

    ret = PtlMEAppend(ni_handle, pt, &me, PTL_PRIORITY_LIST, NULL, me_handle);
    RPTLU_ERR_POP(ret, "Error appending empty buffer to priority list\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_POST_EMPTY_BUFFER);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_ptinit
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_ptinit(ptl_handle_ni_t ni_handle, ptl_handle_eq_t eq_handle, ptl_pt_index_t data_pt,
                             ptl_pt_index_t control_pt)
{
    int ret = PTL_OK;
    struct rptl *rptl;
    int mpi_errno = MPI_SUCCESS;
    int i;
    ptl_md_t md;
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_PTINIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_PTINIT);


    /* setup the parts of rptls that can be done before world size or
     * target information */
    MPIU_CHKPMEM_MALLOC(rptl, struct rptl *, sizeof(struct rptl), mpi_errno, "rptl");
    MPL_DL_APPEND(rptl_info.rptl_list, rptl);

    rptl->local_state = RPTL_LOCAL_STATE_NORMAL;
    rptl->pause_ack_counter = 0;

    rptl->data.ob_max_count = 0;
    rptl->data.ob_curr_count = 0;

    rptl->data.pt = data_pt;
    rptl->control.pt = control_pt;

    rptl->ni = ni_handle;
    rptl->eq = eq_handle;

    md.start = 0;
    md.length = (ptl_size_t) (-1);
    md.options = 0x0;
    md.eq_handle = rptl->eq;
    md.ct_handle = PTL_CT_NONE;
    ret = PtlMDBind(rptl->ni, &md, &rptl->md);
    RPTLU_ERR_POP(ret, "Error binding new global MD\n");

    /* post world_size number of empty buffers on the control portal */
    if (rptl->control.pt != PTL_PT_ANY) {
        MPIU_CHKPMEM_MALLOC(rptl->control.me, ptl_handle_me_t *,
                            2 * rptl_info.world_size * sizeof(ptl_handle_me_t), mpi_errno,
                            "rptl target info");
        for (i = 0; i < 2 * rptl_info.world_size; i++) {
            ret = post_empty_buffer(rptl->ni, rptl->control.pt, &rptl->control.me[i]);
            RPTLU_ERR_POP(ret, "Error in post_empty_buffer\n");
        }
        rptl->control.me_idx = 0;
    }

  fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_PTINIT);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_ptfini
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_ptfini(ptl_pt_index_t pt_index)
{
    int i;
    int ret = PTL_OK;
    struct rptl *rptl;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_PTFINI);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_PTFINI);

    /* find the right rptl */
    for (rptl = rptl_info.rptl_list; rptl && rptl->data.pt != pt_index; rptl = rptl->next);
    assert(rptl);

    /* free control portals that were created */
    if (rptl->control.pt != PTL_PT_ANY) {
        for (i = 0; i < rptl_info.world_size * 2; i++) {
            ret = PtlMEUnlink(rptl->control.me[i]);
            RPTLU_ERR_POP(ret, "Error unlinking control buffers\n");
        }
        MPIU_Free(rptl->control.me);
    }

    MPL_DL_DELETE(rptl_info.rptl_list, rptl);
    MPIU_Free(rptl);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_PTFINI);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME alloc_op
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int alloc_op(struct rptl_op **op)
{
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_ALLOC_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_ALLOC_OP);

    if (rptl_info.op_pool == NULL) {
        ret = alloc_op_segment();
        RPTLU_ERR_POP(ret, "error allocating op segment\n");
    }

    *op = rptl_info.op_pool;
    MPL_DL_DELETE(rptl_info.op_pool, *op);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_ALLOC_OP);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME free_op
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void free_op(struct rptl_op *op)
{
    MPIDI_STATE_DECL(MPID_STATE_FREE_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_FREE_OP);

    MPL_DL_APPEND(rptl_info.op_pool, op);

    MPIDI_FUNC_EXIT(MPID_STATE_FREE_OP);
}


#undef FUNCNAME
#define FUNCNAME issue_op
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int issue_op(struct rptl_op *op)
{
    int ret = PTL_OK;
    struct rptl_paused_target *target;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_ISSUE_OP);

    if (op->op_type == RPTL_OP_PUT) {
        for (target = rptl_info.paused_target_list; target; target = target->next)
            if (IDS_ARE_EQUAL(target->id, op->u.put.target_id))
                break;

        if (target && op->u.put.flow_control)
            goto fn_exit;

        if (rptl_info.origin_events_left < 2) {
            ret = alloc_target(op->u.put.target_id, RPTL_TARGET_STATE_FLOWCONTROL, NULL);
            RPTLU_ERR_POP(ret, "error allocating paused target\n");
            goto fn_exit;
        }
        rptl_info.origin_events_left -= 2;

        /* force request for an ACK even if the user didn't ask for
         * it.  replace the user pointer with the OP id. */
        ret =
            PtlPut(op->u.put.md_handle, op->u.put.local_offset, op->u.put.length,
                   PTL_ACK_REQ, op->u.put.target_id, op->u.put.pt_index,
                   op->u.put.match_bits, op->u.put.remote_offset, op,
                   op->u.put.hdr_data);
        RPTLU_ERR_POP(ret, "Error issuing PUT\n");
    }
    else {
        for (target = rptl_info.paused_target_list; target; target = target->next)
            if (IDS_ARE_EQUAL(target->id, op->u.get.target_id))
                break;

        if (target)
            goto fn_exit;

        if (rptl_info.origin_events_left < 1) {
            ret = alloc_target(op->u.get.target_id, RPTL_TARGET_STATE_FLOWCONTROL, NULL);
            RPTLU_ERR_POP(ret, "error allocating paused target\n");
            goto fn_exit;
        }
        rptl_info.origin_events_left--;

        ret =
            PtlGet(op->u.get.md_handle, op->u.get.local_offset, op->u.get.length,
                   op->u.get.target_id, op->u.get.pt_index, op->u.get.match_bits,
                   op->u.get.remote_offset, op);
        RPTLU_ERR_POP(ret, "Error issuing GET\n");
    }

    op->state = RPTL_OP_STATE_ISSUED;

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_ISSUE_OP);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_put
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_put(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                          ptl_ack_req_t ack_req, ptl_process_t target_id, ptl_pt_index_t pt_index,
                          ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr,
                          ptl_hdr_data_t hdr_data, int flow_control)
{
    struct rptl_op *op;
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_PUT);

    ret = alloc_op(&op);
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
    op->u.put.flow_control = flow_control;
    op->events_ready = 0;

    MPL_DL_APPEND(rptl_info.op_list, op);

    /* if we are not in a PAUSED state, issue the operation */
    ret = issue_op(op);
    RPTLU_ERR_POP(ret, "Error from issue_op\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_PUT);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_get
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_get(ptl_handle_md_t md_handle, ptl_size_t local_offset, ptl_size_t length,
                          ptl_process_t target_id, ptl_pt_index_t pt_index,
                          ptl_match_bits_t match_bits, ptl_size_t remote_offset, void *user_ptr)
{
    struct rptl_op *op;
    int ret = PTL_OK;
    struct rptl_paused_target *target;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_GET);

    ret = alloc_op(&op);
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

    MPL_DL_APPEND(rptl_info.op_list, op);

    for (target = rptl_info.paused_target_list; target; target = target->next)
        if (IDS_ARE_EQUAL(target->id, target_id))
            break;

    ret = issue_op(op);
    RPTLU_ERR_POP(ret, "Error from issue_op\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_GET);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_pause_messages
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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
        mpi_errno = rptl_info.get_target_info(i, &id, rptl->data.pt, &data_pt, &control_pt);
        if (mpi_errno) {
            ret = PTL_FAIL;
            RPTLU_ERR_POP(ret, "Error getting target info while sending pause messages\n");
        }

        /* disable flow control for control messages */
        ret = MPID_nem_ptl_rptl_put(rptl->md, 0, 0, PTL_NO_ACK_REQ, id, control_pt, 0, 0,
                                    NULL, RPTL_CONTROL_MSG_PAUSE, 0);
        RPTLU_ERR_POP(ret, "Error sending pause message\n");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_PAUSE_MESSAGES);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_pause_ack_messages
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int send_pause_ack_messages(void)
{
    struct rptl_op *op;
    int ret = PTL_OK;
    struct rptl_paused_target *target;
    MPIDI_STATE_DECL(MPID_STATE_SEND_PAUSE_ACK_MESSAGES);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_PAUSE_ACK_MESSAGES);

    for (target = rptl_info.paused_target_list; target; target = target->next) {
        if (target->state != RPTL_TARGET_STATE_RECEIVED_PAUSE)
            continue;

        for (op = rptl_info.op_list; op; op = op->next) {
            if (op->op_type == RPTL_OP_GET && IDS_ARE_EQUAL(op->u.get.target_id, target->id) &&
                op->state == RPTL_OP_STATE_ISSUED)
                break;

            if (op->op_type == RPTL_OP_PUT && IDS_ARE_EQUAL(op->u.put.target_id, target->id)) {
                if (op->state == RPTL_OP_STATE_ISSUED)
                    break;
                if (op->u.put.send || op->u.put.ack)
                    break;
            }
        }

        if (op == NULL) {
            ptl_process_t id;
            ptl_pt_index_t data_pt, control_pt;
            int i;
            int mpi_errno = MPI_SUCCESS;

            for (i = 0; i < rptl_info.world_size; i++) {
                /* find the target that has this target id and get the
                 * control portal information for it */
                mpi_errno = rptl_info.get_target_info(i, &id, target->rptl->data.pt, &data_pt, &control_pt);
                if (mpi_errno) {
                    ret = PTL_FAIL;
                    RPTLU_ERR_POP(ret,
                                  "Error getting target info while sending pause ack message\n");
                }
                if (IDS_ARE_EQUAL(id, target->id))
                    break;
            }

            /* disable flow control for control messages */
            ret =
                MPID_nem_ptl_rptl_put(target->rptl->md, 0, 0, PTL_NO_ACK_REQ, id, control_pt, 0,
                                      0, NULL, RPTL_CONTROL_MSG_PAUSE_ACK, 0);
            RPTLU_ERR_POP(ret, "Error sending pause ack message\n");

            if (target->state == RPTL_TARGET_STATE_RECEIVED_PAUSE)
                target->state = RPTL_TARGET_STATE_PAUSE_ACKED;
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_PAUSE_ACK_MESSAGES);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_unpause_messages
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int send_unpause_messages(void)
{
    int i, mpi_errno = MPI_SUCCESS;
    ptl_process_t id;
    ptl_pt_index_t data_pt, control_pt;
    int ret = PTL_OK;
    struct rptl *rptl;
    MPIDI_STATE_DECL(MPID_STATE_SEND_UNPAUSE_MESSAGES);

    MPIDI_FUNC_ENTER(MPID_STATE_SEND_UNPAUSE_MESSAGES);

    for (rptl = rptl_info.rptl_list; rptl; rptl = rptl->next) {
        assert(rptl->local_state != RPTL_LOCAL_STATE_AWAITING_PAUSE_ACKS ||
               rptl->control.pt != PTL_PT_ANY);
        if (rptl->control.pt == PTL_PT_ANY)
            continue;
        if (rptl->local_state != RPTL_LOCAL_STATE_AWAITING_PAUSE_ACKS)
            continue;

        if (rptl->pause_ack_counter == rptl_info.world_size) {
            /* if we are over the max count limit, do not send an
             * unpause message yet */
            if (rptl->data.ob_curr_count > rptl->data.ob_max_count)
                goto fn_exit;

            ret = PtlPTEnable(rptl->ni, rptl->data.pt);
            RPTLU_ERR_POP(ret, "Error returned while reenabling PT\n");

            rptl->local_state = RPTL_LOCAL_STATE_NORMAL;

            for (i = 0; i < rptl_info.world_size; i++) {
                mpi_errno = rptl_info.get_target_info(i, &id, rptl->data.pt, &data_pt, &control_pt);
                if (mpi_errno) {
                    ret = PTL_FAIL;
                    RPTLU_ERR_POP(ret,
                                  "Error getting target info while sending unpause messages\n");
                }

                /* disable flow control for control messages */
                ret =
                    MPID_nem_ptl_rptl_put(rptl->md, 0, 0, PTL_NO_ACK_REQ, id, control_pt,
                                          0, 0, NULL, RPTL_CONTROL_MSG_UNPAUSE, 0);
                RPTLU_ERR_POP(ret, "Error sending unpause message\n");
            }
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_SEND_UNPAUSE_MESSAGES);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME reissue_ops
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int reissue_ops(ptl_process_t target_id)
{
    struct rptl_paused_target *target;
    struct rptl_op *op;
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_REISSUE_OPS);

    MPIDI_FUNC_ENTER(MPID_STATE_REISSUE_OPS);

    for (target = rptl_info.paused_target_list; target; target = target->next)
        if (IDS_ARE_EQUAL(target->id, target_id))
            break;
    assert(target);

    MPL_DL_DELETE(rptl_info.paused_target_list, target);
    MPIU_Free(target);

    for (op = rptl_info.op_list; op; op = op->next) {
        if ((op->op_type == RPTL_OP_PUT && IDS_ARE_EQUAL(op->u.put.target_id, target_id)) ||
            (op->op_type == RPTL_OP_GET && IDS_ARE_EQUAL(op->u.get.target_id, target_id))) {
            if (op->state != RPTL_OP_STATE_ISSUED) {
                ret = issue_op(op);
                RPTLU_ERR_POP(ret, "Error calling issue_op\n");
            }
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_REISSUE_OPS);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME get_event_info
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static void get_event_info(ptl_event_t * event, struct rptl **ret_rptl, struct rptl_op **ret_op)
{
    struct rptl *rptl;
    struct rptl_op *op;
    struct rptl_paused_target *target, *tmp;
    MPIDI_STATE_DECL(MPID_STATE_GET_EVENT_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_EVENT_INFO);

    if (event->type == PTL_EVENT_SEND || event->type == PTL_EVENT_REPLY ||
        event->type == PTL_EVENT_ACK) {
        op = (struct rptl_op *) event->user_ptr;

        rptl_info.origin_events_left++;

        if (rptl_info.origin_events_left >= 2) {
            for (target = rptl_info.paused_target_list; target;) {
                if (target->state == RPTL_TARGET_STATE_FLOWCONTROL) {
                    tmp = target->next;
                    MPL_DL_DELETE(rptl_info.paused_target_list, target);
                    MPIU_Free(target);
                    target = tmp;
                }
                else
                    target = target->next;
            }
        }

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
    return;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME stash_event
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
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


#undef FUNCNAME
#define FUNCNAME retrieve_event
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static void retrieve_event(struct rptl *rptl, struct rptl_op *op, ptl_event_t * event)
{
    MPIDI_STATE_DECL(MPID_STATE_RETRIEVE_EVENT);

    MPIDI_FUNC_ENTER(MPID_STATE_RETRIEVE_EVENT);

    assert(op->op_type == RPTL_OP_PUT);
    assert(op->u.put.send || op->u.put.ack);

    if (op->u.put.send) {
        memcpy(event, op->u.put.send, sizeof(ptl_event_t));
        MPIU_Free(op->u.put.send);
    }
    else {
        memcpy(event, op->u.put.ack, sizeof(ptl_event_t));
        MPIU_Free(op->u.put.ack);
    }
    event->user_ptr = op->u.put.user_ptr;

    MPL_DL_DELETE(rptl_info.op_list, op);
    free_op(op);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_RETRIEVE_EVENT);
    return;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME issue_pending_ops
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int issue_pending_ops(void)
{
    struct rptl_paused_target *target, *tmp;
    struct rptl_op *op;
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_ISSUE_PENDING_OPS);

    MPIDI_FUNC_ENTER(MPID_STATE_ISSUE_PENDING_OPS);

    for (op = rptl_info.op_list; op; op = op->next) {
        if (op->state == RPTL_OP_STATE_QUEUED) {
            for (target = rptl_info.paused_target_list; target; target = target->next)
                if ((op->op_type == RPTL_OP_PUT && IDS_ARE_EQUAL(op->u.put.target_id, target->id)) ||
                    (op->op_type == RPTL_OP_GET && IDS_ARE_EQUAL(op->u.get.target_id, target->id)))
                    break;
            if (target == NULL) {
                ret = issue_op(op);
                RPTLU_ERR_POP(ret, "error issuing op\n");
            }
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_ISSUE_PENDING_OPS);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_eqget
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_eqget(ptl_handle_eq_t eq_handle, ptl_event_t * event)
{
    struct rptl_op *op;
    struct rptl *rptl;
    ptl_event_t e;
    int ret = PTL_OK, tmp_ret = PTL_OK;
    struct rptl_paused_target *target;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_EQGET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_EQGET);

    /* before we poll the eq, we need to check if there are any
     * completed operations that need to be returned */
    /* FIXME: this is an expensive loop over all pending operations
     * everytime the user does an eqget */
    for (op = rptl_info.op_list; op; op = op->next) {
        if (op->events_ready) {
            retrieve_event(rptl, op, event);
            ret = PTL_OK;
            goto fn_exit;
        }
    }

    /* see if pause ack messages need to be sent out */
    tmp_ret = send_pause_ack_messages();
    if (tmp_ret) {
        ret = tmp_ret;
        RPTLU_ERR_POP(ret, "Error returned from send_pause_ack_messages\n");
    }

    /* see if unpause messages need to be sent out */
    tmp_ret = send_unpause_messages();
    if (tmp_ret) {
        ret = tmp_ret;
        RPTLU_ERR_POP(ret, "Error returned from send_unpause_messages\n");
    }

    /* see if there are any pending ops to be issued */
    tmp_ret = issue_pending_ops();
    if (tmp_ret) {
        ret = tmp_ret;
        RPTLU_ERR_POP(ret, "Error returned from issue_pending_ops\n");
    }

    ret = PtlEQGet(eq_handle, event);
    if (ret == PTL_EQ_EMPTY)
        goto fn_exit;

    /* find the rptl and op associated with this event */
    get_event_info(event, &rptl, &op);

    /* PT_DISABLED events only occur on the target */
    if (event->type == PTL_EVENT_PT_DISABLED) {
        /* we hide PT disabled events from the user */
        ret = PTL_EQ_EMPTY;

        /* we should only receive disable events on the data pt */
        assert(rptl->data.pt == event->pt_index);

        /* if we don't have a control PT, we don't have a way to
         * recover from disable events */
        assert(rptl->control.pt != PTL_PT_ANY);

        rptl->local_state = RPTL_LOCAL_STATE_AWAITING_PAUSE_ACKS;
        rptl->pause_ack_counter = 0;

        /* send out pause messages */
        tmp_ret = send_pause_messages(rptl);
        if (tmp_ret) {
            ret = tmp_ret;
            RPTLU_ERR_POP(ret, "Error returned from send_pause_messages\n");
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
        tmp_ret = post_empty_buffer(rptl->ni, rptl->control.pt,
                                    &rptl->control.me[rptl->control.me_idx]);
        if (tmp_ret) {
            ret = tmp_ret;
            RPTLU_ERR_POP(ret, "Error returned from post_empty_buffer\n");
        }
        rptl->control.me_idx++;
        if (rptl->control.me_idx >= 2 * rptl_info.world_size)
            rptl->control.me_idx = 0;

        if (event->hdr_data == RPTL_CONTROL_MSG_PAUSE) {
            tmp_ret = alloc_target(event->initiator, RPTL_TARGET_STATE_RECEIVED_PAUSE, rptl);
            if (tmp_ret) {
                ret = tmp_ret;
                RPTLU_ERR_POP(ret, "Error returned from alloc_target\n");
            }
        }
        else if (event->hdr_data == RPTL_CONTROL_MSG_PAUSE_ACK) {
            rptl->pause_ack_counter++;
        }
        else {  /* got an UNPAUSE message */
            /* reissue all operations to this target */
            tmp_ret = reissue_ops(event->initiator);
            if (tmp_ret) {
                ret = tmp_ret;
                RPTLU_ERR_POP(ret, "Error returned from reissue_ops\n");
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

            op->state = RPTL_OP_STATE_NACKED;

            if (op->op_type == RPTL_OP_PUT) {
                assert(!(event->type == PTL_EVENT_SEND && op->u.put.send));
                assert(!(event->type == PTL_EVENT_ACK && op->u.put.ack));

                /* if we have received both events, discard them.
                 * otherwise, stash the one we received while waiting
                 * for the other. */
                if (event->type == PTL_EVENT_SEND && op->u.put.ack) {
                    MPIU_Free(op->u.put.ack);
                    op->u.put.ack = NULL;
                }
                else if (event->type == PTL_EVENT_ACK && op->u.put.send) {
                    MPIU_Free(op->u.put.send);
                    op->u.put.send = NULL;
                }
                else {
                    ret = stash_event(op, *event);
                    RPTLU_ERR_POP(ret, "error stashing event\n");
                }
            }

            if (op->op_type == RPTL_OP_PUT)
                tmp_ret = alloc_target(op->u.put.target_id, RPTL_TARGET_STATE_DISABLED, NULL);
            else
                tmp_ret = alloc_target(op->u.get.target_id, RPTL_TARGET_STATE_DISABLED, NULL);
            if (tmp_ret) {
                ret = tmp_ret;
                RPTLU_ERR_POP(ret, "Error returned from alloc_target\n");
            }
        }

        /* if this is a REPLY event, we are done with this op */
        else if (event->type == PTL_EVENT_REPLY) {
            assert(op->op_type == RPTL_OP_GET);

            event->user_ptr = op->u.get.user_ptr;
            MPL_DL_DELETE(rptl_info.op_list, op);
            free_op(op);
        }

        else if (event->type == PTL_EVENT_SEND && op->u.put.ack) {
            assert(op->op_type == RPTL_OP_PUT);

            /* we already got the other event we needed earlier.  mark
             * the op events as ready and return this current event to
             * the user. */
            op->events_ready = 1;
            event->user_ptr = op->u.put.user_ptr;

            /* if flow control is not set, ignore events */
            if (op->u.put.flow_control == 0) {
                retrieve_event(rptl, op, event);
                ret = PTL_EQ_EMPTY;
            }
        }

        else if (event->type == PTL_EVENT_ACK && op->u.put.send) {
            assert(op->op_type == RPTL_OP_PUT);

            /* we already got the other event we needed earlier.  mark
             * the op events as ready and return this current event to
             * the user. */
            op->events_ready = 1;
            event->user_ptr = op->u.put.user_ptr;

            /* if flow control is not set, ignore events */
            if (op->u.put.flow_control == 0) {
                retrieve_event(rptl, op, event);
                ret = PTL_EQ_EMPTY;
            }

            /* if the user asked for an ACK, just return this event.
             * if not, discard this event and retrieve the send
             * event. */
            else if (!(op->u.put.ack_req & PTL_ACK_REQ))
                retrieve_event(rptl, op, event);
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
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_EQGET);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}
