/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidrma.h"

MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_rmaqueue_alloc);
MPIR_T_PVAR_DOUBLE_TIMER_DECL_EXTERN(RMA, rma_rmaqueue_set);

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Get_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int rank;
    int dt_contig ATTRIBUTE((unused));
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET_ACCUMULATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET_ACCUMULATE);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(target_count, target_datatype, dt_contig, data_sz,
                            dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank && win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
           if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
           the same node. However, in ch3:sock, even if origin and target are on the same node, they do
           not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
           which is only set to TRUE when SHM region is allocated in nemesis.
           In future we need to figure out a way to check if origin and target are in the same "SHM comm".
        */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id))
    {
        mpi_errno = MPIDI_CH3I_Shm_get_acc_op(origin_addr, origin_count, origin_datatype,
                                              result_addr, result_count, result_datatype,
                                              target_rank, target_disp, target_count, target_datatype,
                                              op, win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append the operation to the window's RMA ops queue */
        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* TODO: Can we use the MPIDI_RMA_ACC_CONTIG optimization? */

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        if (op == MPI_NO_OP) {
            /* Convert GAcc to a Get */
            MPIDI_CH3_Pkt_get_t *get_pkt = &(new_ptr->pkt.get);
            MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
            get_pkt->addr = (char *) win_ptr->base_addrs[target_rank] +
                win_ptr->disp_units[target_rank] * target_disp;
            get_pkt->count = target_count;
            get_pkt->datatype = target_datatype;
            get_pkt->dataloop_size = 0;
            get_pkt->target_win_handle = win_ptr->all_win_handles[target_rank];
            get_pkt->source_win_handle = win_ptr->handle;

            new_ptr->origin_addr = result_addr;
            new_ptr->origin_count = result_count;
            new_ptr->origin_datatype = result_datatype;
            new_ptr->target_rank = target_rank;
        }

        else {
            MPIDI_CH3_Pkt_accum_t *accum_pkt = &(new_ptr->pkt.accum);
            MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_GET_ACCUM);
            accum_pkt->addr = (char *) win_ptr->base_addrs[target_rank] +
                win_ptr->disp_units[target_rank] * target_disp;
            accum_pkt->count = target_count;
            accum_pkt->datatype = target_datatype;
            accum_pkt->dataloop_size = 0;
            accum_pkt->op = op;
            accum_pkt->target_win_handle = win_ptr->all_win_handles[target_rank];
            accum_pkt->source_win_handle = win_ptr->handle;

            new_ptr->origin_addr = (void *) origin_addr;
            new_ptr->origin_count = origin_count;
            new_ptr->origin_datatype = origin_datatype;
            new_ptr->result_addr = result_addr;
            new_ptr->result_count = result_count;
            new_ptr->result_datatype = result_datatype;
            new_ptr->target_rank = target_rank;
        }

        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);

        /* if source or target datatypes are derived, increment their
           reference counts */
        if (op != MPI_NO_OP && !MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(result_datatype)) {
            MPID_Datatype_get_ptr(result_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
    }

 fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_GET_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Compare_and_swap
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMPARE_AND_SWAP);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_COMPARE_AND_SWAP);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank && win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
           if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
           the same node. However, in ch3:sock, even if origin and target are on the same node, they do
           not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
           which is only set to TRUE when SHM region is allocated in nemesis.
           In future we need to figure out a way to check if origin and target are in the same "SHM comm".
        */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* The datatype must be predefined, and one of: C integer, Fortran integer,
     * Logical, Multi-language types, or Byte.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id))
    {
        mpi_errno = MPIDI_CH3I_Shm_cas_op(origin_addr, compare_addr, result_addr,
                                          datatype, target_rank, target_disp, win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        MPIDI_CH3_Pkt_cas_t *cas_pkt = NULL;

        /* Append this operation to the RMA ops queue */
        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);

        cas_pkt = &(new_ptr->pkt.cas);
        MPIDI_Pkt_init(cas_pkt, MPIDI_CH3_PKT_CAS);
        cas_pkt->addr = (char *) win_ptr->base_addrs[target_rank] +
            win_ptr->disp_units[target_rank] * target_disp;
        cas_pkt->datatype = datatype;
        cas_pkt->target_win_handle = win_ptr->all_win_handles[target_rank];
        cas_pkt->source_win_handle = win_ptr->handle;

        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = 1;
        new_ptr->origin_datatype = datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_datatype = datatype;
        new_ptr->compare_addr = (void *) compare_addr;
        new_ptr->compare_datatype = datatype;
        new_ptr->target_rank = target_rank;
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);
    }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_COMPARE_AND_SWAP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Fetch_and_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Fetch_and_op(const void *origin_addr, void *result_addr,
                       MPI_Datatype datatype, int target_rank,
                       MPI_Aint target_disp, MPI_Op op, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_FETCH_AND_OP);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_FETCH_AND_OP);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank && win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
           if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
           the same node. However, in ch3:sock, even if origin and target are on the same node, they do
           not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
           which is only set to TRUE when SHM region is allocated in nemesis.
           In future we need to figure out a way to check if origin and target are in the same "SHM comm".
        */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* The datatype and op must be predefined.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id))
    {
        mpi_errno = MPIDI_CH3I_Shm_fop_op(origin_addr, result_addr, datatype,
                                          target_rank, target_disp, op, win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        MPIDI_CH3_Pkt_fop_t *fop_pkt = NULL;

        /* Append this operation to the RMA ops queue */
        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        MPIR_T_PVAR_TIMER_START(RMA, rma_rmaqueue_set);
        fop_pkt = &(new_ptr->pkt.fop);
        MPIDI_Pkt_init(fop_pkt, MPIDI_CH3_PKT_FOP);
        fop_pkt->addr = (char *) win_ptr->base_addrs[target_rank] +
            win_ptr->disp_units[target_rank] * target_disp;
        fop_pkt->datatype = datatype;
        fop_pkt->op = op;
        fop_pkt->source_win_handle = win_ptr->handle;
        fop_pkt->target_win_handle = win_ptr->all_win_handles[target_rank];

        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = 1;
        new_ptr->origin_datatype = datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_datatype = datatype;
        new_ptr->target_rank = target_rank;
        MPIR_T_PVAR_TIMER_END(RMA, rma_rmaqueue_set);
    }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_FETCH_AND_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
