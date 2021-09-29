/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_debug.h"

/* --BEGIN ERROR HANDLING-- */
void MPID_nem_dbg_dump_cell (volatile struct MPID_nem_cell *cell)
{

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_D (MPIR_DBG_OTHER, TERSE, "  src = %6d", cell->header.source);
    MPL_DBG_MSG_D (MPIR_DBG_OTHER, TERSE, "  dst = %6d", cell->header.dest);
    MPL_DBG_MSG_D (MPIR_DBG_OTHER, TERSE, "  len = %6d", (int)cell->header.datalen);
    MPL_DBG_MSG_D (MPIR_DBG_OTHER, TERSE, "  sqn = %6d", cell->header.seqno);
    MPL_DBG_MSG_D (MPIR_DBG_OTHER, TERSE, "  typ = %6d", cell->header.type);

    MPIR_FUNC_EXIT;
}

#define state_case(suffix)             \
    case MPIDI_VC_STATE_##suffix:       \
        return "MPIDI_VC_STATE_" #suffix; \
        break

static const char *vc_state_to_str(MPIDI_VC_State_t state)
{
    switch (state) {
        state_case(INACTIVE);
        state_case(ACTIVE);
        state_case(LOCAL_CLOSE);
        state_case(REMOTE_CLOSE);
        state_case(CLOSE_ACKED);
        state_case(CLOSED);
        state_case(INACTIVE_CLOSED);
        state_case(MORIBUND);
        default: return "(invalid state)"; break;
    }
}
#undef state_case

/* satisfy the compiler */
void MPID_nem_dbg_print_vc_sendq(FILE *stream, MPIDI_VC_t *vc);

/* This function can be called by a debugger to dump the sendq state of the
   given vc to the given stream. */
void MPID_nem_dbg_print_vc_sendq(FILE *stream, MPIDI_VC_t *vc)
{
    MPIR_Request * sreq;
    int i;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;

    fprintf(stream, "..VC ptr=%p pg_rank=%d state=%s:\n", vc, vc->pg_rank, vc_state_to_str(vc->state));
    if (vc_ch->is_local) {
        fprintf(stream, "....shm_active_send\n");
        sreq = MPIDI_CH3I_shm_active_send;
        if (sreq) {
            fprintf(stream, "....    sreq=%p ctx=%#x rank=%d tag=%d\n", sreq,
                            sreq->dev.match.parts.context_id,
                            sreq->dev.match.parts.rank,
                            sreq->dev.match.parts.tag);
        }

        fprintf(stream, "....shm send queue (head-to-tail)\n");
        sreq = MPIDI_CH3I_Sendq_head(MPIDI_CH3I_shm_sendq);
        i = 0;
        while (sreq != NULL) {
            fprintf(stream, "....[%d] sreq=%p ctx=%#x rank=%d tag=%d\n", i, sreq,
                            sreq->dev.match.parts.context_id,
                            sreq->dev.match.parts.rank,
                            sreq->dev.match.parts.tag);
            ++i;
            sreq = sreq->next;
        }
    }
    else {
        if (MPID_nem_net_module_vc_dbg_print_sendq != NULL) {
            MPID_nem_net_module_vc_dbg_print_sendq(stream, vc);
        }
        else {
            fprintf(stream, "..no MPID_nem_net_module_vc_dbg_print_sendq function available\n");
        }
    }
}

/* satisfy the compiler */
void MPID_nem_dbg_print_all_sendq(FILE *stream);

/* This function can be called by a debugger to dump the sendq state for all the
   vcs for all the pgs to the given stream. */
void MPID_nem_dbg_print_all_sendq(FILE *stream)
{
    int i;
    MPIDI_PG_t *pg;
    MPIDI_VC_t *vc;
    MPIDI_PG_iterator iter;

    fprintf(stream, "========================================\n");
    fprintf(stream, "MPI_COMM_WORLD  ctx=%#x rank=%d\n", MPIR_Process.comm_world->context_id, MPIR_Process.comm_world->rank);
    fprintf(stream, "MPI_COMM_SELF   ctx=%#x\n", MPIR_Process.comm_self->context_id);
    if (MPIR_Process.comm_parent) {
        fprintf(stream, "MPI_COMM_PARENT ctx=%#x recvctx=%#x\n",
                MPIR_Process.comm_self->context_id,
                MPIR_Process.comm_parent->recvcontext_id);
    }
    else {
        fprintf(stream, "MPI_COMM_PARENT (NULL)\n");
    }

    MPIDI_PG_Get_iterator(&iter);
    while (MPIDI_PG_Has_next(&iter)) {
        MPIDI_PG_Get_next(&iter, &pg);
        fprintf(stream, "PG ptr=%p size=%d id=%s refcount=%d\n", pg, pg->size, (const char*)pg->id, MPIR_Object_get_ref(pg));
        for (i = 0; i < MPIDI_PG_Get_size(pg); ++i) {
            MPIDI_PG_Get_vc(pg, i, &vc);
            MPID_nem_dbg_print_vc_sendq(stream, vc);
        }
    }

    fprintf(stream, "========================================\n");
}

/* --END ERROR HANDLING-- */
