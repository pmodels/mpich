/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidrma.h"

#ifdef USE_MPIU_INSTR
MPIU_INSTR_DURATION_EXTERN_DECL(rmaqueue_alloc);
MPIU_INSTR_DURATION_EXTERN_DECL(rmaqueue_set);
extern void MPIDI_CH3_RMA_InitInstr(void);
#endif

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
    int rank, origin_predefined, result_predefined, target_predefined;
    int dt_contig ATTRIBUTE((unused));
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIU_CHKLMEM_DECL(2);
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

    origin_predefined = TRUE; /* quiet uninitialized warnings (b/c goto) */
    if (op != MPI_NO_OP) {
        MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, origin_predefined);
    }
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(result_datatype, result_predefined);
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, target_predefined);

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
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
        MPIDI_VC_t *orig_vc, *target_vc;

        /* Append the operation to the window's RMA ops queue */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* TODO: Can we use the MPIDI_RMA_ACC_CONTIG optimization? */

        MPIU_INSTR_DURATION_START(rmaqueue_set);
        new_ptr->type = MPIDI_RMA_GET_ACCUMULATE;
        /* Cast away const'ness for origin_address as MPIDI_RMA_Op_t
         * contain both PUT and GET like ops */
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_count = result_count;
        new_ptr->result_datatype = result_datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->target_disp = target_disp;
        new_ptr->target_count = target_count;
        new_ptr->target_datatype = target_datatype;
        new_ptr->op = op;
        MPIU_INSTR_DURATION_END(rmaqueue_set);

	/* check if target is local and shared memory is allocated on window,
	  if so, we do not need to increment reference counts on datatype. This is
	  because this operation will be directly done on shared memory region, instead
	  of sending and receiving through the progress engine, therefore datatype
	  will not be referenced by the progress engine */

        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
	if (!(win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
            /* if source or target datatypes are derived, increment their
               reference counts */
            if (!origin_predefined) {
                MPID_Datatype_get_ptr(origin_datatype, dtp);
                MPID_Datatype_add_ref(dtp);
            }
            if (!result_predefined) {
                MPID_Datatype_get_ptr(result_datatype, dtp);
                MPID_Datatype_add_ref(dtp);
            }
            if (!target_predefined) {
                MPID_Datatype_get_ptr(target_datatype, dtp);
                MPID_Datatype_add_ref(dtp);
            }
        }
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
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

    /* The datatype must be predefined, and one of: C integer, Fortran integer,
     * Logical, Multi-language types, or Byte.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        mpi_errno = MPIDI_CH3I_Shm_cas_op(origin_addr, compare_addr, result_addr,
                                          datatype, target_rank, target_disp, win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append this operation to the RMA ops queue */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        MPIU_INSTR_DURATION_START(rmaqueue_set);
        new_ptr->type = MPIDI_RMA_COMPARE_AND_SWAP;
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = 1;
        new_ptr->origin_datatype = datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->target_disp = target_disp;
        new_ptr->target_count = 1;
        new_ptr->target_datatype = datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_count = 1;
        new_ptr->result_datatype = datatype;
        new_ptr->compare_addr = (void *) compare_addr;
        new_ptr->compare_count = 1;
        new_ptr->compare_datatype = datatype;
        MPIU_INSTR_DURATION_END(rmaqueue_set);
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

    /* The datatype and op must be predefined.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        mpi_errno = MPIDI_CH3I_Shm_fop_op(origin_addr, result_addr, datatype,
                                          target_rank, target_disp, op, win_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append this operation to the RMA ops queue */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        MPIU_INSTR_DURATION_START(rmaqueue_set);
        new_ptr->type = MPIDI_RMA_FETCH_AND_OP;
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = 1;
        new_ptr->origin_datatype = datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->target_disp = target_disp;
        new_ptr->target_count = 1;
        new_ptr->target_datatype = datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_count = 1;
        new_ptr->result_datatype = datatype;
        new_ptr->op = op;
        MPIU_INSTR_DURATION_END(rmaqueue_set);
    }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_FETCH_AND_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
