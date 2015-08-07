/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "rptl.h"

#undef FUNCNAME
#define FUNCNAME rptli_post_control_buffer
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int rptli_post_control_buffer(ptl_handle_ni_t ni_handle, ptl_pt_index_t pt,
                              ptl_handle_me_t * me_handle)
{
    int ret;
    ptl_me_t me;
    ptl_process_t id;
    MPIDI_STATE_DECL(MPID_STATE_RPTLI_POST_CONTROL_BUFFER);

    MPIDI_FUNC_ENTER(MPID_STATE_RPTLI_POST_CONTROL_BUFFER);

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

    while (1) {
        ret = PtlMEAppend(ni_handle, pt, &me, PTL_PRIORITY_LIST, NULL, me_handle);
        if (ret != PTL_NO_SPACE)
            break;
    }
    RPTLU_ERR_POP(ret, "Error appending empty buffer to priority list\n");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_RPTLI_POST_CONTROL_BUFFER);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_init(int world_size, uint64_t max_origin_events,
                           int (*get_target_info) (int rank, ptl_process_t * id,
                                                   ptl_pt_index_t local_data_pt,
                                                   ptl_pt_index_t * target_data_pt,
                                                   ptl_pt_index_t * target_control_pt))
{
    int ret = PTL_OK;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_INIT);

    rptl_info.rptl_list = NULL;
    rptl_info.target_list = NULL;

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
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_ptl_rptl_drain_eq(int eq_count, ptl_handle_eq_t *eq)
{
    int ret = PTL_OK;
    ptl_event_t event;
    struct rptl_op_pool_segment *op_segment;
    int i;
    struct rptl_target *target, *t;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_PTL_RPTL_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_PTL_RPTL_FINALIZE);

    for (target = rptl_info.target_list; target; target = target->next) {
        while (target->control_op_list || target->data_op_list) {
            for (i = 0; i < eq_count; i++) {
                /* read and ignore all events */
                ret = MPID_nem_ptl_rptl_eqget(eq[i], &event);
                if (ret == PTL_EQ_EMPTY)
                    ret = PTL_OK;
                RPTLU_ERR_POP(ret, "Error calling MPID_nem_ptl_rptl_eqget\n");
            }
        }
    }

    for (target = rptl_info.target_list; target;) {
        assert(target->data_op_list == NULL);
        assert(target->control_op_list == NULL);

        while (target->op_segment_list) {
            op_segment = target->op_segment_list;
            MPL_DL_DELETE(target->op_segment_list, op_segment);
            MPIU_Free(op_segment);
        }

        t = target->next;
        MPIU_Free(target);
        target = t;
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_PTL_RPTL_FINALIZE);
    return ret;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_ptl_rptl_ptinit
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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

    rptl->local_state = RPTL_LOCAL_STATE_ACTIVE;
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
            ret = rptli_post_control_buffer(rptl->ni, rptl->control.pt, &rptl->control.me[i]);
            RPTLU_ERR_POP(ret, "Error in rptli_post_control_buffer\n");
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
#define FCNAME MPL_QUOTE(FUNCNAME)
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
