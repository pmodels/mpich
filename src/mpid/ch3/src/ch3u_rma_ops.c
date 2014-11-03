/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidrma.h"

static int enableShortACC = 1;

#define MPIDI_PASSIVE_TARGET_DONE_TAG  348297
#define MPIDI_PASSIVE_TARGET_RMA_TAG 563924

#undef FUNCNAME
#define FUNCNAME MPIDI_Put
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Put(const void *origin_addr, int origin_count, MPI_Datatype
              origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig ATTRIBUTE((unused)), rank;
    MPID_Datatype *dtp;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPIDI_msg_sz_t data_sz;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_PUT);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_PUT);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* If the put is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_put_op(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_put_t *put_pkt = NULL;

        /* queue it up */
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        if (new_ptr == NULL) {
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr, &new_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

        put_pkt = &(new_ptr->pkt.put);
        MPIDI_Pkt_init(put_pkt, MPIDI_CH3_PKT_PUT);
        put_pkt->addr = (char *) win_ptr->base_addrs[target_rank] +
            win_ptr->disp_units[target_rank] * target_disp;
        put_pkt->count = target_count;
        put_pkt->datatype = target_datatype;
        put_pkt->dataloop_size = 0;
        put_pkt->target_win_handle = win_ptr->all_win_handles[target_rank];
        put_pkt->source_win_handle = win_ptr->handle;

        /* FIXME: For contig and very short operations, use a streamlined op */
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->target_rank = target_rank;

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        /* if source or target datatypes are derived, increment their
         * reference counts */
        if (!MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_PUT);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Get
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Get(void *origin_addr, int origin_count, MPI_Datatype
              origin_datatype, int target_rank, MPI_Aint target_disp,
              int target_count, MPI_Datatype target_datatype, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig ATTRIBUTE((unused)), rank;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* If the get is a local operation, do it here */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_get_op(origin_addr, origin_count, origin_datatype, target_rank,
                                          target_disp, target_count, target_datatype, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_get_t *get_pkt = NULL;

        /* queue it up */
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        if (new_ptr == NULL) {
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr, &new_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

        get_pkt = &(new_ptr->pkt.get);
        MPIDI_Pkt_init(get_pkt, MPIDI_CH3_PKT_GET);
        get_pkt->addr = (char *) win_ptr->base_addrs[target_rank] +
            win_ptr->disp_units[target_rank] * target_disp;
        get_pkt->count = target_count;
        get_pkt->datatype = target_datatype;
        get_pkt->dataloop_size = 0;
        get_pkt->target_win_handle = win_ptr->all_win_handles[target_rank];
        get_pkt->source_win_handle = win_ptr->handle;

        /* FIXME: For contig and very short operations, use a streamlined op */
        new_ptr->origin_addr = origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->target_rank = target_rank;

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        /* if source or target datatypes are derived, increment their
         * reference counts */
        if (!MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_GET);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}



#undef FUNCNAME
#define FUNCNAME MPIDI_Accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype
                     origin_datatype, int target_rank, MPI_Aint target_disp,
                     int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig ATTRIBUTE((unused)), rank;
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_ACCUMULATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_ACCUMULATE);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_acc_op(origin_addr, origin_count, origin_datatype,
                                          target_rank, target_disp, target_count, target_datatype,
                                          op, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_accum_t *accum_pkt = NULL;

        /* queue it up */
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        if (new_ptr == NULL) {
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr, &new_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

        /* If predefined and contiguous, use a simplified element */
        if (MPIR_DATATYPE_IS_PREDEFINED(origin_datatype) &&
            MPIR_DATATYPE_IS_PREDEFINED(target_datatype) && enableShortACC) {
            MPI_Aint origin_type_size;
            size_t len;

            MPID_Datatype_get_size_macro(origin_datatype, origin_type_size);
            MPIU_Assign_trunc(len, origin_count * origin_type_size, size_t);
            if (MPIR_CVAR_CH3_RMA_ACC_IMMED && len <= MPIDI_RMA_IMMED_INTS * sizeof(int)) {
                MPIDI_CH3_Pkt_accum_immed_t *accumi_pkt;

                accumi_pkt = &(new_ptr->pkt.accum_immed);
                MPIDI_Pkt_init(accumi_pkt, MPIDI_CH3_PKT_ACCUM_IMMED);
                accumi_pkt->addr = (char *) win_ptr->base_addrs[target_rank] +
                    win_ptr->disp_units[target_rank] * target_disp;
                accumi_pkt->count = target_count;
                accumi_pkt->datatype = target_datatype;
                accumi_pkt->op = op;
                accumi_pkt->target_win_handle = win_ptr->all_win_handles[target_rank];
                accumi_pkt->source_win_handle = win_ptr->handle;

                new_ptr->origin_addr = (void *) origin_addr;
                new_ptr->origin_count = origin_count;
                new_ptr->origin_datatype = origin_datatype;
                new_ptr->target_rank = target_rank;

                mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
                if (mpi_errno)
                    MPIU_ERR_POP(mpi_errno);

                goto fn_exit;
            }
        }

        accum_pkt = &(new_ptr->pkt.accum);

        MPIDI_Pkt_init(accum_pkt, MPIDI_CH3_PKT_ACCUMULATE);
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
        new_ptr->target_rank = target_rank;

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        /* if source or target datatypes are derived, increment their
         * reference counts */
        if (!MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Get_accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Get_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op,
                         MPID_Win * win_ptr)
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

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    MPIDI_Datatype_get_info(target_count, target_datatype, dt_contig, data_sz, dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_get_acc_op(origin_addr, origin_count, origin_datatype,
                                              result_addr, result_count, result_datatype,
                                              target_rank, target_disp, target_count,
                                              target_datatype, op, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append the operation to the window's RMA ops queue */
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        if (new_ptr == NULL) {
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr, &new_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

        /* TODO: Can we use the MPIDI_RMA_ACC_CONTIG optimization? */

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

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);

        /* if source or target datatypes are derived, increment their
         * reference counts */
        if (op != MPI_NO_OP && !MPIR_DATATYPE_IS_PREDEFINED(origin_datatype)) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(result_datatype)) {
            MPID_Datatype_get_ptr(result_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
        }
        if (!MPIR_DATATYPE_IS_PREDEFINED(target_datatype)) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
            new_ptr->is_dt = 1;
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
                           MPI_Aint target_disp, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMPARE_AND_SWAP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_COMPARE_AND_SWAP);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
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
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_cas_op(origin_addr, compare_addr, result_addr,
                                          datatype, target_rank, target_disp, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_cas_t *cas_pkt = NULL;

        /* Append this operation to the RMA ops queue */
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        if (new_ptr == NULL) {
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr, &new_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

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

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
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
                       MPI_Aint target_disp, MPI_Op op, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_FETCH_AND_OP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_FETCH_AND_OP);

    MPIU_ERR_CHKANDJUMP(win_ptr->states.access_state == MPIDI_RMA_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    rank = win_ptr->comm_ptr->rank;

    if (win_ptr->shm_allocated == TRUE && target_rank != rank &&
        win_ptr->create_flavor != MPI_WIN_FLAVOR_SHARED) {
        /* check if target is local and shared memory is allocated on window,
         * if so, we directly perform this operation on shared memory region. */

        /* FIXME: Here we decide whether to perform SHM operations by checking if origin and target are on
         * the same node. However, in ch3:sock, even if origin and target are on the same node, they do
         * not within the same SHM region. Here we filter out ch3:sock by checking shm_allocated flag first,
         * which is only set to TRUE when SHM region is allocated in nemesis.
         * In future we need to figure out a way to check if origin and target are in the same "SHM comm".
         */
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
    }

    /* The datatype and op must be predefined.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED ||
        (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
        mpi_errno = MPIDI_CH3I_Shm_fop_op(origin_addr, result_addr, datatype,
                                          target_rank, target_disp, op, win_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPIDI_RMA_Op_t *new_ptr = NULL;
        MPIDI_CH3_Pkt_fop_t *fop_pkt = NULL;

        /* Append this operation to the RMA ops queue */
        new_ptr = MPIDI_CH3I_Win_op_alloc(win_ptr);
        if (new_ptr == NULL) {
            mpi_errno = MPIDI_CH3I_RMA_Cleanup_ops_aggressive(win_ptr, &new_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

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

        mpi_errno = MPIDI_CH3I_Win_enqueue_op(win_ptr, new_ptr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_FETCH_AND_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
